#pragma once

#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/ViewPanel.h>

#include <iomanip>
#include <sstream>
#include <vector>

#include "World/ProjectWorld.h"

namespace RoverLab {

class RoverMissionPanel final : public pipeframe::ui::ViewPanel {
public:
  RoverMissionPanel(ProjectWorld& world, pipeframe::ui::ProjectUIHost&) : world(world) {}

  void OnUpdate(float delta) override {
    refresh += delta;
    if (refresh < .2f) return;
    refresh = 0;
    RefreshView();
  }

protected:
  pipeframe::ui::View BuildView() override {
    using namespace pipeframe::ui;

    std::vector<View> children;
    children.push_back(views::Text("intro",
      "RL8 MISSION\nMicrobot: discover the finish, RETURN TO START, then SOLVE MAZE.\n"
      "Ultrasonic / LiDAR: explore, then select a scene destination.\n"
      "Reset/reload clears learning.")
        .FitHeight());

    auto& scene = world.simulation.GetScene();
    bool foundSolver = false;
    scene.Components().Each<RoverSolverComponent>([&](pipeframe::ecs::Entity entity) {
      foundSolver = true;
      const auto key = "mission-" + std::to_string(entity);
      if (!scene.IsActive(entity)) {
        children.push_back(views::Text(key + "-inactive",
          "The rover with Rover Solver is inactive. Enable the rover in the Hierarchy "
          "to use exploration and destination controls.")
            .FitHeight());
        return;
      }

      const auto& config = *scene.Components().Get<RoverSolverComponent>(entity);
      if (!config.enabled) {
        children.push_back(views::Text(key + "-disabled",
          "Rover Solver is disabled. Select RoverEntity, expand Rover Solver in the "
          "Inspector, and turn Enabled ON. Keep Rover Navigation disabled when using Solver.")
            .FitHeight());
        return;
      }

      const auto* mission = world.simulation.solver.Mission(entity);
      std::ostringstream text;
      text << std::fixed << std::setprecision(2);
      if ((mission && mission->timed) || (!mission && config.missionMode == "TimedExplore")) {
        text << "EXPLORATION | A* destination travel time\n";
        if (!mission) {
          text << "Press Play. Exploration duration: " << config.explorationDuration << ".";
        } else {
          text << MissionPhaseText(mission->phase) << '\n';
          text << mission->explorationOutcome << '\n';
          text << "Exploration: " << mission->explorationElapsed;
          if (mission->explorationLimit > 0) text << " / " << mission->explorationLimit;
          text << " simulation seconds"
               << (mission->explorationLimit == 0 ? " (no time limit)" : "") << '\n';
          text << "New observed area: " << mission->newArea
               << " m2 | Travel: " << mission->explorationDistance << " m\n";
          if (mission->explorationDistance > .01)
            text << "Area gained / travel: " << mission->newArea / mission->explorationDistance
                 << " m2/m\n";
          text << "Survey: " << mission->surveySeconds
               << " s | Recovery: " << mission->recoverySeconds << " s\n";
          text << "Observation areas: " << mission->eligibleAreas << " eligible / "
               << mission->reachableAreas << " reachable\n";
          if (mission->phase == MissionPhase::Explore && mission->expectedGain > 0)
            text << "Target estimate: " << mission->expectedGain << " m2 new area | "
                 << mission->targetTravelSeconds << " s travel\n";
          text << world.simulation.solver.Status(entity) << '\n';
          if (mission->destination)
            text << "Destination: " << mission->destination->x << ", " << mission->destination->y
                 << " m\n";
          if (mission->phase == MissionPhase::Navigate || mission->phase == MissionPhase::Arrived)
            text << "Planned: " << mission->plannedDistance << " m | Estimated "
                 << mission->plannedSeconds
                 << " s (distance + turns)\nLeg: " << mission->current.seconds << " s | "
                 << mission->current.distance << " m\n";
          text << mission->selectionMessage
               << "\nSafety: " << world.simulation.drive.StopReason(entity);
        }
        if (mission) {
          children.push_back(views::Text(key + "-navigation-state",
            std::string(MissionPhaseText(mission->phase)) + "\n" +
              world.simulation.solver.Status(entity) + "\n" + mission->selectionMessage)
              .FitHeight());
        }
        children.push_back(views::Row(key + "-cursor",
          {
            views::Button(key + "-normal",
              "NORMAL CURSOR",
              [this] {
                world.destinationTool.Cancel();
                RefreshView();
              }),
            views::Button(key + "-select",
              world.destinationTool.IsArmed(entity) ? "SELECTING: CLICK SCENE"
                                                    : "SELECT DESTINATION",
              [this, entity] {
                world.destinationTool.Arm(entity, world.simulation.solver);
                RefreshView();
              })
              .Selected(world.destinationTool.IsArmed(entity))
              .Enabled(mission && mission->CanSelectDestination()),
          }));
        children.push_back(views::Button(key + "-retry", "RETRY SAME DESTINATION", [this, entity] {
          world.simulation.solver.RetryDestination(entity);
          RefreshView();
        }).Enabled(mission && mission->phase == MissionPhase::NavigationBlocked));
        children.push_back(views::Button(key + "-cancel",
          "CANCEL ROUTE",
          [this, entity] {
            world.simulation.solver.CancelDestination(entity);
            RefreshView();
          })
            .Enabled(mission && (mission->phase == MissionPhase::Navigate ||
                                  mission->phase == MissionPhase::NavigationBlocked ||
                                  mission->phase == MissionPhase::Failed)));
        children.push_back(views::Button(key + "-until-complete",
          "EXPLORE UNTIL COMPLETE",
          [this, entity] {
            world.destinationTool.Cancel();
            world.simulation.solver.BeginTimedExploration(entity, 0);
            RefreshView();
          })
            .Enabled(mission && mission->phase != MissionPhase::Navigate &&
                     mission->phase != MissionPhase::NavigationBlocked));
        children.push_back(views::Row(key + "-explore",
          {
            views::Button("five",
              "EXPLORE 5 MIN",
              [this, entity] {
                world.destinationTool.Cancel();
                world.simulation.solver.BeginTimedExploration(entity, 300);
                RefreshView();
              })
              .Enabled(mission && mission->phase != MissionPhase::Navigate &&
                       mission->phase != MissionPhase::NavigationBlocked),
            views::Button("ten",
              "EXPLORE 10 MIN",
              [this, entity] {
                world.destinationTool.Cancel();
                world.simulation.solver.BeginTimedExploration(entity, 600);
                RefreshView();
              })
              .Enabled(mission && mission->phase != MissionPhase::Navigate &&
                       mission->phase != MissionPhase::NavigationBlocked),
          }));
        children.push_back(
          views::Button(key + "-finish", "FINISH EXPLORING / USE MAP", [this, entity] {
            world.simulation.solver.FinishExploration(entity);
            RefreshView();
          }).Enabled(mission && mission->phase == MissionPhase::Explore));
        children.push_back(views::Text(key, text.str()).FitHeight());
        children.push_back(views::Text(key + "-cursor-hint",
          "Select Destination changes the scene cursor to a yellow crosshair. "
          "Click known free floor in the scene. Escape or Normal Cursor cancels. "
          "Drag the minimap to pan; it does not set destinations.")
            .FitHeight());
        children.push_back(views::Text(key + "-hint",
          "Explore buttons retain the map. Until Complete has no time limit; blocked or uncertain "
          "runs stop with an explanation. "
          "The budget uses simulation time; Pause pauses it. Cyan shows the remaining route. "
          "Unknown or blocked destinations are refused.")
            .FitHeight());
        return;
      }
      const auto* cells = scene.Components().Get<MicromouseMazeComponent>(entity);
      text << "Goal tag: " << config.goalTag << " | "
           << (cells && cells->enabled ? "Cell wall flood fill" : config.algorithm) << '\n';
      text << "Solve: fastest estimated time through learned space (distance + turns)\n";

      if (!mission) {
        text << "Waiting for Play";
      } else {
        text << MissionPhaseText(mission->phase) << '\n';
        if (!mission->learnedGoal) {
          text << (mission->phase == MissionPhase::Explore
                     ? "Goal location unknown: searching floor for arrival marker\n"
                     : "Goal has not been discovered\n");
          if (const auto target = world.simulation.solver.SearchTarget(entity))
            text << "Search destination (estimated): " << target->x << ", " << target->y << " m\n";
        }
        text << world.simulation.solver.Status(entity) << '\n';
        if (const auto* learned = world.simulation.solver.LearnedCells(entity))
          text << "Cell centres visited: "
               << std::count(learned->visited.begin(), learned->visited.end(), true) << " / "
               << learned->visited.size() << '\n';
        text << "Current leg: " << mission->current.seconds << " s | " << mission->current.distance
             << " m\n";
        if (mission->first.success)
          text << "First goal: " << mission->first.seconds << " s | " << mission->first.distance
               << " m\n";
        if (mission->phase == MissionPhase::Repeat)
          text << "Remaining plan: " << mission->plannedDistance << " m | "
               << mission->plannedSeconds << " estimated seconds\n";
        if (mission->repeat.success)
          text << "Solve " << mission->repeatCount << ": " << mission->repeat.seconds << " s | "
               << mission->repeat.distance << " m\n";
        if (mission->phase == MissionPhase::Repeat || mission->repeatCount > 0)
          text << "Solve start offset: " << 100 * mission->repeatStartOffset << " cm\n";
        text << "Safety: " << world.simulation.drive.StopReason(entity);
      }

      children.push_back(views::Text(key, text.str()).FitHeight());
      children.push_back(views::Text(key + "-repeat-reason",
        "Return to Start stops at the original pose. Solve Maze starts a separate learned run. "
        "Solve plans a new route through observed free space, including turning cost. Unknown "
        "walls are never shortcuts.")
          .FitHeight());
      children.push_back(views::Button(key + "-return", "RETURN TO START", [this, entity] {
        world.simulation.solver.ReturnToStart(entity);
        RefreshView();
      }).Enabled(mission && mission->CanRepeat()));
      children.push_back(views::Button(key + "-solve", "SOLVE MAZE", [this, entity] {
        world.simulation.solver.SolveMaze(entity);
        RefreshView();
      }).Enabled(mission && mission->CanSolve()));
    });

    if (!foundSolver) {
      children.push_back(views::Text("solver-missing",
        "No Rover Solver in this scene.\n"
        "Open SCENES and choose Microbot for infrared maze missions, or Ultrasonic / LiDAR "
        "for explore-and-select.\n"
        "To configure your own rover, select RoverEntity, use + ADD COMPONENT > Rover Solver, "
        "set Mission Mode to TimedExplore, and disable Rover Navigation. "
        "Then press Play. Select Destination unlocks after exploration finishes.")
          .FitHeight());
    }

    children.push_back(views::Button("hide", "HIDE", [this] { Hide(); }));

    return views::Scroll("mission-scroll",
      views::Column("mission", std::move(children)).Padding(12).Spacing(12).FitHeight())
      .FillHeight();
  }

private:
  ProjectWorld& world;
  float refresh{1};
};

}  // namespace RoverLab

#pragma once

#include <PipeFrame/ECS/Scene.h>

#include <optional>
#include <string>

#include "MazeSolver.h"

namespace RoverLab {

enum class MissionPhase {
  Explore,
  GoalFound,
  ReturnToStart,
  Repeat,
  RepeatComplete,
  Failed,
  AwaitDestination,
  Navigate,
  NavigationBlocked,
  Arrived,
  ReadyToSolve
};

inline const char* MissionPhaseText(MissionPhase phase) {
  switch (phase) {
    case MissionPhase::Explore:
      return "Unknown-maze exploration";
    case MissionPhase::GoalFound:
      return "Goal discovered";
    case MissionPhase::ReturnToStart:
      return "Returning on learned map";
    case MissionPhase::ReadyToSolve:
      return "At start: ready to solve maze";
    case MissionPhase::Repeat:
      return "Solving learned maze";
    case MissionPhase::RepeatComplete:
      return "Solve completed";
    case MissionPhase::AwaitDestination:
      return "Ready: select a scene destination";
    case MissionPhase::Navigate:
      return "Navigating selected destination";
    case MissionPhase::NavigationBlocked:
      return "Destination NOT reached: navigation blocked";
    case MissionPhase::Arrived:
      return "Destination reached";
    case MissionPhase::Failed:
      return "Mission failed";
  }
  return "Unknown";
}

struct MissionResult {
  double seconds{};
  double distance{};
  bool success{};
};

// Learning belongs to this runtime session. Reset/reload/scene changes clear it.
// There is no copy of the authored environment or the unseen goal position here.
struct RoverMission {
  MissionPhase phase{MissionPhase::Explore};
  bool timed{};
  bool solveAfterReturn{};
  double explorationLimit{300}, explorationElapsed{}, plannedSeconds{}, plannedDistance{};
  // Zero limit means explore until no reachable unknown boundary remains.
  std::string explorationOutcome{"Exploring"};
  double observedArea{}, newArea{}, explorationDistance{}, recoverySeconds{}, surveySeconds{};
  double lastInformationAt{}, expectedGain{}, targetTravelSeconds{};
  unsigned reachableAreas{}, eligibleAreas{};
  std::optional<MazePoint> destination;
  std::string selectionMessage;

  bool CanSelectDestination() const {
    return timed && (phase == MissionPhase::AwaitDestination || phase == MissionPhase::Arrived);
  }

  std::optional<MazePoint> learnedGoal;
  pipeframe::SceneObject marker;
  double goalHeading{};
  double goalApproachSign{1};
  MazePoint lastMotionDelta{};
  std::optional<MazePoint> lastPosition;
  MissionResult current, first, repeat;
  std::string failure;
  unsigned repeatCount{};
  double repeatStartOffset{};
  std::vector<MazePoint> trail{{0, 0}};

  bool TravellingKnownMap() const {
    return phase == MissionPhase::ReturnToStart || phase == MissionPhase::Repeat ||
           phase == MissionPhase::Navigate;
  }

  bool Finished() const {
    return phase == MissionPhase::GoalFound || phase == MissionPhase::RepeatComplete ||
           phase == MissionPhase::Failed || phase == MissionPhase::AwaitDestination ||
           phase == MissionPhase::Arrived || phase == MissionPhase::NavigationBlocked ||
           phase == MissionPhase::ReadyToSolve;
  }

  bool CanRepeat() const {
    return learnedGoal && marker.IsValid() &&
           (phase == MissionPhase::GoalFound || phase == MissionPhase::RepeatComplete);
  }

  bool CanSolve() const {
    return learnedGoal && marker.IsValid() && phase == MissionPhase::ReadyToSolve;
  }

  void SimplifyTrail(double tolerance = .025) {
    if (trail.size() < 3) return;

    // Remove small recovery zigzags, retaining corners within 2.5 cm of
    // measured travel. Every resulting segment is rechecked before driving.
    std::vector<bool> keep(trail.size());
    keep.front() = keep.back() = true;
    std::vector<std::pair<std::size_t, std::size_t>> pending{{0, trail.size() - 1}};
    while (!pending.empty()) {
      const auto [first, last] = pending.back();
      pending.pop_back();
      const auto a = trail[first], b = trail[last];
      const double dx = b.x - a.x, dy = b.y - a.y, lengthSquared = dx * dx + dy * dy;
      double maximum = tolerance;
      std::size_t split = first;
      for (std::size_t i = first + 1; i < last; ++i) {
        const double t =
          lengthSquared > 1e-8
            ? std::clamp(
                ((trail[i].x - a.x) * dx + (trail[i].y - a.y) * dy) / lengthSquared, 0.0, 1.0)
            : 0;
        const double distance = std::hypot(trail[i].x - a.x - t * dx, trail[i].y - a.y - t * dy);
        if (distance > maximum) {
          maximum = distance;
          split = i;
        }
      }
      if (split != first) {
        keep[split] = true;
        pending.emplace_back(first, split);
        pending.emplace_back(split, last);
      }
    }
    std::vector<MazePoint> simplified;
    for (std::size_t i = 0; i < trail.size(); ++i)
      if (keep[i]) simplified.push_back(trail[i]);
    trail = std::move(simplified);
  }

  void Advance(double delta, MazePoint position, double spacing = .025) {
    if (lastPosition &&
        std::hypot(position.x - lastPosition->x, position.y - lastPosition->y) > 1e-6)
      lastMotionDelta = {position.x - lastPosition->x, position.y - lastPosition->y};
    if (!Finished()) {
      current.seconds += delta;
      if (lastPosition)
        current.distance += std::hypot(position.x - lastPosition->x, position.y - lastPosition->y);
    }

    if (phase == MissionPhase::Explore &&
        std::hypot(position.x - trail.back().x, position.y - trail.back().y) >= spacing) {
      // Close loops when odometry returns to an earlier segment. Keeping failed
      // exploratory detours would ask the return leg to repeat every recovery.
      // The tiny connector remains subject to mapping and live Drive safety.
      // Include the segment immediately before the current one: a short
      // reverse recovery often retraces exactly that segment. Excluding it
      // records the failed corner excursion as a mandatory repeat waypoint.
      for (std::size_t i = 1; i + 1 < trail.size(); ++i) {
        const auto a = trail[i - 1], b = trail[i];
        const double dx = b.x - a.x, dy = b.y - a.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared < 1e-8) continue;
        const double t =
          std::clamp(((position.x - a.x) * dx + (position.y - a.y) * dy) / lengthSquared, 0.0, 1.0);
        const MazePoint projection{a.x + t * dx, a.y + t * dy};
        if (std::hypot(position.x - projection.x, position.y - projection.y) <=
            std::min(.02, spacing * .8)) {
          trail.resize(i);
          trail.push_back(projection);
          break;
        }
      }

      // Keep corners, merge almost-collinear samples within a 3 mm deviation.
      // This is measured odometry, never the authored maze or true pose.
      bool merge = false;
      if (trail.size() >= 2) {
        const auto a = trail[trail.size() - 2], b = trail.back();
        const double dx = position.x - a.x, dy = position.y - a.y;
        const double length = std::hypot(dx, dy);
        merge =
          length > 0 &&
          std::abs((b.x - a.x) * dy - (b.y - a.y) * dx) / length < std::min(.003, spacing * .12) &&
          (b.x - a.x) * (position.x - b.x) + (b.y - a.y) * (position.y - b.y) > 0;
      }
      if (merge)
        trail.back() = position;
      else if (trail.size() < 20000)
        trail.push_back(position);
    }

    lastPosition = position;
  }

  void Fail(std::string reason) {
    phase = MissionPhase::Failed;
    failure = std::move(reason);
  }
};

}  // namespace RoverLab

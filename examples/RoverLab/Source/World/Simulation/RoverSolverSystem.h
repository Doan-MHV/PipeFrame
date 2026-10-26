#pragma once

#include <map>
#include <numbers>
#include <optional>
#include <unordered_set>

#include "Components/RoverNavigationComponent.h"
#include "Components/RoverSolverComponent.h"
#include "Events/RoverGoalEvent.h"
#include "RoverDriveSystem.h"
#include "RoverRangeObservations.h"
#include "World/Planning/CellMaze.h"
#include "World/Planning/FrontierPlanner.h"
#include "World/Planning/MeasuredRoute.h"
#include "World/Planning/RoverMission.h"
#include "World/Planning/TimedRoute.h"

namespace RoverLab {
class RoverSolverSystem final : public pipeframe::FixedUpdateSystem<void> {
  enum class Mode { Plan, Travel, Survey, Settle, Reverse, ConfirmGoal, Halted };

  struct State {
    pipeframe::SceneObject rover, controller;
    RoverSolverComponent config;
    std::uint64_t generation{};
    MazeMemory memory;
    RoverMission mission;
    MeasuredRoute measuredRoute;
    std::vector<double> pathHeadings;
    bool followingMeasuredRoute{};
    bool fastestSolve{};
    CellMaze cellMaze;
    bool cellReady{}, cellMoving{}, cellAligned{};
    int cellPhase{-1};
    MazePoint cellTarget;
    double cellBearing{}, cellStarted{}, cellTimeLimit{30};
    bool measuredPositionAccepted{};
    double measuredTurnLimit{1};
    unsigned routeFailures{}, routeResurveys{};
    bool coveringFloor{};
    bool followingTrail{}, reverseTravel{}, trailDirectionReady{};
    unsigned idleSurveys{};
    bool departure{};
    std::unordered_set<std::uint64_t> observed;
    std::size_t baselineObserved{};
    bool observationBaselineReady{};
    double nextIdleSurvey{};
    unsigned strandedRecoveries{};
    bool seekingTurnSpace{};
    MazeGrid grid;
    std::vector<MazePoint> path;
    std::vector<bool> pathReverse;
    std::size_t next{};
    std::optional<MazeKey> goal;

    Mode mode{Mode::Survey};

    double noNewCellTime{};
    double gridAge{1}, wait{}, blocked{}, stalled{}, elapsed{}, best{1e9};
    double lastHeading{}, scanned{}, settleTime{}, reverseX{}, reverseY{};
    double recoverySign{-1};

    bool headingReady{}, turning{}, commanded{};
    std::string status{"Waiting for mapping"};
  };

public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;

  RoverSolverSystem(pipeframe::BehaviourScene& scene,
    pipeframe::EventChannels& events,
    RoverMappingSystem& mapping,
    RoverDriveSystem& drive)
    : scene(scene), events(events), mapping(mapping), drive(drive), ranges(scene, events) {
    goalConnection = events.Listen<RoverGoalEvent>([this](const auto& event) {
      if (!event.rover.IsValid() || !event.marker.IsValid()) return;

      const auto* config = event.rover.template GetComponent<RoverSolverComponent>();
      if (config && config->enabled && config->goalTag == event.tag)
        arrivals.insert_or_assign(event.rover.GetEntity(), event);
    });
  }

  std::string_view GetSystemId() const override { return "roverlab.solver"; }

  void SetObjectResolver(Resolver value) {
    resolve = value;
    ranges.SetObjectResolver(value);
    Reset();
  }

  void Reset() {
    states.clear();
    arrivals.clear();
    ranges.Reset();
    time = 0;
  }

  std::string Status(pipeframe::ecs::Entity entity) const {
    const auto it = states.find(entity);
    return it == states.end() ? "Solver inactive" : it->second.status;
  }

  const RoverMission* Mission(pipeframe::ecs::Entity entity) const {
    const auto found = states.find(entity);
    return found == states.end() ? nullptr : &found->second.mission;
  }

  const CellMaze* LearnedCells(pipeframe::ecs::Entity entity) const {
    const auto found = states.find(entity);
    return found != states.end() && found->second.cellReady ? &found->second.cellMaze : nullptr;
  }

  std::optional<MazePoint> SearchTarget(pipeframe::ecs::Entity entity) const {
    const auto found = states.find(entity);
    if (found == states.end() || !found->second.goal ||
        found->second.mission.phase != MissionPhase::Explore)
      return std::nullopt;

    const auto& state = found->second;
    return MazePoint{
      state.goal->first * state.grid.cell / 2, state.goal->second * state.grid.cell / 2};
  }

  bool BeginTimedExploration(pipeframe::ecs::Entity entity, double seconds) {
    const auto it = states.find(entity);
    if (it == states.end() || (seconds != 0 && seconds != 300 && seconds != 600) ||
        it->second.mission.phase == MissionPhase::Navigate ||
        it->second.mission.phase == MissionPhase::NavigationBlocked)
      return false;
    auto& state = it->second;
    state.mission = RoverMission{};
    state.mission.timed = true;
    state.mission.explorationLimit = seconds;
    ResetRoute(state);
    state.noNewCellTime = 0;
    state.idleSurveys = 0;
    state.baselineObserved = state.observed.size();
    state.observationBaselineReady = true;
    // An explicit new session permits another attempt with the retained map.
    state.memory.trials.clear();
    state.memory.attempted.clear();
    state.memory.observationAreas.clear();
    state.status = seconds == 0 ? "Exploring until complete; existing map retained"
                                : "Exploration with time limit started; existing map retained";
    return true;
  }

  bool FinishExploration(pipeframe::ecs::Entity entity) {
    const auto it = states.find(entity);
    if (it == states.end() || !it->second.mission.timed ||
        it->second.mission.phase != MissionPhase::Explore)
      return false;
    EndExploration(it->second, "Stopped by user", "Map retained; select a scene destination.");
    return true;
  }

  bool SelectDestination(pipeframe::ecs::Entity entity, MazePoint point) {
    const auto it = states.find(entity);
    if (it == states.end()) return false;
    auto& state = it->second;
    auto& mission = state.mission;
    if (!mission.CanSelectDestination()) {
      mission.selectionMessage = "Wait for exploration to finish, or cancel the current route.";
      return false;
    }
    const auto* map = mapping.Find(entity);
    const int cell =
      map && std::isfinite(point.x) && std::isfinite(point.y) ? map->Index(point.x, point.y) : -1;
    if (cell < 0 || map->cells[cell] >= 0) {
      mission.selectionMessage =
        "Choose known free floor; unknown and occupied cells cannot be destinations.";
      return false;
    }
    mission.destination = point;
    mission.phase = MissionPhase::Navigate;
    mission.current = {};
    mission.lastPosition = MazePoint{map->pose.x, map->pose.y};
    mission.failure.clear();
    mission.selectionMessage = "Planning minimum estimated travel time through learned free space";
    mission.plannedSeconds = mission.plannedDistance = 0;
    ResetRoute(state);
    return true;
  }

  bool CancelDestination(pipeframe::ecs::Entity entity) {
    const auto it = states.find(entity);
    if (it == states.end() || !it->second.mission.timed ||
        (it->second.mission.phase != MissionPhase::Navigate &&
          it->second.mission.phase != MissionPhase::NavigationBlocked &&
          it->second.mission.phase != MissionPhase::Failed))
      return false;
    it->second.mission.destination.reset();
    it->second.mission.failure.clear();
    it->second.mission.plannedSeconds = it->second.mission.plannedDistance = 0;
    WaitForDestination(it->second, "Navigation cancelled; choose another destination");
    return true;
  }

  bool RetryDestination(pipeframe::ecs::Entity entity) {
    const auto it = states.find(entity);
    if (it == states.end() || !it->second.mission.timed ||
        it->second.mission.phase != MissionPhase::NavigationBlocked ||
        !it->second.mission.destination)
      return false;
    auto& state = it->second;
    state.mission.phase = MissionPhase::Navigate;
    state.mission.current = {};
    state.mission.lastPosition.reset();
    state.mission.failure.clear();
    state.mission.selectionMessage = "Retrying the same destination with current map evidence";
    ResetRoute(state);
    return true;
  }

  std::vector<MazePoint> PlannedPath(pipeframe::ecs::Entity entity) const {
    const auto it = states.find(entity);
    if (it == states.end() || !it->second.mission.TravellingKnownMap() ||
        it->second.mode != Mode::Travel)
      return {};
    const auto& state = it->second;
    const auto* map = mapping.Find(entity);
    if (!map) return {};
    std::vector<MazePoint> result{{map->pose.x, map->pose.y}};
    result.insert(
      result.end(), state.path.begin() + std::min(state.next, state.path.size()), state.path.end());
    return result;
  }

  // Compatibility for callers that explicitly request the combined sequence.
  bool ReturnAndRepeat(pipeframe::ecs::Entity entity) { return ReturnToStart(entity, true); }

  bool ReturnToStart(pipeframe::ecs::Entity entity, bool solveAfterReturn = false) {
    const auto found = states.find(entity);
    if (found == states.end() || !found->second.mission.CanRepeat()) return false;

    auto& state = found->second;
    state.fastestSolve = false;
    state.mission.solveAfterReturn = solveAfterReturn;
    state.mission.phase = MissionPhase::ReturnToStart;
    state.mission.current = {};
    state.mode = Mode::Travel;
    state.path.assign(state.mission.trail.rbegin(), state.mission.trail.rend());
    state.pathReverse.clear();
    state.next = 0;
    state.followingTrail = state.reverseTravel = true;
    state.trailDirectionReady = false;
    state.best = 1e9;
    state.stalled = state.blocked = 0;
    state.turning = false;
    state.goal.reset();
    state.wait = 0;
    state.gridAge = 1;
    state.noNewCellTime = 0;
    state.routeFailures = 0;
    state.routeResurveys = 0;
    state.strandedRecoveries = 0;
    state.seekingTurnSpace = false;
    state.status = "Return requested; retained learned map, live safety active";
    if (state.rover.GetComponent<MicromouseHardwareComponent>() && !state.cellReady)
      InstallMeasuredRoute(state, true);
    return true;
  }

  bool SolveMaze(pipeframe::ecs::Entity entity) {
    const auto found = states.find(entity);
    if (found == states.end() || !found->second.mission.CanSolve()) return false;
    auto& state = found->second;
    const auto* map = mapping.Find(entity);
    if (!map || std::hypot(map->pose.x, map->pose.y) > .08 ||
        std::abs(std::remainder(map->pose.heading, 2 * Pi)) > .02)
      return false;
    state.mission.phase = MissionPhase::Repeat;
    state.mission.current = {};
    state.mission.lastPosition = MazePoint{map->pose.x, map->pose.y};
    state.followingTrail = state.reverseTravel = false;
    state.path.clear();
    state.goal.reset();
    state.mode = Mode::Settle;
    state.settleTime = time;
    state.wait = 0;
    state.routeFailures = state.routeResurveys = 0;
    state.fastestSolve = true;
    state.followingMeasuredRoute = false;
    state.gridAge = 1;
    Send(state, 0, 0, "Solve requested; planning fastest route through learned free space");
    return true;
  }

  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) return;

    time += delta;
    ranges.Update(delta);
    std::erase_if(arrivals, [&](const auto& item) {
      return !item.second.rover.IsValid() || !item.second.marker.IsValid() ||
             time - item.second.simulationTime > .25;
    });

    std::erase_if(states, [&](auto& item) {
      auto& state = item.second;
      const auto* cfg = state.controller.IsValid()
                          ? state.controller.template GetComponent<RoverSolverComponent>()
                          : nullptr;
      const bool remove = !cfg || !cfg->enabled || !state.rover.IsValid() || !resolve ||
                          !scene.IsActive(state.controller.GetEntity()) ||
                          resolve(cfg->rover.objectId) != state.rover;

      if (remove) {
        RoverMotionCommandEvent stop;
        stop.simulationTime = time;
        stop.roverId = state.config.rover.objectId;
        events.Publish(stop);
      }

      return remove;
    });

    scene.Components().Each<RoverSolverComponent>([&](pipeframe::ecs::Entity entity) {
      const auto controller = scene.GetObject(entity);
      const auto& cfg = *controller.GetComponent<RoverSolverComponent>();
      if (!cfg.enabled || !scene.IsActive(entity) || !resolve) return;

      const auto rover = resolve(cfg.rover.objectId);
      if (!rover.IsValid() || !scene.IsActive(rover.GetEntity())) return;

      auto& state = states[rover.GetEntity()];
      const auto* map = mapping.Find(rover.GetEntity());
      if (state.rover != rover || state.controller != controller || state.config != cfg ||
          (map && map->generation != state.generation)) {
        state = State{};
        state.rover = rover;
        state.controller = controller;
        state.config = cfg;
        state.mission.timed = cfg.missionMode == "TimedExplore";
        state.mission.explorationLimit = cfg.explorationDuration == "Until complete" ? 0
                                         : cfg.explorationDuration == "10 minutes"   ? 600
                                                                                     : 300;
        state.generation = map ? map->generation : 0;
      }

      // The exploration budget is simulation time, even while observations
      // are absent or stale. A sensing fault must not keep the timer running forever.
      if (state.mission.timed && state.mission.phase == MissionPhase::Explore) {
        state.mission.explorationElapsed += delta;
        auto& progress = state.mission;
        if (state.mode == Mode::Reverse || state.blocked > 0) progress.recoverySeconds += delta;
        if (state.mode == Mode::Survey) progress.surveySeconds += delta;
        if (progress.explorationLimit > 0 &&
            progress.explorationElapsed >= progress.explorationLimit) {
          EndExploration(state,
            "Time limit reached",
            "Exploration paused at the time limit; this does not mean the map is complete.");
          return;
        }
        if (progress.explorationElapsed - progress.lastInformationAt >=
            std::max(cfg.noNewCellTimeout, progress.targetTravelSeconds + 30)) {
          EndExploration(state,
            "Blocked or uncertain",
            "No new observed area within the progress timeout. Check sensors, clearance and "
            "remaining boundaries; continue exploration or select a known destination.");
          return;
        }
      }

      int owners = 0;
      scene.Components().Each<RoverSolverComponent>([&](pipeframe::ecs::Entity id) {
        const auto& other = *scene.Components().Get<RoverSolverComponent>(id);
        owners += scene.IsActive(id) && other.enabled && other.rover.objectId == cfg.rover.objectId;
      });

      scene.Components().Each<RoverNavigationComponent>([&](pipeframe::ecs::Entity id) {
        const auto& other = *scene.Components().Get<RoverNavigationComponent>(id);
        owners += scene.IsActive(id) && other.enabled && other.rover.objectId == cfg.rover.objectId;
      });

      if (owners != 1) {
        Send(state, 0, 0, "Disable Rover Navigation; keep one controller per rover");
        return;
      }

      const auto* settings = rover.GetComponent<RoverDriveComponent>();
      const auto* body = rover.GetComponent<pipeframe::KinematicBody2DComponent>();
      const auto* safety = rover.GetComponent<RoverSafetyComponent>();
      const auto* collider = rover.GetComponent<pipeframe::EnvironmentCollider2DComponent>();
      const auto* transform = rover.GetComponent<pipeframe::Transform2DComponent>();

      if (!cfg.Valid() || !settings || !settings->enabled || !settings->ValidSettings() ||
          cfg.speed > settings->speed || cfg.turnRate > settings->maximumTurnRate || !body ||
          !body->enabled || !safety || !safety->ValidSettings() || !collider ||
          !collider->enabled || collider->trigger || collider->shape != "Box" ||
          collider->size.x <= 0 || collider->size.y <= 0 || !std::isfinite(collider->size.x) ||
          !std::isfinite(collider->size.y) || !transform ||
          transform->scale != pipeframe::Vector2f{1, 1}) {
        Send(state, 0, 0, "Check solver/drive settings, box collider and unit scale");
        return;
      }

      // Only scale/body dimensions are read above. Navigation never reads true position/rotation.
      if (!map || time - map->pose.time > settings->readingTimeout || map->width <= 0 ||
          map->height <= 0 || map->cells.size() > 250000) {
        Send(state, 0, 0, "Waiting for fresh mapping, or planning budget exceeded (250000 cells)");
        return;
      }

      const MazePoint position{map->pose.x, map->pose.y};
      const double heading = map->pose.heading;
      auto& mission = state.mission;
      // Centimetre-scale simplification can cut a large fraction of a small
      // micromouse's corner clearance. Scale trace fidelity to the actual chassis.
      const bool micromouse = rover.GetComponent<MicromouseHardwareComponent>() != nullptr;
      const double traceSpacing =
        micromouse
          ? std::min(.025, .05 * std::min(double(collider->size.x), double(collider->size.y)))
          : .025;
      mission.Advance(delta, position, traceSpacing);
      if (micromouse && !mission.timed && mission.phase == MissionPhase::Explore)
        state.measuredRoute.Record(position, heading);
      if (mission.timed && mission.phase == MissionPhase::Explore)
        mission.explorationDistance = mission.current.distance;
      state.memory.time += delta;
      std::erase_if(state.memory.blockedEdges,
        [&](const auto& edge) { return edge.second <= state.memory.time; });

      const auto arrival = arrivals.find(rover.GetEntity());
      if (!mission.timed && arrival != arrivals.end() && arrival->second.rover == rover &&
          arrival->second.tag == cfg.goalTag && arrival->second.simulationTime <= time &&
          time - arrival->second.simulationTime <= .1) {
        if (mission.phase == MissionPhase::Explore) {
          mission.marker = arrival->second.marker;
          mission.learnedGoal = position;
          mission.goalHeading = heading;
          mission.goalApproachSign = mission.lastMotionDelta.x * std::cos(heading) +
                                           mission.lastMotionDelta.y * std::sin(heading) <
                                         0
                                       ? -1
                                       : 1;
          mission.trail.push_back(position);
          mission.SimplifyTrail(traceSpacing);
          if (micromouse) {
            state.measuredRoute.Record(position, heading, true);
            state.measuredRoute.CloseLoops();
          }
          mission.current.success = true;
          mission.first = mission.current;
          mission.phase = MissionPhase::GoalFound;
        } else if (mission.phase == MissionPhase::Repeat &&
                   mission.marker == arrival->second.marker) {
          mission.current.success = true;
          mission.repeat = mission.current;
          ++mission.repeatCount;
          mission.phase = MissionPhase::RepeatComplete;
        }
      }

      if (!mission.Finished() && !(mission.timed && mission.phase == MissionPhase::Explore) &&
          mission.current.seconds >= cfg.runTimeout) {
        if (mission.timed)
          FailNavigation(state, "Navigation time limit reached; destination not reached.");
        else
          mission.Fail("Mission time limit reached; no successful arrival");
      }

      if (mission.Finished()) {
        Send(state,
          0,
          0,
          (mission.phase == MissionPhase::Failed ||
            mission.phase == MissionPhase::NavigationBlocked)
            ? mission.failure
          : mission.timed ? (mission.phase == MissionPhase::Arrived
                                ? "Destination reached; click another destination"
                                : "Stopped; use Select Destination to choose a scene location")
          : mission.phase == MissionPhase::ReadyToSolve
            ? "Returned and stopped. Press Solve Maze when ready"
            : "Goal reached; stopped. Press Return to Start");
        return;
      }

      unsigned coverage = 0;
      bool fullScan = false;
      bool fresh = true;
      double oldest = time;
      double sensorRange = std::numeric_limits<double>::infinity();

      ranges.Each([&](const RangeObservation& sample) {
        if (sample.settings.parent.objectId != cfg.rover.objectId) return;
        fresh &= sample.present && sample.valid && sample.age < settings->readingTimeout;
        fullScan |=
          sample.sensor.IsValid() && sample.sensor.GetComponent<LidarSensorComponent>() != nullptr;
        oldest = std::min(oldest, sample.capturedAt);
        sensorRange = std::min(sensorRange, sample.settings.maximumRange);
        for (int d = 0; d < 4; ++d)
          if (std::abs(std::remainder(sample.settings.localAngleDegrees - 90 * d, 360.0)) <=
              sample.settings.coneAngleDegrees / 2)
            coverage |= 1u << d;
      });

      if (!fresh || coverage != 15) {
        Send(state, 0, 0, "Waiting for four fresh sensor directions");
        return;
      }

      const auto previous = drive.SafetyStatus(rover.GetEntity());
      state.blocked =
        state.commanded && previous == RoverSafetyStatus::Obstacle ? state.blocked + delta : 0;

      if (const auto* cellSettings = rover.GetComponent<MicromouseMazeComponent>();
        cellSettings && cellSettings->enabled && !mission.timed) {
        FollowCellMaze(state, *cellSettings, position, heading, *safety, *collider, delta);
        return;
      }

      state.gridAge += delta;
      if (state.gridAge >= .5) {
        const double length = collider->size.x / 2, width = collider->size.y / 2;
        const double radius =
          std::hypot(std::max(length, safety->halfLength), std::max(width, safety->halfWidth)) +
          safety->clearance;
        if (radius > 2 || settings->stopDistance > 10) {
          Send(state, 0, 0, "Body/stopping envelope exceeds RL7 planning budget");
          return;
        }
        state.grid = BuildMazeGrid(*map,
          state.memory,
          std::max(length, safety->halfLength),
          std::max(width, safety->halfWidth),
          safety->clearance,
          // Learned endpoints may be closer than the exploratory look-ahead
          // buffer. Keep rectangular collision clearance; Drive still enforces
          // its unchanged stopping distance on every actual command.
          mission.TravellingKnownMap() ? 0.0 : settings->stopDistance,
          length,
          width,
          !mission.TravellingKnownMap(),
          mission.phase == MissionPhase::Navigate || state.fastestSolve);

        // Revisit penalties encourage first-run exploration. A learned return
        // must be allowed to retrace that safe route instead of avoiding it.
        if (mission.TravellingKnownMap())
          std::fill(state.grid.visits.begin(), state.grid.visits.end(), 0);

        if (mission.timed && mission.phase == MissionPhase::Explore) {
          const auto before = state.observed.size();
          for (int i = 0; i < state.grid.Count(); ++i) {
            if (map->cells[i] == 0) continue;
            const auto key = state.grid.Key(i);
            const auto stable =
              (std::uint64_t(std::uint32_t(key.first)) << 32) | std::uint32_t(key.second);
            state.observed.insert(stable);
          }
          if (!state.observationBaselineReady) {
            state.baselineObserved = before;
            state.observationBaselineReady = true;
          }
          if (state.observed.size() > before) {
            mission.lastInformationAt = mission.explorationElapsed;
            state.idleSurveys = 0;
          }
          mission.observedArea = state.observed.size() * state.grid.cell * state.grid.cell;
          mission.newArea =
            (state.observed.size() - state.baselineObserved) * state.grid.cell * state.grid.cell;
        }
        state.gridAge = 0;
      }

      auto& g = state.grid;
      const int here = g.At(position);
      if (here < 0) {
        Send(state, 0, 0, "Estimated rover is outside planning grid");
        return;
      }

      const auto key = g.Key(here);
      state.noNewCellTime += delta;
      if (!state.memory.last || *state.memory.last != key) {
        state.memory.last = key;
        auto& visits = state.memory.visits[key];
        if (visits == 0) {
          state.noNewCellTime = 0;
          state.idleSurveys = 0;
        }
        visits = std::min(100, visits + 1);
      }

      if (!mission.timed && mission.phase == MissionPhase::Explore &&
          state.noNewCellTime >= cfg.noNewCellTimeout)
        state.mode = Mode::Halted;
      if (state.mode == Mode::Halted) {
        mission.Fail("Exploration stalled: no new cells; Reset or change solver settings");
        Send(state, 0, 0, mission.failure);
        return;
      }

      if (mission.phase == MissionPhase::ReturnToStart &&
          std::hypot(position.x, position.y) <=
            (state.followingMeasuredRoute
                ? .0015
                : std::max(.06, g.cell * .75) + std::min(.025, g.cell * .25))) {
        const double error = std::remainder(-heading, 2 * Pi);
        if (std::abs(error) > .01) {
          Send(state,
            0,
            std::clamp(4 * error, -cfg.turnRate, cfg.turnRate),
            "At learned start; aligning to original heading");
          return;
        }

        mission.phase = MissionPhase::ReadyToSolve;
        mission.repeatStartOffset = std::hypot(position.x, position.y);
        state.path.clear();
        state.mode = Mode::Settle;
        Send(state, 0, 0, "Returned and stopped. Press Solve Maze when ready");
        if (mission.solveAfterReturn) SolveMaze(rover.GetEntity());
        return;
      }

      if (state.followingMeasuredRoute) {
        FollowMeasuredRoute(state, position, heading, delta);
        return;
      }

      if (state.mode == Mode::ConfirmGoal) {
        state.elapsed += delta;
        const double error = std::remainder(mission.goalHeading - heading, 2 * Pi);

        if (state.elapsed > 8 || state.blocked >= .6 ||
            std::hypot(position.x - state.reverseX, position.y - state.reverseY) > .06) {
          mission.Fail("Learned goal could not be confirmed by the floor detector");
          Send(state, 0, 0, mission.failure);
        } else if (std::abs(error) > .01) {
          Send(state,
            0,
            std::clamp(4 * error, -cfg.turnRate, cfg.turnRate),
            "Aligning to learned arrival direction");
        } else {
          Send(state,
            mission.goalApproachSign * std::min(.03, cfg.speed),
            0,
            "Confirming learned marker with a bounded 6 cm approach; live safety active");
        }
        return;
      }

      if (state.mode == Mode::Settle) {
        if (oldest < state.settleTime) {
          Send(state, 0, 0, "Waiting for observations after turn");
          return;
        }
        state.mode = state.path.empty() ? Mode::Plan : Mode::Travel;
      }

      if (state.mode == Mode::Survey) {
        // A fresh rotating LiDAR scan already covers a full revolution. Rotating
        // the entire rover again gains no bearing coverage and delays exploration.
        if (fullScan) {
          RememberFrontier(g, state.memory, key);
          state.mode = Mode::Plan;
          state.wait = 0;
          Send(state, 0, 0, "Full LiDAR scan available; planning without a body survey turn");
          return;
        }

        // A full sonar survey needs room for the whole chassis to rotate.
        // Starting it beside a corner can strand the rover at a non-cardinal
        // heading even though it could have continued straight or backed out.
        if (!state.headingReady && !MazeMotionGraph{g}.CanTurn(here)) {
          RememberFrontier(g, state.memory, key);
          state.mode = Mode::Settle;
          state.settleTime = time;
          state.elapsed = state.blocked = 0;
          Send(state, 0, 0, "Survey skipped: insufficient turning room; planning translation");
          return;
        }
        state.elapsed += delta;
        if (state.headingReady)
          state.scanned += std::max(0.0, std::remainder(heading - state.lastHeading, 2 * Pi));
        state.headingReady = true;
        state.lastHeading = heading;
        if (state.scanned >= 2 * Pi - .05 || state.blocked >= .6 ||
            state.elapsed > 2 * Pi / cfg.turnRate + 4) {
          RememberFrontier(g, state.memory, key);
          state.mode = Mode::Settle;
          state.settleTime = time;
          state.elapsed = state.blocked = 0;
          Send(state, 0, 0, "Survey ended; waiting for fresh observations");
        } else
          Send(state, 0, cfg.turnRate, "One bounded observation turn");
        return;
      }

      if (state.mode == Mode::Reverse) {
        state.elapsed += delta;
        const double recoveredDistance =
          std::hypot(position.x - state.reverseX, position.y - state.reverseY);
        const bool foundTurningSpace = state.seekingTurnSpace && recoveredDistance >= .025 &&
                                       !MazeMotionGraph{g}.Starts(here, heading).empty();
        if (foundTurningSpace || recoveredDistance >= .15 || state.blocked >= .6 ||
            state.elapsed >= 4) {
          state.seekingTurnSpace = false;
          state.mode = Mode::Settle;
          state.settleTime = time;
          state.gridAge = 1;
          Send(state, 0, 0, "Recovery ended; replanning around failed route");
        } else
          Send(state,
            state.recoverySign * std::min(.05, cfg.speed),
            0,
            state.recoverySign < 0 ? "Reverse recovery; Drive checks rear clearance"
                                   : "Forward retreat from refused reverse; live safety active");
        return;
      }

      if (state.mode == Mode::Plan &&
          (mission.phase == MissionPhase::Navigate || state.fastestSolve)) {
        const auto destination = state.fastestSolve ? mission.learnedGoal : mission.destination;
        const int goal = destination ? g.At(*destination) : -1;
        // Accept a destination within one cell. Tiny within-cell offsets must
        // not add a sideways turn at the end of an otherwise straight route.
        std::vector<int> endpoints;
        if (goal >= 0) {
          const auto root = g.Point(here);
          for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
              const int col = goal % g.width + dx, row = goal / g.width + dy;
              if (col < 0 || row < 0 || col >= g.width || row >= g.height) continue;
              const int cell = row * g.width + col;
              const auto point = g.Point(cell);
              if (std::hypot(point.x + position.x - root.x - destination->x,
                    point.y + position.y - root.y - destination->y) <= g.cell)
                endpoints.push_back(cell);
            }
        }
        const auto route =
          FastestKnownRoute(g, here, goal, heading, cfg.speed, cfg.turnRate, endpoints);
        if (route.cells.empty()) {
          if (StartDeparture(state,
                position,
                heading,
                goal,
                std::max(double(collider->size.x / 2), safety->halfLength) + safety->clearance,
                std::max(double(collider->size.y / 2), safety->halfWidth) + safety->clearance))
            return;
          FailNavigation(state,
            "No safe route from the current pose through known floor. Retry, or cancel this "
            "destination before choosing another.");
          return;
        }
        mission.plannedSeconds = route.seconds;
        mission.plannedDistance = (route.cells.size() - 1) * g.cell;
        mission.selectionMessage = "Cyan: planned route | Yellow: selected destination";
        InstallPath(state, route, position);
      }

      if (state.mode == Mode::Plan) {
        state.wait -= delta;
        if (state.wait > 0) {
          Send(state, 0, 0, state.status);
          return;
        }

        state.wait = 1;
        // A sonar survey can stop partway through a turn when new near-wall
        // evidence removes every cardinal start orientation. Another survey
        // at that same pose cannot restore turning room. Use the existing
        // bounded, live-sensor-guarded translation before trying to plan again.
        if (mission.phase != MissionPhase::Navigate &&
            MazeMotionGraph{g}.Starts(here, heading).empty() && state.strandedRecoveries < 2) {
          state.recoverySign = state.strandedRecoveries++ == 0 ? -1 : 1;
          state.seekingTurnSpace = true;
          state.mode = Mode::Reverse;
          state.elapsed = state.blocked = 0;
          state.reverseX = position.x;
          state.reverseY = position.y;
          Send(state, 0, 0, "No turning space after survey; bounded sensor-guarded retreat");
          return;
        }
        const MazePoint destination = mission.phase == MissionPhase::ReturnToStart
                                        ? MazePoint{0, 0}
                                        : mission.learnedGoal.value_or(position);
        const double endpointTolerance = mission.phase == MissionPhase::ReturnToStart
                                           ? std::max(.06, g.cell * .75)
                                           : std::max(.035, g.cell * .75);
        ExplorationChoice observation;
        if (mission.timed && mission.phase == MissionPhase::Explore) {
          std::vector<double> policyTimes;
          if (cfg.algorithm == "RightHand") {
            policyTimes.assign(g.Count(), std::numeric_limits<double>::infinity());
            const auto trace = RightHandMazeSolver{}.Trace(g, here, heading);
            double seconds = 0, facing = heading;
            for (std::size_t i = 0; i < trace.cells.size(); ++i) {
              const double nextHeading = trace.headings[i] * Pi / 2;
              seconds += RouteTurnSeconds(nextHeading - facing, cfg.turnRate);
              if (i > 0) seconds += g.cell / cfg.speed;
              policyTimes[trace.cells[i]] = std::min(policyTimes[trace.cells[i]], seconds);
              facing = nextHeading;
            }
          }
          observation = ChooseObservationArea(g,
            here,
            state.memory,
            heading,
            cfg.speed,
            cfg.turnRate,
            sensorRange,
            fullScan,
            true,
            policyTimes.empty() ? nullptr : &policyTimes);
          mission.reachableAreas = observation.reachableAreas;
          mission.eligibleAreas = observation.eligibleAreas;
          mission.expectedGain = observation.expectedArea;
          if (observation.goal >= 0) mission.targetTravelSeconds = observation.travelSeconds;
        }
        int goal = mission.TravellingKnownMap() ? ChooseKnownDestination(g,
                                                    here,
                                                    position,
                                                    destination,
                                                    endpointTolerance,
                                                    mission.phase == MissionPhase::ReturnToStart)
                   : mission.timed              ? observation.goal
                   : micromouse
                     ? ChooseMicromouseTarget(
                         g, here, state.memory, heading, cfg.searchSpacing, cfg.speed, cfg.turnRate)
                     : ChooseCoverageTarget(g, here, state.memory, cfg.searchSpacing);

        state.coveringFloor = !mission.timed && mission.phase == MissionPhase::Explore && goal >= 0;
        if (goal < 0 && !mission.timed && mission.phase == MissionPhase::Explore)
          goal = ChooseFrontier(g, here, state.memory);

        if (goal < 0) {
          if (mission.TravellingKnownMap()) {
            if (mission.phase == MissionPhase::Repeat && !state.fastestSolve &&
                StartLearnedTrace(state, position))
              return;
            if (!fullScan && state.routeResurveys++ < 3) {
              state.mode = Mode::Survey;
              state.scanned = state.elapsed = state.blocked = 0;
              state.headingReady = false;
              Send(state, 0, 0, "Learned route uncertain; bounded sonar resurvey before retry");
              return;
            }

            mission.Fail("No safe learned route to destination; Reset to explore again");
            Send(state, 0, 0, mission.failure);
            return;
          }

          if (mission.timed) {
            mission.explorationOutcome = observation.reachableAreas > 0
                                           ? "Waiting: observation areas cooling down or exhausted"
                                         : observation.boundaryCells > 0
                                           ? "Blocked or uncertain boundary"
                                           : "Checking completion";
            if (observation.boundaryCells == 0 &&
                mission.explorationElapsed - mission.lastInformationAt >= 15) {
              EndExploration(state,
                "Complete: no observed free-to-unknown boundaries",
                "Mapping complete for the observed reachable enclosure; select a scene "
                "destination.");
              return;
            }
          }
          if (!fullScan && state.idleSurveys < 3 && time >= state.nextIdleSurvey) {
            ++state.idleSurveys;
            state.nextIdleSurvey = time + 20;
            state.mode = Mode::Survey;
            state.scanned = state.elapsed = state.blocked = 0;
            state.headingReady = false;
            Send(state, 0, 0, "No eligible route; bounded sonar resurvey for ambiguous evidence");
            return;
          }

          Send(state, 0, 0, "No eligible frontier; waiting for changed evidence or bounded retry");
          return;
        }

        if (mission.timed && mission.phase == MissionPhase::Explore) {
          mission.explorationOutcome = "Exploring a new observation area";
          RecordObservationAttempt(g, state.memory, goal);
        }
        if (mission.timed && mission.phase == MissionPhase::Explore && cfg.algorithm == "AStar") {
          const auto route = FastestKnownRoute(g, here, goal, heading, cfg.speed, cfg.turnRate);
          if (route.cells.empty()) {
            RememberFrontier(g, state.memory, g.Key(goal));
            Send(state, 0, 0, "Observation area has no manoeuvrable route; choosing another");
            return;
          }
          InstallPath(state, route, position);
        } else {
          const auto solver = MakeMazeSolver(cfg.algorithm);
          auto routeGrid = g;
          if (state.coveringFloor || mission.timed)
            std::fill(routeGrid.visits.begin(), routeGrid.visits.end(), 0);
          const auto route = solver->Plan(routeGrid, here, goal, heading);
          state.goal = g.Key(goal);
          if (route.cells.empty()) {
            if (mission.TravellingKnownMap()) {
              if (mission.phase == MissionPhase::Repeat && !state.fastestSolve &&
                  StartLearnedTrace(state, position))
                return;
              mission.Fail("No safe learned route to destination; Reset to explore again");
              Send(state, 0, 0, mission.failure);
              return;
            }
            RememberFrontier(g, state.memory, *state.goal);
            Send(
              state, 0, 0, "Selected algorithm found no manoeuvrable route (or detected a loop)");
            return;
          }
          // DFS and wall following may choose longer routes than the shortest
          // reachability estimate. Budget the actual chosen route's travel.
          if (mission.timed && mission.phase == MissionPhase::Explore)
            mission.targetTravelSeconds =
              RouteTravelSeconds(route, heading, g.cell, cfg.speed, cfg.turnRate);
          InstallPath(state, route, position);
        }
      }

      if (state.next == state.path.size()) {
        if (state.departure) {
          state.departure = false;
          state.path.clear();
          state.mode = Mode::Settle;
          state.settleTime = time;
          state.gridAge = 1;
          Send(state, 0, 0, "Corner clearance reached; replanning to the same destination");
          return;
        }
        if (mission.phase == MissionPhase::Navigate) {
          const double tolerance = g.cell * 1.5;
          if (!mission.destination || std::hypot(position.x - mission.destination->x,
                                        position.y - mission.destination->y) > tolerance) {
            FailNavigation(
              state, "Could not reach the selected cell. Retry or cancel this destination.");
          } else {
            mission.phase = MissionPhase::Arrived;
            mission.current.success = true;
            mission.selectionMessage =
              "Destination reached within map-cell tolerance. Choose another point.";
            state.path.clear();
            Send(state, 0, 0, mission.selectionMessage);
          }
          return;
        }

        if (mission.phase == MissionPhase::Repeat) {
          state.mode = Mode::ConfirmGoal;
          state.elapsed = state.blocked = 0;
          state.reverseX = position.x;
          state.reverseY = position.y;
          Send(state, 0, 0, "At learned goal; requiring fresh arrival confirmation");
          return;
        }

        if (mission.phase == MissionPhase::ReturnToStart) {
          mission.Fail("Unable to reach start tolerance on learned route");
          Send(state, 0, 0, mission.failure);
          return;
        }

        if (state.goal) {
          RememberFrontier(g, state.memory, *state.goal);
          if (state.coveringFloor) ++state.memory.visits[*state.goal];
        }

        state.path.clear();
        if (state.coveringFloor) {
          state.mode = Mode::Plan;
          state.wait = 0;
          Send(state, 0, 0, "Search pass completed; selecting unsearched floor");
          return;
        }

        state.mode = Mode::Survey;
        state.scanned = state.elapsed = state.blocked = 0;
        state.headingReady = false;
        Send(state, 0, 0, "Frontier reached; surveying once");
        return;
      }

      if (!state.followingTrail && state.next < state.pathReverse.size())
        state.reverseTravel = state.pathReverse[state.next];
      const auto target = state.path[state.next];
      const double distance = std::hypot(target.x - position.x, target.y - position.y);
      // The recorded corner can be finer than a map cell. Skipping it at a
      // grid-sized tolerance cuts the corner on every subsequent segment.
      const double waypointTolerance = state.followingTrail && micromouse
                                         ? std::min(.002, traceSpacing * .25)
                                         : std::min(.025, g.cell * .25);
      if (distance <= waypointTolerance) {
        ++state.next;
        state.best = 1e9;
        state.stalled = 0;
        state.turning = false;
        state.trailDirectionReady = false;
        Send(state, 0, 0, "Waypoint reached");
        return;
      }

      if (state.followingTrail && !state.trailDirectionReady) {
        // Exploration may itself have backed away from an obstacle. Retracing
        // such a segment means driving forward, not spinning the chassis 180°.
        const double bearing = std::atan2(target.y - position.y, target.x - position.x);
        state.reverseTravel = std::abs(std::remainder(bearing - heading, 2 * Pi)) > Pi / 2;
        state.trailDirectionReady = true;
      }

      const double error = std::remainder(std::atan2(target.y - position.y, target.x - position.x) +
                                            (state.reverseTravel ? Pi : 0) - heading,
        2 * Pi);
      // A learned straight leg can correct small tracking errors while moving.
      // Significant turns still stop and use the full turning-space check.
      const bool continuous = mission.TravellingKnownMap();
      if (!state.turning && std::abs(error) > (continuous ? .10 : .01)) {
        state.turning = true;
        state.best = 1e9;
        state.stalled = 0;
      }

      if (state.turning && std::abs(error) < .003) {
        state.turning = false;
        state.mode = Mode::Settle;
        state.settleTime = time;
        state.best = 1e9;
        state.stalled = 0;
        Send(state, 0, 0, "Aligned; waiting for fresh readings");
        return;
      }

      const double metric = state.turning ? std::abs(error) : distance;
      if (metric < state.best - .005) {
        state.best = metric;
        state.stalled = 0;
      } else
        state.stalled += delta;

      const int end = g.At(target);
      // A measured trace may cross distant sonar evidence that needs a closer
      // observation. Revalidate the next stopping envelope, not the entire long
      // trace segment at once. Live sensor/Drive checks remain authoritative.
      const double checkedDistance =
        state.followingTrail
          ? std::min(distance,
              std::max(.20, settings->stopDistance + safety->halfLength + safety->clearance))
          : distance;
      bool clear = state.followingTrail ? g.Open(here) : g.Open(end), probe = false;
      const int checks = std::max(1, int(std::ceil(checkedDistance / (g.cell * .4))));
      for (int i = 0; i <= checks && clear; ++i) {
        const double t = double(i) / checks * checkedDistance / distance;
        const MazePoint point{
          std::lerp(position.x, target.x, t), std::lerp(position.y, target.y, t)};
        const int index = g.At(point), raw = map->Index(point.x, point.y);
        clear = g.Open(index);
        if (clear && (mission.phase == MissionPhase::Navigate || state.fastestSolve)) {
          const unsigned axis =
            std::abs(target.x - position.x) >= std::abs(target.y - position.y) ? 1u : 2u;
          clear = (g.clear[index] & axis) != 0;
        }
        if (clear && raw >= 0 && map->cells[raw] == 0 &&
            !state.memory.covered.contains(g.Key(index)))
          probe = true;
      }

      if (state.departure) {
        clear = KnownStraightClear(g,
          position,
          target,
          heading,
          std::max(double(collider->size.x / 2), safety->halfLength) + safety->clearance,
          std::max(double(collider->size.y / 2), safety->halfWidth) + safety->clearance,
          state.memory.covered);
        probe = false;
      }
      if (clear && state.turning) {
        clear = KnownTurnClear(g,
          position,
          heading,
          error,
          std::max(double(collider->size.x / 2), safety->halfLength) + safety->clearance,
          std::max(double(collider->size.y / 2), safety->halfWidth) + safety->clearance,
          state.memory.covered,
          mission.phase == MissionPhase::Explore && std::abs(error) <= .10);
      }
      if (!clear || state.blocked >= .6 || state.stalled >= 3) {
        state.departure = false;
        if (state.followingTrail && clear && state.blocked >= .6 &&
            distance <= (micromouse ? std::min(.06, double(collider->size.y) * .2) : .06)) {
          // Preserve corners precisely whenever possible. If live stopping
          // clearance refuses only the final <=6 cm correction, try the next
          // recorded segment instead; it gets its own map and safety checks.
          if (state.next + 1 < state.path.size()) {
            // Rejoin the next recorded segment nearby. Aiming directly at its
            // far endpoint would carry this corner's lateral error through an
            // entire corridor, outside the footprint that was actually learned.
            const auto next = state.path[state.next + 1];
            const double length = std::hypot(next.x - target.x, next.y - target.y);
            if (length <= .20) {
              ++state.next;
            } else {
              const double fraction = .20 / length;
              state.path[state.next] = {
                std::lerp(target.x, next.x, fraction), std::lerp(target.y, next.y, fraction)};
            }
          } else {
            ++state.next;
          }
          state.best = 1e9;
          state.stalled = state.blocked = 0;
          state.turning = state.trailDirectionReady = false;
          Send(state, 0, 0, "Small trace correction refused; checking next learned segment");
          return;
        }

        if (mission.phase == MissionPhase::Repeat && !state.fastestSolve && !state.followingTrail) {
          if (StartLearnedTrace(state, position)) return;
        }

        if (state.followingTrail) {
          if (mission.phase == MissionPhase::ReturnToStart) {
            // A first-run forward manoeuvre can be refused in reverse by the
            // rear stopping envelope. Replan through observed free space once.
            state.recoverySign = state.reverseTravel ? 1 : -1;
            state.followingTrail = state.reverseTravel = false;
            state.path.clear();
            state.mode = Mode::Reverse;
            state.elapsed = state.blocked = 0;
            state.reverseX = position.x;
            state.reverseY = position.y;
            state.wait = 0;
            state.gridAge = 1;
            Send(state,
              0,
              0,
              std::string("Recorded return refused: ") +
                (!clear                                     ? "map changed"
                  : previous == RoverSafetyStatus::Obstacle ? "live obstacle"
                                                            : "no progress") +
                "; retreat and replan");
            return;
          }

          const std::string cause = !clear                ? "map evidence changed"
                                    : state.blocked >= .6 ? "live safety refused motion"
                                                          : "steering made no progress";
          mission.Fail("Recorded learned path blocked: " + cause);
          Send(state, 0, 0, mission.failure);
          return;
        }

        if (mission.TravellingKnownMap() && ++state.routeFailures >= 3) {
          if (mission.timed) {
            FailNavigation(state,
              "Route blocked after three safe recovery attempts: " +
                (!clear ? std::string("map clearance changed")
                        : drive.StopReason(rover.GetEntity())) +
                ". Destination not reached. Retry or cancel the route.");
            return;
          }
          mission.Fail("Learned route blocked after three recovery attempts");
          Send(state, 0, 0, mission.failure);
          return;
        }

        const int direction =
          (int(std::llround(std::atan2(target.y - position.y, target.x - position.x) / (Pi / 2))) %
              4 +
            4) %
          4;
        // A refused manoeuvre affects a small physical neighbourhood, not
        // only one raster edge. Otherwise a few millimetres of recovery drift
        // lets the planner repeat the same unsafe approach from the next cell.
        const int radius = int(std::ceil(.15 / g.cell));
        for (int dy = -radius; dy <= radius; ++dy) {
          for (int dx = -radius; dx <= radius; ++dx) {
            if (std::hypot(dx, dy) * g.cell > .15) continue;
            const int col = here % g.width + dx, row = here / g.width + dy;
            if (col < 0 || row < 0 || col >= g.width || row >= g.height) continue;
            const int from = row * g.width + col;
            const int adjacent = g.Adjacent(from, direction);
            if (adjacent >= 0)
              state.memory.blockedEdges[{g.Key(from), g.Key(adjacent)}] = state.memory.time + 30;
          }
        }

        if (state.goal) RememberFrontier(g, state.memory, *state.goal);
        const std::string reason = !clear ? "Map changed along route"
                                   : state.blocked >= .6
                                     ? "Drive refused route: " + drive.StopReason(rover.GetEntity())
                                     : "Route made no progress";

        state.path.clear();
        state.mode = Mode::Reverse;
        state.recoverySign = state.reverseTravel ? 1 : -1;
        state.elapsed = state.blocked = 0;
        state.reverseX = position.x;
        state.reverseY = position.y;
        Send(state, 0, 0, reason + "; target remembered before bounded recovery");
        return;
      }

      if (state.turning)
        Send(state,
          0,
          std::clamp(4 * error, -cfg.turnRate, cfg.turnRate),
          "Following planned heading");
      else {
        Send(state,
          (state.reverseTravel ? -1 : 1) *
            std::min(probe ? std::min(.03, cfg.speed) : cfg.speed, distance / delta),
          continuous ? std::clamp(4 * error, -.3, .3) : 0,
          probe                          ? "Probing local unknown cells; live Drive safety active"
          : mission.TravellingKnownMap() ? "Following learned route"
          : state.coveringFloor          ? "Searching known free floor for arrival marker"
                                         : "Following route to untried frontier");
      }
    });
  }

private:
  static constexpr double Pi = std::numbers::pi;

  static void ResetRoute(State& state) {
    state.path.clear();
    state.pathReverse.clear();
    state.goal.reset();
    state.next = 0;
    state.mode = Mode::Plan;
    state.followingTrail = state.reverseTravel = state.turning = state.departure = false;
    state.trailDirectionReady = false;
    state.wait = state.stalled = state.blocked = state.elapsed = 0;
    state.best = 1e9;
    state.gridAge = 1;
    state.routeFailures = state.routeResurveys = 0;
  }

  bool StartDeparture(State& state,
    MazePoint position,
    double heading,
    int goal,
    double halfLength,
    double halfWidth) {
    const auto& g = state.grid;
    double best = std::numeric_limits<double>::infinity();
    MazePoint target;
    bool reverse = false;
    // Never blindly back up: choose a short known-clear translation only if it
    // leads to a valid route to this destination. Drive still checks live rear safety.
    for (int sign : {-1, 1}) {
      for (int step = 1; step <= 12; ++step) {
        const double distance = step * .05;
        const MazePoint candidate{position.x + sign * distance * std::cos(heading),
          position.y + sign * distance * std::sin(heading)};
        if (!KnownStraightClear(
              g, position, candidate, heading, halfLength, halfWidth, state.memory.covered))
          break;
        const auto route = FastestKnownRoute(
          g, g.At(candidate), goal, heading, state.config.speed, state.config.turnRate);
        if (route.cells.empty()) continue;
        const double seconds = distance / state.config.speed + route.seconds;
        if (seconds < best) {
          best = seconds;
          target = candidate;
          reverse = sign < 0;
        }
      }
    }
    if (!std::isfinite(best)) return false;
    state.path = {target};
    state.pathReverse = {reverse};
    state.next = 0;
    state.departure = true;
    state.reverseTravel = reverse;
    state.mode = Mode::Travel;
    state.best = 1e9;
    state.stalled = state.blocked = 0;
    state.turning = false;
    state.mission.selectionMessage =
      "Moving to known turning space before following the destination route";
    Send(state, 0, 0, state.mission.selectionMessage);
    return true;
  }

  void FailNavigation(State& state, std::string reason) {
    ResetRoute(state);
    state.mission.phase =
      state.fastestSolve ? MissionPhase::Failed : MissionPhase::NavigationBlocked;
    state.mission.current.success = false;
    state.mission.failure = reason;
    state.mission.selectionMessage = reason;
    Send(state, 0, 0, std::move(reason));
  }

  void EndExploration(State& state, std::string outcome, std::string message) {
    state.mission.explorationOutcome = std::move(outcome);
    WaitForDestination(state, std::move(message));
  }

  void WaitForDestination(State& state, std::string message) {
    ResetRoute(state);
    state.mission.phase = MissionPhase::AwaitDestination;
    state.mission.selectionMessage = message;
    Send(state, 0, 0, std::move(message));
  }

  // Regular-cell navigation uses measured boundary walls, not hidden tilemap data.
  // Translation joins cell centres; all direction changes happen in the cell interior.
  void FollowCellMaze(State& state,
    const MicromouseMazeComponent& config,
    MazePoint position,
    double heading,
    const RoverSafetyComponent& safety,
    const pipeframe::EnvironmentCollider2DComponent& collider,
    double delta) {
    auto& maze = state.cellMaze;
    auto& mission = state.mission;
    const auto& cfg = state.config;
    const double radius = std::hypot(std::max(double(collider.size.x / 2), safety.halfLength),
                            std::max(double(collider.size.y / 2), safety.halfWidth)) +
                          safety.clearance;
    if (!config.Valid() || radius + .005 >= (config.cellPitch - config.wallThickness) / 2) {
      Send(state,
        0,
        0,
        "Cell maze dimensions must allow the complete chassis to turn at cell centres");
      return;
    }
    if (!state.cellReady) {
      maze.Reset(config);
      state.cellReady = true;
    } else if (maze.settings != config) {
      mission.Fail("Cell geometry changed during a run; Reset to relearn walls");
      Send(state, 0, 0, mission.failure);
      return;
    }
    const int here = maze.At(position);
    if (here < 0) {
      mission.Fail("Odometry left the configured regular cell maze");
      Send(state, 0, 0, mission.failure);
      return;
    }
    if (state.cellPhase != int(mission.phase)) {
      state.cellPhase = int(mission.phase);
      state.cellMoving = false;
      state.path.clear();
      state.pathHeadings.clear();
      state.next = 0;
    }
    const auto beginMove = [&](MazePoint target, double bearing) {
      state.cellTarget = target;
      state.cellBearing = bearing;
      state.cellMoving = true;
      state.mode = Mode::Travel;
      state.cellAligned = false;
      state.cellStarted = time;
      state.cellTimeLimit = std::max(30.0,
        2 * std::hypot(target.x - position.x, target.y - position.y) / cfg.speed +
          2 * Pi / cfg.turnRate + 5);
      state.blocked = 0;
    };
    const auto centre = maze.Centre(here);
    const double offCentre = std::hypot(position.x - centre.x, position.y - centre.y);

    if (!state.cellMoving && offCentre > .0015) {
      // Arrival sensing can stop before the goal cell centre. Finish that same
      // straight approach before turning for return; never pivot beside a wall.
      const double bearing = std::atan2(centre.y - position.y, centre.x - position.x);
      const double facing =
        std::abs(std::remainder(bearing - heading, 2 * Pi)) < Pi / 2 ? bearing : bearing + Pi;
      if (std::abs(std::remainder(facing - heading, 2 * Pi)) > .12) {
        mission.Fail("Mouse is off the cell-centre track; reset at a cell centre");
        Send(state, 0, 0, mission.failure);
        return;
      }
      beginMove(centre, facing);
    }
    if (state.cellMoving) {
      const double distance =
        std::hypot(state.cellTarget.x - position.x, state.cellTarget.y - position.y);
      if (distance <= .0015) {
        state.cellMoving = false;
        state.cellAligned = false;
        state.settleTime = time;
        if (state.next < state.path.size()) ++state.next;
        Send(state, 0, 0, "Cell centre reached; refreshing wall observations");
        return;
      }
      if (state.blocked >= .6 || time - state.cellStarted > state.cellTimeLimit) {
        mission.Fail(
          "Cell motion blocked by live safety; check the wall readings and regular maze geometry");
        Send(state, 0, 0, mission.failure);
        return;
      }
      const double turnError = std::remainder(state.cellBearing - heading, 2 * Pi);
      if (!state.cellAligned) {
        if (std::abs(turnError) > .003) {
          // A turn may start only inside the authored wall-free cell interior.
          if (std::abs(turnError) > .12 &&
              offCentre + radius >= (config.cellPitch - config.wallThickness) / 2) {
            mission.Fail("Insufficient cell-interior turning room");
            Send(state, 0, 0, mission.failure);
            return;
          }
          Send(state,
            0,
            std::clamp(4 * turnError, -cfg.turnRate, cfg.turnRate),
            "Turning at cell centre; live clearance checked");
          return;
        }
        state.cellAligned = true;
        state.settleTime = time;
        Send(state, 0, 0, "Aligned; refreshing directional readings");
        return;
      }
      if (time - state.settleTime < .12) {
        Send(state, 0, 0, "Waiting for fresh wall readings");
        return;
      }
      const double bearing =
        std::atan2(state.cellTarget.y - position.y, state.cellTarget.x - position.x);
      const bool reverse = std::abs(std::remainder(bearing - state.cellBearing, 2 * Pi)) > Pi / 2;
      const double error = std::remainder(bearing + (reverse ? Pi : 0) - heading, 2 * Pi);
      Send(state,
        (reverse ? -1 : 1) * std::min(cfg.speed, 2 * distance),
        std::clamp(4 * error, -.3, .3),
        state.fastestSolve ? "Following fastest learned cell route"
                           : "Following measured open cell edge");
      return;
    }
    if (time - state.settleTime < .12) {
      Send(state, 0, 0, "Waiting for wall samples at cell centre");
      return;
    }
    if (mission.phase == MissionPhase::ReturnToStart && here == 0) {
      const double error = std::remainder(-heading, 2 * Pi);
      if (std::abs(error) > .003) {
        Send(state,
          0,
          std::clamp(4 * error, -cfg.turnRate, cfg.turnRate),
          "Aligning original start heading");
        return;
      }
      mission.phase = MissionPhase::ReadyToSolve;
      mission.repeatStartOffset = std::hypot(position.x, position.y);
      state.path.clear();
      Send(state, 0, 0, "Returned and stopped. Press Solve Maze for a new learned-map route");
      if (mission.solveAfterReturn) SolveMaze(state.rover.GetEntity());
      return;
    }
    const int facing = (int(std::llround(heading / (Pi / 2))) % 4 + 4) % 4;
    if (std::abs(std::remainder(heading - facing * Pi / 2, 2 * Pi)) > .02) {
      Send(state,
        0,
        std::clamp(
          4 * std::remainder(facing * Pi / 2 - heading, 2 * Pi), -cfg.turnRate, cfg.turnRate),
        "Aligning cell wall observations");
      return;
    }
    maze.visited[here] = true;
    unsigned observed = 0;
    ranges.Each([&](const RangeObservation& sample) {
      if (sample.settings.parent.objectId != cfg.rover.objectId || !sample.valid ||
          !sample.present || sample.capturedAt < state.settleTime || sample.age > .3)
        return;
      const double angle = heading + sample.settings.localAngleDegrees * Pi / 180;
      const int d = (int(std::llround(angle / (Pi / 2))) % 4 + 4) % 4;
      if (std::abs(std::remainder(angle - d * Pi / 2, 2 * Pi)) > .03) return;
      const double c = std::cos(heading), sn = std::sin(heading);
      const double mountX =
        c * sample.settings.localPosition.x - sn * sample.settings.localPosition.y;
      const double mountY =
        sn * sample.settings.localPosition.x + c * sample.settings.localPosition.y;
      const double distance = sample.distance +
                              (mountX + position.x - centre.x) * std::cos(d * Pi / 2) +
                              (mountY + position.y - centre.y) * std::sin(d * Pi / 2);
      const auto edge = maze.Classify(distance,
        sample.status == SensorReadingStatus::Hit ||
          sample.status == SensorReadingStatus::BelowMinimumRange);
      if (edge != CellMaze::Unknown) {
        maze.Observe(here, d, edge);
        observed |= 1u << d;
      }
    });
    if (observed != 15) {
      Send(state, 0, 0, "Waiting for unambiguous front/side/rear cell walls");
      return;
    }
    state.grid = maze.Grid();
    int goal = -1;
    if (mission.phase == MissionPhase::Explore) {
      goal = maze.NextUnvisited(here);
      // Backtracking can also cross already observed straight cells in one
      // leg. Stop at the first unvisited cell or bend to take new wall samples.
      const int step = goal - here;
      for (int count = 0; goal >= 0 && maze.visited[goal] && count < int(maze.edges.size());
        ++count) {
        const int next = maze.NextUnvisited(goal);
        if (next < 0 || next - goal != step) break;
        goal = next;
      }
    } else if (mission.phase == MissionPhase::ReturnToStart)
      goal = 0;
    else if (mission.learnedGoal)
      goal = maze.At(*mission.learnedGoal);
    if (goal < 0) {
      mission.Fail("No reachable unvisited cells remain; the finish has not been detected");
      Send(state, 0, 0, mission.failure);
      return;
    }
    if (goal == here) {
      mission.Fail("Learned goal cell reached but the floor detector did not confirm the marker");
      Send(state, 0, 0, mission.failure);
      return;
    }
    const auto route = FastestKnownRoute(state.grid, here, goal, heading, cfg.speed, cfg.turnRate);
    if (route.cells.size() < 2) {
      mission.Fail("No route through observed open cell edges");
      Send(state, 0, 0, mission.failure);
      return;
    }
    mission.plannedSeconds = route.seconds;
    mission.plannedDistance = (route.cells.size() - 1) * config.cellPitch;
    state.path.clear();
    state.pathHeadings.clear();
    state.next = 0;
    for (std::size_t i = 1; i < route.cells.size(); ++i) {
      state.path.push_back(maze.Centre(route.cells[i]));
      state.pathHeadings.push_back(route.headings[i] * Pi / 2);
    }
    state.goal = state.grid.Key(goal);
    // Keep every cell in the displayed route, but brake only at its next
    // corner/reversal or endpoint. The planner admits observed open edges only.
    // Exploration still stops at the next unvisited cell to learn its walls.
    std::size_t straightEnd = 1;
    while (straightEnd + 1 < route.cells.size() &&
           route.cells[straightEnd] - route.cells[straightEnd - 1] ==
             route.cells[straightEnd + 1] - route.cells[straightEnd] &&
           route.headings[straightEnd] == route.headings[straightEnd + 1])
      ++straightEnd;
    state.next = straightEnd - 1;
    beginMove(state.path[state.next], state.pathHeadings[state.next]);
    Send(state,
      0,
      0,
      mission.phase == MissionPhase::Explore
        ? "Flood fill: next unvisited cell through observed walls"
        : "Time-cost route rebuilt from learned cell walls");
  }

  static void InstallPath(State& state, const TimedRoute& result, MazePoint position) {
    state.strandedRecoveries = 0;
    const auto& route = result.cells;
    const auto& grid = state.grid;
    const auto root = grid.Point(route.front());
    const double offsetX = position.x - root.x, offsetY = position.y - root.y;
    state.path.clear();
    state.pathReverse.clear();
    for (std::size_t i = 1; i < route.size(); ++i) {
      if (i + 1 == route.size() || route[i] - route[i - 1] != route[i + 1] - route[i] ||
          result.headings[i] != result.headings[i + 1]) {
        const auto point = grid.Point(route[i]);
        state.path.push_back({point.x + offsetX, point.y + offsetY});
        const int direction = route[i] - route[i - 1] == 1            ? 0
                              : route[i] - route[i - 1] == grid.width ? 1
                              : route[i] - route[i - 1] == -1         ? 2
                                                                      : 3;
        state.pathReverse.push_back(result.headings[i] != direction);
      }
    }
    state.goal = grid.Key(route.back());
    state.next = 0;
    state.mode = Mode::Travel;
    state.best = 1e9;
    state.stalled = state.blocked = 0;
    state.turning = false;
    state.measuredTurnLimit = state.config.turnRate;
  }

  bool StartLearnedTrace(State& state, MazePoint position) {
    if (state.followingTrail || state.mission.trail.size() < 2) return false;

    const auto& trail = state.mission.trail;
    std::size_t next = 1;
    double nearest = 1e9;
    MazePoint join;
    for (std::size_t i = 1; i < trail.size(); ++i) {
      const auto a = trail[i - 1], b = trail[i];
      const double dx = b.x - a.x, dy = b.y - a.y, lengthSquared = dx * dx + dy * dy;
      const double t =
        lengthSquared > 1e-8
          ? std::clamp(
              ((position.x - a.x) * dx + (position.y - a.y) * dy) / lengthSquared, 0.0, 1.0)
          : 0;
      const MazePoint point{a.x + t * dx, a.y + t * dy};
      const double distance = std::hypot(point.x - position.x, point.y - position.y);
      if (distance < nearest) {
        nearest = distance;
        next = i;
        join = point;
      }
    }
    if (nearest > .25) return false;

    state.path = {join};
    state.pathReverse.clear();
    state.path.insert(state.path.end(), trail.begin() + next, trail.end());
    state.next = 0;
    state.mode = Mode::Travel;
    state.followingTrail = true;
    state.reverseTravel = state.trailDirectionReady = state.turning = false;
    state.best = 1e9;
    state.stalled = state.blocked = 0;
    Send(state, 0, 0, "Grid route unavailable; following the recorded successful path");
    return true;
  }

  void InstallMeasuredRoute(State& state, bool returning) {
    state.path.clear();
    state.pathHeadings.clear();
    const auto append = [&](const MeasuredPose& pose) {
      state.path.push_back(pose.position);
      state.pathHeadings.push_back(pose.heading);
    };
    if (returning) {
      for (auto it = state.measuredRoute.poses.rbegin(); it != state.measuredRoute.poses.rend();
        ++it)
        append(*it);
    } else {
      for (const auto& pose : state.measuredRoute.poses)
        append(pose);
    }
    state.followingMeasuredRoute = true;
    state.measuredPositionAccepted = false;
    state.measuredTurnLimit = state.config.turnRate;
    state.followingTrail = false;
    state.mode = Mode::Travel;
    state.next = 0;
    state.best = 1e9;
    state.blocked = state.stalled = 0;
    state.turning = false;
  }

  void FollowMeasuredRoute(State& state, MazePoint position, double heading, double delta) {
    if (state.next >= state.path.size()) {
      if (state.mission.phase == MissionPhase::Repeat) {
        state.followingMeasuredRoute = false;
        state.mode = Mode::ConfirmGoal;
        state.elapsed = state.blocked = 0;
        state.reverseX = position.x;
        state.reverseY = position.y;
        Send(state, 0, 0, "At measured goal; requiring fresh arrival confirmation");
        return;
      }
      state.mission.Fail("Measured route ended without arrival confirmation");
      Send(state, 0, 0, state.mission.failure);
      return;
    }

    const auto target = state.path[state.next];
    const double distance = std::hypot(target.x - position.x, target.y - position.y);
    const auto* hardware = state.rover.GetComponent<MicromouseHardwareComponent>();
    const double endpointTolerance = hardware ? std::min(.005, hardware->width * .05) : .001;
    // Finite sensor/contact precision can refuse the final small
    // translation. Accept that measured position, then independently check its
    // recorded turn and next leg. Bound this to 5% of chassis width, capped at
    // 5 mm; a distant obstruction still fails rather than skipping the route.
    if (state.blocked > 0 && !state.turning && distance <= endpointTolerance) {
      state.measuredPositionAccepted = true;
      state.stalled = state.blocked = 0;
      state.best = 1e9;
    }
    double error = MeasuredRoute::Angle(state.pathHeadings[state.next] - heading);
    double sign = 1;
    const bool atPosition = distance <= .001 || state.measuredPositionAccepted;
    if (!atPosition) {
      const double bearing = std::atan2(target.y - position.y, target.x - position.x);
      sign = std::abs(MeasuredRoute::Angle(bearing - heading)) > Pi / 2 ? -1 : 1;
      error = MeasuredRoute::Angle(bearing + (sign < 0 ? Pi : 0) - heading);
    } else if (std::abs(error) <= .003) {
      ++state.next;
      state.measuredPositionAccepted = false;
      state.best = 1e9;
      state.stalled = state.blocked = 0;
      Send(state, 0, 0, "Measured pose reached; preserving corner heading");
      return;
    }

    // Follow small heading errors with a gentle arc. Stopping to converge to
    // a microscopic bearing error at every recorded sample is needlessly slow
    // and creates artificial pivots that were not in the original motion.
    const bool turning = atPosition || std::abs(error) > .10;
    const double metric = turning ? std::abs(error) : distance;
    if (turning != state.turning || metric < state.best - .0001) {
      state.best = metric;
      state.stalled = 0;
    } else {
      state.stalled += delta;
    }
    state.turning = turning;
    if (std::abs(error) > .003) {
      // The safety envelope reserves a future angular sweep. Near a wall a
      // slow, sensor-approved correction may fit while the full-rate command
      // does not. Reduce the request, never the footprint or safety margin.
      if (state.blocked > 0)
        state.measuredTurnLimit = std::max(.002, state.measuredTurnLimit * .5);
      else
        state.measuredTurnLimit = std::min(state.config.turnRate, state.measuredTurnLimit * 1.05);
    }
    if (state.blocked >= .6 || state.stalled >= 5) {
      state.mission.Fail("Measured learned route blocked: " +
                         (state.blocked >= .6 ? drive.StopReason(state.rover.GetEntity())
                                              : std::string("no pose progress")) +
                         (turning ? " (turn)" : " (translation)") + " at waypoint " +
                         std::to_string(state.next) + "; pose error " + std::to_string(distance) +
                         " m / " + std::to_string(error) + " rad");
      Send(state, 0, 0, state.mission.failure);
      return;
    }

    // This is a route actually traversed by this chassis, including its heading,
    // rather than a new shortcut inferred from uncertain sonar cells. It still
    // passes through Drive's fresh front/rear/turn envelopes and real collision.
    // A changed obstacle refuses motion and fails the leg; no pose is teleported.
    if (turning) {
      Send(state,
        0,
        std::clamp(4 * error, -state.measuredTurnLimit, state.measuredTurnLimit),
        "Following measured corner heading; live safety active");
    } else {
      const double acceleration = hardware && hardware->Valid() ? hardware->Acceleration() : 1;
      const double speed =
        std::min({state.config.speed, std::sqrt(2 * acceleration * distance), distance / delta});
      const double correction =
        std::abs(error) <= .003
          ? 0
          : std::clamp(4 * error, -state.measuredTurnLimit, state.measuredTurnLimit);
      Send(state, sign * speed, correction, "Following measured learned route; live safety active");
    }
  }

  void Send(State& state, double speed, double turn, std::string message) {
    RoverMotionCommandEvent event;
    event.simulationTime = time;
    event.roverId = state.config.rover.objectId;
    event.forwardSpeed = speed;
    event.turnRate = turn;
    state.commanded = speed != 0 || turn != 0;
    state.status = std::move(message);
    events.Publish(event);
  }

  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  RoverMappingSystem& mapping;
  RoverDriveSystem& drive;
  RoverRangeObservations ranges;
  Resolver resolve;
  double time{};
  std::map<pipeframe::ecs::Entity, State> states;
  std::map<pipeframe::ecs::Entity, RoverGoalEvent> arrivals;
  pipeframe::EventSubscription goalConnection;
};

}  // namespace RoverLab

#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>
#include <string>

struct RoverSolverComponent : pipeframe::Component<RoverSolverComponent> {
  pipeframe::SceneObjectReference rover{};
  std::string algorithm{"AStar"};
  std::string missionMode{"TimedExplore"};
  std::string explorationDuration{"Until complete"};
  double speed{0.10};
  double turnRate{1.0};
  double noNewCellTimeout{120.0};
  std::string goalTag{"Finish"};
  double runTimeout{600};
  double searchSpacing{.30};
  bool enabled{true};

  bool operator==(const RoverSolverComponent&) const = default;

  bool Valid() const {
    return (missionMode == "TimedExplore" || missionMode == "ArrivalMarker") &&
           (explorationDuration == "5 minutes" || explorationDuration == "10 minutes" ||
             explorationDuration == "Until complete") &&
           std::isfinite(searchSpacing) && searchSpacing >= .05 && searchSpacing <= .5 &&
           !goalTag.empty() && goalTag.size() <= 64 && std::isfinite(runTimeout) &&
           runTimeout >= 10 && runTimeout <= 3600 && std::isfinite(noNewCellTimeout) &&
           noNewCellTimeout >= 5 && noNewCellTimeout <= 600 && std::isfinite(speed) && speed > 0 &&
           std::isfinite(turnRate) && turnRate > 0 &&
           (algorithm == "BFS" || algorithm == "DFS" || algorithm == "Dijkstra" ||
             algorithm == "AStar" || algorithm == "FloodFill" || algorithm == "RightHand");
  }

  static auto BuildSchema() {
    using namespace pipeframe;

    return Define("project.RoverSolverComponent", "Rover Solver")
      .OnlyOn("project.RoverEntity")
      .OwnerReference("rover", &RoverSolverComponent::rover)
      .Editable({.key = "missionMode",
                  .displayName = "Mission Mode",
                  .kind = PropertyKind::Enum,
                  .defaultValue = std::string{"TimedExplore"},
                  .enumOptions = {"TimedExplore", "ArrivalMarker"}},
        &RoverSolverComponent::missionMode)
      .Editable({.key = "explorationDuration",
                  .displayName = "Exploration Duration (simulation time)",
                  .kind = PropertyKind::Enum,
                  .defaultValue = std::string{"Until complete"},
                  .enumOptions = {"Until complete", "5 minutes", "10 minutes"}},
        &RoverSolverComponent::explorationDuration)
      .Editable({.key = "algorithm",
                  .displayName = "Route Algorithm",
                  .kind = PropertyKind::Enum,
                  .defaultValue = std::string{"AStar"},
                  .enumOptions = {"BFS", "DFS", "Dijkstra", "AStar", "FloodFill", "RightHand"}},
        &RoverSolverComponent::algorithm)
      .Editable({.key = "speed",
                  .displayName = "Cruise Speed",
                  .kind = PropertyKind::Number,
                  .defaultValue = .10,
                  .unit = "metres / second",
                  .minimum = .001},
        &RoverSolverComponent::speed)
      .Editable({.key = "turnRate",
                  .displayName = "Turn Rate",
                  .kind = PropertyKind::Number,
                  .defaultValue = 1.0,
                  .unit = "radians / second",
                  .minimum = .001},
        &RoverSolverComponent::turnRate)
      .Editable({.key = "noNewCellTimeout",
                  .displayName = "No New Cell Timeout",
                  .kind = PropertyKind::Number,
                  .defaultValue = 120.0,
                  .unit = "seconds",
                  .minimum = 5,
                  .maximum = 600},
        &RoverSolverComponent::noNewCellTimeout)
      .Editable({.key = "goalTag",
                  .displayName = "Goal Tag (arrival detection)",
                  .kind = PropertyKind::String,
                  .defaultValue = std::string{"Finish"}},
        &RoverSolverComponent::goalTag)
      .Editable({.key = "searchSpacing",
                  .displayName = "Floor Search Spacing",
                  .kind = PropertyKind::Number,
                  .defaultValue = .30,
                  .unit = "metres",
                  .minimum = .05,
                  .maximum = .5},
        &RoverSolverComponent::searchSpacing)
      .Editable({.key = "runTimeout",
                  .displayName = "Mission Time Limit",
                  .kind = PropertyKind::Number,
                  .defaultValue = 600.0,
                  .unit = "seconds per leg",
                  .minimum = 10,
                  .maximum = 3600},
        &RoverSolverComponent::runTimeout)
      .Editable({.key = "enabled",
                  .displayName = "Enabled",
                  .kind = PropertyKind::Boolean,
                  .defaultValue = true},
        &RoverSolverComponent::enabled)
      .Validate("Choose a supported algorithm and finite positive speeds",
        [](const auto& value) { return value.Valid(); });
  }
};

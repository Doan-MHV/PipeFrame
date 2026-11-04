#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>
#include <string>

// A simulated floor marker. It is not an ultrasonic/LiDAR return.
struct RoverGoalComponent : pipeframe::Component<RoverGoalComponent> {
  std::string tag{"Finish"};
  double radius{0.20};
  bool enabled{true};

  bool operator==(const RoverGoalComponent&) const = default;

  bool Valid() const {
    return !tag.empty() && tag.size() <= 64 && std::isfinite(radius) && radius >= .05 &&
           radius <= 1;
  }

  static auto BuildSchema() {
    using namespace pipeframe;

    return Define("project.RoverGoalComponent", "Goal Marker")
      .OnlyOn("project.RoverGoalEntity")
      .Editable("tag", "Goal Tag", &RoverGoalComponent::tag)
      .Editable("radius", "Arrival Radius", &RoverGoalComponent::radius, {.unit = "metres", .minimum = .05, .maximum = 1})
      .Editable("enabled", "Enabled", &RoverGoalComponent::enabled)
      .Validate("Use a goal tag and an arrival radius from 0.05 to 1 metre",
        [](const auto& value) { return value.Valid(); });
  }
};

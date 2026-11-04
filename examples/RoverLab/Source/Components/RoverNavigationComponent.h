#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>

struct RoverNavigationComponent : pipeframe::Component<RoverNavigationComponent> {
  pipeframe::SceneObjectReference rover{};

  double speed{0.10};
  double turnDistance{0.55};
  double passageDistance{0.60};
  double turnRate{1.0};

  bool enabled{true};

  bool Valid() const {
    return std::isfinite(speed) && speed > 0 && std::isfinite(turnDistance) && turnDistance > 0 &&
           std::isfinite(passageDistance) && passageDistance > turnDistance &&
           std::isfinite(turnRate) && turnRate > 0;
  }

  static auto BuildSchema() {
    using namespace pipeframe;

    return Define("project.RoverNavigationComponent", "Rover Navigation")
      .RetiredField("wallDistance")
      .RetiredField("openingDistance")
      .OnlyOn("project.RoverEntity")
      .OwnerReference("rover", &RoverNavigationComponent::rover)
      .Editable("speed", "Cruise Speed", &RoverNavigationComponent::speed, {.unit = "metres / second", .minimum = 0.001})
      .Editable("turnDistance", "Front Turn Distance", &RoverNavigationComponent::turnDistance, {.unit = "metres", .minimum = 0.001})
      .Editable("passageDistance", "Minimum Passage Reading", &RoverNavigationComponent::passageDistance, {.unit = "metres", .minimum = 0.001})
      .Editable("turnRate", "Turn Rate", &RoverNavigationComponent::turnRate, {.unit = "radians / second", .minimum = 0.001})
      .Editable("enabled", "Enabled", &RoverNavigationComponent::enabled)
      .Validate(
        "Use finite positive settings; "
        "passage distance must exceed turn distance",
        [](const auto& config) { return config.Valid(); });
  }
};
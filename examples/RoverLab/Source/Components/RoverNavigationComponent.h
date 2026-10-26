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
      .Editable(
        {
          .key = "speed",
          .displayName = "Cruise Speed",
          .kind = PropertyKind::Number,
          .defaultValue = 0.10,
          .unit = "metres / second",
          .minimum = 0.001,
        },
        &RoverNavigationComponent::speed)
      .Editable(
        {
          .key = "turnDistance",
          .displayName = "Front Turn Distance",
          .kind = PropertyKind::Number,
          .defaultValue = 0.55,
          .unit = "metres",
          .minimum = 0.001,
        },
        &RoverNavigationComponent::turnDistance)
      .Editable(
        {
          .key = "passageDistance",
          .displayName = "Minimum Passage Reading",
          .kind = PropertyKind::Number,
          .defaultValue = 0.60,
          .unit = "metres",
          .minimum = 0.001,
        },
        &RoverNavigationComponent::passageDistance)
      .Editable(
        {
          .key = "turnRate",
          .displayName = "Turn Rate",
          .kind = PropertyKind::Number,
          .defaultValue = 1.0,
          .unit = "radians / second",
          .minimum = 0.001,
        },
        &RoverNavigationComponent::turnRate)
      .Editable(
        {
          .key = "enabled",
          .displayName = "Enabled",
          .kind = PropertyKind::Boolean,
          .defaultValue = true,
        },
        &RoverNavigationComponent::enabled)
      .Validate(
        "Use finite positive settings; "
        "passage distance must exceed turn distance",
        [](const auto& config) { return config.Valid(); });
  }
};
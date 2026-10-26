#pragma once

#include <PipeFrame/Project/Component.h>

struct UltrasonicSensorComponent : pipeframe::Component<UltrasonicSensorComponent> {
  pipeframe::SceneObjectReference parent{};

  pipeframe::Vector2f localPosition{};
  double localAngleDegrees{0.0};

  double minimumRange{0.02};
  double maximumRange{2.0};
  double coneAngleDegrees{30.0};
  bool enabled{true};

  static auto BuildSchema() {
    using namespace pipeframe;
    using K = PropertyKind;

    return Define(
      "project.UltrasonicSensorComponent", "UltrasonicSensorComponent")
      .Editable({.key = "parent",
                  .displayName = "Parent Rover",
                  .kind = K::ObjectReference,
                  .defaultValue = SceneObjectReference{}},
        &UltrasonicSensorComponent::parent)

      .Editable({.key = "localPosition",
                  .displayName = "Mount Position",
                  .kind = K::Vector2,
                  .defaultValue = Vector2f{},
                  .unit = "metres"},
        &UltrasonicSensorComponent::localPosition)

      .Editable({.key = "localAngleDegrees",
                  .displayName = "Mount Angle",
                  .kind = K::Number,
                  .defaultValue = 0.0,
                  .unit = "degrees",
                  .minimum = -180.0,
                  .maximum = 180.0},
        &UltrasonicSensorComponent::localAngleDegrees)

      .Editable({.key = "minimumRange",
                  .displayName = "Minimum Range",
                  .kind = K::Number,
                  .defaultValue = 0.02,
                  .unit = "metres",
                  .minimum = 0.001,
                  .maximum = 10.0},
        &UltrasonicSensorComponent::minimumRange)

      .Editable({.key = "maximumRange",
                  .displayName = "Maximum Range",
                  .kind = K::Number,
                  .defaultValue = 2.0,
                  .unit = "metres",
                  .minimum = 0.01,
                  .maximum = 10.0},
        &UltrasonicSensorComponent::maximumRange)

      .Editable({.key = "coneAngleDegrees",
                  .displayName = "Detection Cone",
                  .kind = K::Number,
                  .defaultValue = 30.0,
                  .unit = "degrees",
                  .minimum = 1.0,
                  .maximum = 90.0},
        &UltrasonicSensorComponent::coneAngleDegrees)

      .Editable(
        {.key = "enabled", .displayName = "Enabled", .kind = K::Boolean, .defaultValue = true},
        &UltrasonicSensorComponent::enabled)

      // Scenes saved before event-based readings contain these transient fields.
      .RetiredField("distance")
      .RetiredField("sampleTime")
      .RetiredField("status")
      .Validate("Maximum range must exceed minimum range", [](const auto& sensor) {
        return std::isfinite(sensor.minimumRange) && std::isfinite(sensor.maximumRange) &&
               sensor.minimumRange > 0 && sensor.maximumRange > sensor.minimumRange;
      });
  }
};

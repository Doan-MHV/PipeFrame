#pragma once

#include <PipeFrame/Project/Component.h>

struct InfraredSensorComponent : pipeframe::Component<InfraredSensorComponent> {
  pipeframe::SceneObjectReference parent{};

  pipeframe::Vector2f localPosition{};
  double localAngleDegrees{0.0};

  double minimumRange{0.005};
  double maximumRange{0.5};
  double coneAngleDegrees{6.0};
  double samplePeriod{.02};
  double noiseAmplitude{};
  std::int64_t randomSeed{42};
  bool enabled{true};
  bool operator==(const InfraredSensorComponent&) const = default;

  bool Valid() const {
    return std::isfinite(localPosition.x) && std::isfinite(localPosition.y) &&
           std::isfinite(localAngleDegrees) && std::isfinite(minimumRange) && minimumRange > 0 &&
           std::isfinite(maximumRange) && maximumRange > minimumRange && maximumRange <= 2 &&
           std::isfinite(coneAngleDegrees) && coneAngleDegrees >= 1 && coneAngleDegrees <= 30 &&
           std::isfinite(samplePeriod) && samplePeriod >= .001 && samplePeriod <= 1 &&
           std::isfinite(noiseAmplitude) && noiseAmplitude >= 0 && noiseAmplitude <= .05 &&
           randomSeed >= 0 && randomSeed <= 2147483647;
  }

  static auto BuildSchema() {
    using namespace pipeframe;
    using K = PropertyKind;

    return Define("project.InfraredSensorComponent", "Infrared Wall Sensor")
      .OnlyOn("project.InfraredSensorEntity")
      .Editable({.key = "parent",
                  .displayName = "Parent Rover",
                  .kind = K::ObjectReference,
                  .defaultValue = SceneObjectReference{}},
        &InfraredSensorComponent::parent)

      .Editable({.key = "localPosition",
                  .displayName = "Mount Position",
                  .kind = K::Vector2,
                  .defaultValue = Vector2f{},
                  .unit = "metres"},
        &InfraredSensorComponent::localPosition)

      .Editable({.key = "localAngleDegrees",
                  .displayName = "Mount Angle",
                  .kind = K::Number,
                  .defaultValue = 0.0,
                  .unit = "degrees",
                  .minimum = -180.0,
                  .maximum = 180.0},
        &InfraredSensorComponent::localAngleDegrees)

      .Editable({.key = "minimumRange",
                  .displayName = "Minimum Range",
                  .kind = K::Number,
                  .defaultValue = 0.005,
                  .unit = "metres",
                  .minimum = 0.001,
                  .maximum = 2.0},
        &InfraredSensorComponent::minimumRange)

      .Editable({.key = "maximumRange",
                  .displayName = "Maximum Range",
                  .kind = K::Number,
                  .defaultValue = 0.5,
                  .unit = "metres",
                  .minimum = 0.01,
                  .maximum = 2.0},
        &InfraredSensorComponent::maximumRange)

      .Editable({.key = "coneAngleDegrees",
                  .displayName = "Optical Field of View",
                  .kind = K::Number,
                  .defaultValue = 6.0,
                  .unit = "degrees",
                  .minimum = 1.0,
                  .maximum = 30.0},
        &InfraredSensorComponent::coneAngleDegrees)

      .Editable(
        {.key = "enabled", .displayName = "Enabled", .kind = K::Boolean, .defaultValue = true},
        &InfraredSensorComponent::enabled)

      .Editable({.key = "samplePeriod",
                  .displayName = "Sample Interval",
                  .kind = K::Number,
                  .defaultValue = .02,
                  .unit = "seconds",
                  .minimum = .001,
                  .maximum = 1},
        &InfraredSensorComponent::samplePeriod)
      .Editable({.key = "noiseAmplitude",
                  .displayName = "Range Noise Amplitude",
                  .kind = K::Number,
                  .defaultValue = 0.0,
                  .unit = "metres",
                  .minimum = 0,
                  .maximum = .05},
        &InfraredSensorComponent::noiseAmplitude)
      .Editable({.key = "randomSeed",
                  .displayName = "Random Seed",
                  .kind = K::Integer,
                  .defaultValue = std::int64_t{42},
                  .minimum = 0,
                  .maximum = 2147483647},
        &InfraredSensorComponent::randomSeed)
      .Validate("Finite optical range, mount and sampling settings required",
        [](const auto& sensor) { return sensor.Valid(); });
  }
};

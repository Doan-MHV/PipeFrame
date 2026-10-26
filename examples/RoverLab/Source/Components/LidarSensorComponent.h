#pragma once
#include <PipeFrame/Project/Component.h>

#include <cmath>
#include <cstdint>

struct LidarSensorComponent : pipeframe::Component<LidarSensorComponent> {
  pipeframe::SceneObjectReference parent{};
  pipeframe::Vector2f localPosition{};

  double localAngleDegrees{0.0};
  double minimumRange{0.02};
  double maximumRange{2.0};
  double scanRate{10.0};

  std::int64_t samplesPerScan{360};
  double deliveryDelay{0.0};
  double noiseAmplitude{0.0};
  double dropoutProbability{0.0};
  std::int64_t randomSeed{42};

  bool enabled{true};

  bool operator==(const LidarSensorComponent&) const = default;

  bool Valid() const {
    return std::isfinite(localPosition.x) && std::isfinite(localPosition.y) &&
           std::isfinite(localAngleDegrees) && std::isfinite(minimumRange) &&
           std::isfinite(maximumRange) && minimumRange >= .001 && maximumRange > minimumRange &&
           maximumRange <= 30 && std::isfinite(scanRate) && scanRate >= 5 && scanRate <= 30 &&
           samplesPerScan >= 36 && samplesPerScan <= 720 && samplesPerScan % 12 == 0 &&
           std::isfinite(deliveryDelay) && deliveryDelay >= 0 && deliveryDelay <= 2 &&
           std::isfinite(noiseAmplitude) && noiseAmplitude >= 0 && noiseAmplitude <= 1 &&
           std::isfinite(dropoutProbability) && dropoutProbability >= 0 &&
           dropoutProbability <= 1 && randomSeed >= 0 && randomSeed <= 2147483647;
  }

  static auto BuildSchema() {
    using namespace pipeframe;
    return Define("project.LidarSensorComponent", "LiDAR Sensor")
      .Editable({.key = "parent",
                  .displayName = "Parent Rover",
                  .kind = PropertyKind::ObjectReference,
                  .defaultValue = pipeframe::SceneObjectReference{}},
        &LidarSensorComponent::parent)
      .Editable({.key = "localPosition",
                  .displayName = "Mount Position",
                  .kind = PropertyKind::Vector2,
                  .defaultValue = pipeframe::Vector2f{},
                  .unit = "metres"},
        &LidarSensorComponent::localPosition)
      .Editable({.key = "localAngleDegrees",
                  .displayName = "Mount Angle",
                  .kind = PropertyKind::Number,
                  .defaultValue = 0.0,
                  .unit = "degrees",
                  .minimum = -180,
                  .maximum = 180},
        &LidarSensorComponent::localAngleDegrees)
      .Editable({.key = "minimumRange",
                  .displayName = "Minimum Range",
                  .kind = PropertyKind::Number,
                  .defaultValue = 0.02,
                  .unit = "metres",
                  .minimum = 0.001,
                  .maximum = 10},
        &LidarSensorComponent::minimumRange)
      .Editable({.key = "maximumRange",
                  .displayName = "Maximum Range",
                  .kind = PropertyKind::Number,
                  .defaultValue = 2.0,
                  .unit = "metres",
                  .minimum = 0.01,
                  .maximum = 30},
        &LidarSensorComponent::maximumRange)
      .Editable({.key = "scanRate",
                  .displayName = "Scan Rate",
                  .kind = PropertyKind::Number,
                  .defaultValue = 10.0,
                  .unit = "Hz",
                  .minimum = 5,
                  .maximum = 30},
        &LidarSensorComponent::scanRate)
      .Editable({.key = "samplesPerScan",
                  .displayName = "Samples Per Revolution",
                  .kind = PropertyKind::Integer,
                  .defaultValue = std::int64_t{360},
                  .minimum = 36,
                  .maximum = 720},
        &LidarSensorComponent::samplesPerScan)
      .Editable({.key = "deliveryDelay",
                  .displayName = "Delivery Delay",
                  .kind = PropertyKind::Number,
                  .defaultValue = 0.0,
                  .unit = "seconds",
                  .minimum = 0,
                  .maximum = 2},
        &LidarSensorComponent::deliveryDelay)
      .Editable({.key = "noiseAmplitude",
                  .displayName = "Distance Noise Amplitude",
                  .kind = PropertyKind::Number,
                  .defaultValue = 0.0,
                  .unit = "metres",
                  .minimum = 0,
                  .maximum = 1},
        &LidarSensorComponent::noiseAmplitude)
      .Editable({.key = "dropoutProbability",
                  .displayName = "Beam Dropout Probability",
                  .kind = PropertyKind::Number,
                  .defaultValue = 0.0,
                  .minimum = 0,
                  .maximum = 1},
        &LidarSensorComponent::dropoutProbability)
      .Editable({.key = "randomSeed",
                  .displayName = "Random Seed",
                  .kind = PropertyKind::Integer,
                  .defaultValue = std::int64_t{42},
                  .minimum = 0,
                  .maximum = 2147483647},
        &LidarSensorComponent::randomSeed)
      .Editable({.key = "enabled",
                  .displayName = "Enabled",
                  .kind = PropertyKind::Boolean,
                  .defaultValue = true},
        &LidarSensorComponent::enabled)
      .Validate("Invalid LiDAR settings; samples per revolution must be a multiple of 12",
        [](const auto& value) { return value.Valid(); });
  }
};

#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>
#include <cstdint>

struct UltrasonicScheduleComponent : pipeframe::Component<UltrasonicScheduleComponent> {
  double samplePeriod{0.10};
  double slotPeriod{0.025};
  double deliveryDelay{0.0};
  bool sequential{false};

  // Uniform distance error in [-noiseAmplitude, +noiseAmplitude].
  double noiseAmplitude{0.0};

  // Probability per capture: 0 = never, 1 = always.
  double dropoutProbability{0.0};

  // int64_t matches PipeFrame's Integer property representation.
  std::int64_t randomSeed{42};

  bool operator==(const UltrasonicScheduleComponent&) const = default;

  bool Valid() const {
    return std::isfinite(samplePeriod) && samplePeriod >= 0.01 && samplePeriod <= 10.0 &&
           std::isfinite(slotPeriod) && slotPeriod >= 0.001 && slotPeriod <= 10.0 &&
           std::isfinite(deliveryDelay) && deliveryDelay >= 0.0 && deliveryDelay <= 2.0 &&
           std::isfinite(noiseAmplitude) && noiseAmplitude >= 0.0 && noiseAmplitude <= 1.0 &&
           std::isfinite(dropoutProbability) && dropoutProbability >= 0.0 &&
           dropoutProbability <= 1.0 && randomSeed >= 0 && randomSeed <= 2147483647;
  }

  static auto BuildSchema() {
    using namespace pipeframe;

    return Define(
      "project.UltrasonicScheduleComponent", "Ultrasonic Schedule")
      .Editable(
        {
          .key = "sequential",
          .displayName = "Sequential Sampling",
          .kind = PropertyKind::Boolean,
          .defaultValue = false,
        },
        &UltrasonicScheduleComponent::sequential)
      .Editable(
        {
          .key = "samplePeriod",
          .displayName = "Per-Sensor Sample Interval",
          .kind = PropertyKind::Number,
          .defaultValue = 0.10,
          .unit = "seconds",
          .minimum = 0.01,
          .maximum = 10.0,
        },
        &UltrasonicScheduleComponent::samplePeriod)
      .Editable(
        {
          .key = "slotPeriod",
          .displayName = "Sequential Capture Gap",
          .kind = PropertyKind::Number,
          .defaultValue = 0.025,
          .unit = "seconds",
          .minimum = 0.001,
          .maximum = 10.0,
        },
        &UltrasonicScheduleComponent::slotPeriod)
      .Editable(
        {
          .key = "deliveryDelay",
          .displayName = "Delivery Delay",
          .kind = PropertyKind::Number,
          .defaultValue = 0.0,
          .unit = "seconds",
          .minimum = 0.0,
          .maximum = 2.0,
        },
        &UltrasonicScheduleComponent::deliveryDelay)
      .Editable(
        {
          .key = "noiseAmplitude",
          .displayName = "Distance Noise Amplitude",
          .kind = PropertyKind::Number,
          .defaultValue = 0.0,
          .unit = "metres",
          .minimum = 0.0,
          .maximum = 1.0,
        },
        &UltrasonicScheduleComponent::noiseAmplitude)
      .Editable(
        {
          .key = "dropoutProbability",
          .displayName = "Dropout Probability",
          .kind = PropertyKind::Number,
          .defaultValue = 0.0,
          .minimum = 0.0,
          .maximum = 1.0,
        },
        &UltrasonicScheduleComponent::dropoutProbability)
      .Editable(
        {
          .key = "randomSeed",
          .displayName = "Random Seed",
          .kind = PropertyKind::Integer,
          .defaultValue = std::int64_t{42},
          .minimum = 0.0,
          .maximum = 2147483647.0,
        },
        &UltrasonicScheduleComponent::randomSeed)
      .Validate(
        "Use valid sampling intervals, finite noise, a dropout probability "
        "from 0 to 1, and a nonnegative integer seed",
        [](const auto& settings) { return settings.Valid(); });
  }
};

#pragma once

#include <PipeFrame/Project/Component.h>

#include <algorithm>
#include <cmath>
#include <numbers>

// Optional ideal differential-drive hardware. SI units; no slip, battery or motor curve.
struct MicromouseHardwareComponent : pipeframe::Component<MicromouseHardwareComponent> {
  double mass{.10};
  double length{.10};
  double width{.07};
  double wheelRadius{.016};
  double trackWidth{.055};
  double maximumRPM{120};
  double wheelTorque{.001};

  bool operator==(const MicromouseHardwareComponent&) const = default;

  bool Valid() const {
    return InRange(mass, .001, 10) && InRange(length, .01, 2) && InRange(width, .01, 2) &&
           InRange(wheelRadius, .001, .5) && InRange(trackWidth, .001, width) &&
           InRange(maximumRPM, 1, 10000) && InRange(wheelTorque, .00001, 10);
  }

  double WheelSpeed() const { return maximumRPM * 2 * std::numbers::pi * wheelRadius / 60; }
  double Acceleration() const { return 2 * wheelTorque / (wheelRadius * mass); }
  double AngularAcceleration() const {
    const double inertia = mass * (length * length + width * width) / 12;
    return wheelTorque * trackWidth / (wheelRadius * inertia);
  }

  // Slew from actual constrained motion, not a command that physics may have refused.
  // Explicit stops and safety refusals remain immediate in this kinematic model.
  void LimitAcceleration(double previousForward,
    double previousTurn,
    double delta,
    double& forward,
    double& turn) const {
    const double dv = forward == 0 ? 0 : forward - previousForward;
    const double dw = turn == 0 ? 0 : turn - previousTurn;
    // Both channels share the two wheel torques; an arc cannot spend the full
    // torque budget independently on translation and rotation.
    const double requiredTime =
      std::abs(dv) / Acceleration() + std::abs(dw) / AngularAcceleration();
    const double fraction = requiredTime > delta ? delta / requiredTime : 1;
    if (forward != 0) forward = previousForward + dv * fraction;
    if (turn != 0) turn = previousTurn + dw * fraction;
  }

  static auto BuildSchema() {
    using namespace pipeframe;
    return Define("project.MicromouseHardwareComponent", "Micromouse Hardware")
      .OnlyOn("project.RoverEntity")
      .Editable({.key = "mass",
                  .displayName = "Mass",
                  .kind = PropertyKind::Number,
                  .defaultValue = .10,
                  .unit = "kg",
                  .minimum = .001,
                  .maximum = 10},
        &MicromouseHardwareComponent::mass)
      .Editable({.key = "length",
                  .displayName = "Chassis Length",
                  .kind = PropertyKind::Number,
                  .defaultValue = .10,
                  .unit = "m",
                  .minimum = .01,
                  .maximum = 2},
        &MicromouseHardwareComponent::length)
      .Editable({.key = "width",
                  .displayName = "Chassis Width",
                  .kind = PropertyKind::Number,
                  .defaultValue = .07,
                  .unit = "m",
                  .minimum = .01,
                  .maximum = 2},
        &MicromouseHardwareComponent::width)
      .Editable({.key = "wheelRadius",
                  .displayName = "Wheel Radius",
                  .kind = PropertyKind::Number,
                  .defaultValue = .016,
                  .unit = "m",
                  .minimum = .001,
                  .maximum = .5},
        &MicromouseHardwareComponent::wheelRadius)
      .Editable({.key = "trackWidth",
                  .displayName = "Wheel Track Width",
                  .kind = PropertyKind::Number,
                  .defaultValue = .055,
                  .unit = "m",
                  .minimum = .001,
                  .maximum = 2},
        &MicromouseHardwareComponent::trackWidth)
      .Editable({.key = "maximumRPM",
                  .displayName = "Maximum Wheel RPM",
                  .kind = PropertyKind::Number,
                  .defaultValue = 120.0,
                  .unit = "rpm",
                  .minimum = 1,
                  .maximum = 10000},
        &MicromouseHardwareComponent::maximumRPM)
      .Editable({.key = "wheelTorque",
                  .displayName = "Torque Per Wheel (ideal limit)",
                  .kind = PropertyKind::Number,
                  .defaultValue = .001,
                  .unit = "N m",
                  .minimum = .00001,
                  .maximum = 10},
        &MicromouseHardwareComponent::wheelTorque)
      .Validate("Use finite positive hardware values; wheel track must fit chassis width",
        [](const auto& value) { return value.Valid(); });
  }

private:
  static bool InRange(double value, double minimum, double maximum) {
    return std::isfinite(value) && value >= minimum && value <= maximum;
  }
};

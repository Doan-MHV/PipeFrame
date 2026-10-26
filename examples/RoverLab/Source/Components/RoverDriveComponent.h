#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>

struct RoverDriveComponent : pipeframe::Component<RoverDriveComponent> {
    double speed{0.16};
    double stopDistance{0.35};
    double readingTimeout{0.3};
    double commandTimeout{0.25};
    double trackWidth{0.18};
    double maximumWheelSpeed{0.30};
    double maximumTurnRate{2.0};  // radians per second
    bool enabled{true};

    static auto BuildSchema() {
        using namespace pipeframe;

        return Define("project.RoverDriveComponent", "Rover Drive")
            .RetiredField("frontSensor")
            .RetiredField("sensor")
            .Editable({.key = "speed",
                          .displayName = "Maximum Speed",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.16,
                          .unit = "metres / second",
                          .minimum = 0},
                &RoverDriveComponent::speed)
            .Editable({.key = "stopDistance",
                          .displayName = "Stop Distance",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.35,
                          .unit = "metres",
                          .minimum = 0},
                &RoverDriveComponent::stopDistance)
            .Editable({.key = "readingTimeout",
                          .displayName = "Reading Timeout",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.3,
                          .unit = "seconds",
                          .minimum = 0.001},
                &RoverDriveComponent::readingTimeout)
            .Editable({.key = "commandTimeout",
                          .displayName = "Command Timeout",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.25,
                          .unit = "seconds",
                          .minimum = 0.001},
                &RoverDriveComponent::commandTimeout)
            .Editable({.key = "trackWidth",
                          .displayName = "Track Width",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.18,
                          .unit = "metres",
                          .minimum = 0.001},
                &RoverDriveComponent::trackWidth)
            .Editable({.key = "maximumWheelSpeed",
                          .displayName = "Maximum Wheel Speed",
                          .kind = PropertyKind::Number,
                          .defaultValue = 0.30,
                          .unit = "metres / second",
                          .minimum = 0},
                &RoverDriveComponent::maximumWheelSpeed)
            .Editable({.key = "maximumTurnRate",
                          .displayName = "Maximum Turn Rate",
                          .kind = PropertyKind::Number,
                          .defaultValue = 2.0,
                          .unit = "radians / second",
                          .minimum = 0},
                &RoverDriveComponent::maximumTurnRate)
            .Editable({.key = "enabled",
                          .displayName = "Enabled",
                          .kind = PropertyKind::Boolean,
                          .defaultValue = true},
                &RoverDriveComponent::enabled)
            .Validate(
                "Drive settings must be finite; speed limits and stop distance "
                "must be nonnegative; track width and timeouts must be positive",
                [](const auto& drive) { return drive.ValidSettings(); });
    }

    bool ValidSettings() const {
        return std::isfinite(speed) && speed >= 0 && std::isfinite(stopDistance) &&
               stopDistance >= 0 && std::isfinite(readingTimeout) && readingTimeout > 0 &&
               std::isfinite(commandTimeout) && commandTimeout > 0 && std::isfinite(trackWidth) &&
               trackWidth > 0 && std::isfinite(maximumWheelSpeed) && maximumWheelSpeed >= 0 &&
               std::isfinite(maximumTurnRate) && maximumTurnRate >= 0;
    }
};

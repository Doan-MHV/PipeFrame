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
            .Editable("speed", "Maximum Speed", &RoverDriveComponent::speed, {.unit = "metres / second", .minimum = 0})
            .Editable("stopDistance", "Stop Distance", &RoverDriveComponent::stopDistance, {.unit = "metres", .minimum = 0})
            .Editable("readingTimeout", "Reading Timeout", &RoverDriveComponent::readingTimeout, {.unit = "seconds", .minimum = 0.001})
            .Editable("commandTimeout", "Command Timeout", &RoverDriveComponent::commandTimeout, {.unit = "seconds", .minimum = 0.001})
            .Editable("trackWidth", "Track Width", &RoverDriveComponent::trackWidth, {.unit = "metres", .minimum = 0.001})
            .Editable("maximumWheelSpeed", "Maximum Wheel Speed", &RoverDriveComponent::maximumWheelSpeed, {.unit = "metres / second", .minimum = 0})
            .Editable("maximumTurnRate", "Maximum Turn Rate", &RoverDriveComponent::maximumTurnRate, {.unit = "radians / second", .minimum = 0})
            .Editable("enabled", "Enabled", &RoverDriveComponent::enabled)
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

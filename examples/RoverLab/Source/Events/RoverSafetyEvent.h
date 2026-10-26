#pragma once

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/Event.h>

#include <string>
#include <string_view>

namespace RoverLab {

enum class RoverSafetyStatus {
    Clear,
    Stopped,
    NoCommand,
    Disabled,
    MissingConfiguration,
    MissingCoverage,
    WaitingForReading,
    StaleReading,
    InvalidReading,
    Obstacle
};

inline std::string_view SafetyStatusText(RoverSafetyStatus status) {
    switch (status) {
        case RoverSafetyStatus::Clear:
            return "Clear";

        case RoverSafetyStatus::Stopped:
            return "Stop requested";

        case RoverSafetyStatus::NoCommand:
            return "No fresh command";

        case RoverSafetyStatus::Disabled:
            return "Drive disabled or invalid";

        case RoverSafetyStatus::MissingConfiguration:
            return "Check Rover Safety and scale";

        case RoverSafetyStatus::MissingCoverage:
            return "Required sensor direction missing";

        case RoverSafetyStatus::WaitingForReading:
            return "Waiting for sensor reading";

        case RoverSafetyStatus::StaleReading:
            return "Sensor reading expired";

        case RoverSafetyStatus::InvalidReading:
            return "Invalid sensor reading or settings";

        case RoverSafetyStatus::Obstacle:
            return "Obstacle inside safety clearance";
    }

    return "Unknown";
}

struct RoverSafetyEvent final : pipeframe::Event {
    pipeframe::SceneObject rover;
    RoverSafetyStatus status{RoverSafetyStatus::NoCommand};
    std::string reason;
    // Signed drive output before collision constraints; zero when rejected.
    double appliedForwardSpeed{};
};

} // namespace RoverLab
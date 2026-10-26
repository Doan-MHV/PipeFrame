#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>

struct RoverSafetyComponent : pipeframe::Component<RoverSafetyComponent> {
    double halfLength{0.16};
    double halfWidth{0.09};
    double clearance{0.03};

    bool ValidSettings() const {
        return std::isfinite(halfLength) && halfLength > 0 &&
               std::isfinite(halfWidth) && halfWidth > 0 &&
               std::isfinite(clearance) && clearance >= 0;
    }

    static auto BuildSchema() {
        using namespace pipeframe;

        return Define(
            "project.RoverSafetyComponent",
            "Rover Safety"
        )
            .Editable(
                {
                    .key = "halfLength",
                    .displayName = "Half Length",
                    .kind = PropertyKind::Number,
                    .defaultValue = 0.16,
                    .unit = "metres",
                    .minimum = 0.001
                },
                &RoverSafetyComponent::halfLength
            )
            .Editable(
                {
                    .key = "halfWidth",
                    .displayName = "Half Width",
                    .kind = PropertyKind::Number,
                    .defaultValue = 0.09,
                    .unit = "metres",
                    .minimum = 0.001
                },
                &RoverSafetyComponent::halfWidth
            )
            .Editable(
                {
                    .key = "clearance",
                    .displayName = "Safety Clearance",
                    .kind = PropertyKind::Number,
                    .defaultValue = 0.03,
                    .unit = "metres",
                    .minimum = 0
                },
                &RoverSafetyComponent::clearance
            )
            .Validate(
                "Dimensions must be positive and clearance nonnegative",
                [](const auto& safety) {
                    return safety.ValidSettings();
                }
            );
    }
};
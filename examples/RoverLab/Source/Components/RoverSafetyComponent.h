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

        return Define("project.RoverSafetyComponent", "Rover Safety")
            .Editable("halfLength", "Half Length", &RoverSafetyComponent::halfLength, {.unit = "metres", .minimum = 0.001})
            .Editable("halfWidth", "Half Width", &RoverSafetyComponent::halfWidth, {.unit = "metres", .minimum = 0.001})
            .Editable("clearance", "Safety Clearance", &RoverSafetyComponent::clearance, {.unit = "metres", .minimum = 0})
            .Validate("Dimensions must be positive and clearance nonnegative",
                      [](const auto& safety) { return safety.ValidSettings(); });
    }
};
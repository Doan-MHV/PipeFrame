#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
namespace ant_simulation {
inline constexpr const char *SignalBeaconTypeId = "ant.signal-beacon";
struct SignalBeaconComponent {
    double radius{5.0};
    double rotationSpeed{90.0};
    pipeframe::Color color{40, 210, 240, 255};
    static auto Schema() {
        using namespace pipeframe;
        return ComponentSchema<SignalBeaconComponent>(SignalBeaconTypeId, "Signal Beacon")
            .Required()
            .Editable({.key = "radius",
                       .displayName = "Radius",
                       .kind = PropertyKind::Number,
                       .defaultValue = 5.0,
                       .unit = "m",
                       .minimum = 0.5,
                       .maximum = 32,
                       .step = 0.5},
                      &SignalBeaconComponent::radius)
            .Editable({.key = "rotationSpeed",
                       .displayName = "Rotation Speed",
                       .kind = PropertyKind::Number,
                       .defaultValue = 90.0,
                       .unit = "degrees/s",
                       .minimum = -360,
                       .maximum = 360,
                       .step = 5},
                      &SignalBeaconComponent::rotationSpeed)
            .Editable({.key = "color",
                       .displayName = "Color",
                       .kind = PropertyKind::Color,
                       .defaultValue = Color{40, 210, 240, 255}},
                      &SignalBeaconComponent::color);
    }
};
} // namespace ant_simulation

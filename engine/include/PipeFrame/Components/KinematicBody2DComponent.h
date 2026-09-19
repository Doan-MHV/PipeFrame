#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

namespace pipeframe {

inline constexpr const char* KinematicBody2DComponentTypeId = "pipeframe.kinematic-body2d";
// Velocity-driven body. Uses an attached enabled non-trigger Box Environment
// Collider when present, otherwise a world-space circle. No forces or dynamic impulses.
struct KinematicBody2DComponent {
    Vector2f velocity{};
    float radius{.5f};
    std::int64_t layerMask{4294967295LL};
    bool enabled{true};
    static auto Schema() {
        return ComponentSchema<KinematicBody2DComponent>(KinematicBody2DComponentTypeId, "Kinematic Body")
            .Editable({.key = "velocity",
                       .displayName = "Velocity",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{},
                       .unit = "world units / second"},
                      &KinematicBody2DComponent::velocity)
            .Editable({.key = "radius",
                       .displayName = "Fallback Circle Radius",
                       .kind = PropertyKind::Number,
                       .defaultValue = .5,
                       .minimum = .001,
                       .maximum = 10000},
                      &KinematicBody2DComponent::radius)
            .Editable({.key = "layerMask",
                       .displayName = "Collision Mask",
                       .kind = PropertyKind::Integer,
                       .defaultValue = std::int64_t{4294967295LL},
                       .minimum = 0,
                       .maximum = 4294967295.0},
                      &KinematicBody2DComponent::layerMask)
            .Editable({.key = "enabled", .displayName = "Enabled", .kind = PropertyKind::Boolean, .defaultValue = true},
                      &KinematicBody2DComponent::enabled);
    }
};

}  // namespace pipeframe

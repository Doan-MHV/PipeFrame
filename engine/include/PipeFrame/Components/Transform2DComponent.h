#pragma once
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Project/ComponentSchema.h>

#include <numbers>
namespace pipeframe {
struct Transform2DComponent {
    static auto Schema() {
        constexpr double toDegrees = 180.0 / std::numbers::pi;
        return ComponentSchema<Transform2DComponent>(Transform2DComponentTypeId, "Transform")
            .Required()
            .Editable({.key = "position",
                       .displayName = "Position",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{},
                       .unit = "world units"},
                      &Transform2DComponent::position)
            .Accessor(
                {.key = "rotation",
                 .displayName = "Rotation",
                 .kind = PropertyKind::Number,
                 .defaultValue = 0.0,
                 .editable = true,
                 .unit = "degrees"},
                [](const auto& value) -> PropertyValue { return double(value.rotation) * toDegrees; },
                [](auto& object, const PropertyValue& value) {
                    const auto radians = std::get<double>(value) / toDegrees;
                    if (std::abs(radians) > std::numeric_limits<float>::max())
                        throw std::invalid_argument("Rotation exceeds runtime precision range");
                    object.rotation = static_cast<float>(radians);
                })
            .Editable(
                {.key = "scale", .displayName = "Scale", .kind = PropertyKind::Vector2, .defaultValue = Vector2f{1, 1}},
                &Transform2DComponent::scale);
    }
    Vector2f position{};
    float rotation{};
    Vector2f scale{1, 1};
};
struct Motion2DComponent {
    static auto Schema() {
        return ComponentSchema<Motion2DComponent>("pipeframe.motion2d", "Motion")
            .ReadOnly({.key = "velocity",
                       .displayName = "Velocity",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{}},
                      &Motion2DComponent::velocity)
            .ReadOnly({.key = "speed", .displayName = "Speed", .kind = PropertyKind::Number, .defaultValue = 0.0},
                      &Motion2DComponent::speed)
            .ReadOnly({.key = "travelDistance",
                       .displayName = "Travel Distance",
                       .kind = PropertyKind::Number,
                       .defaultValue = 0.0},
                      &Motion2DComponent::travelDistance);
    }
    Vector2f velocity{};
    float speed{};
    float travelDistance{};
};
}  // namespace pipeframe

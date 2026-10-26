#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Environment/PlaygroundGeometry.h>
namespace pipeframe {
inline constexpr const char* SpriteRendererComponentTypeId = "pipeframe.sprite-renderer2d";
inline constexpr const char* SpriteEntityTypeId = "pipeframe.sprite2d";
struct SpriteRendererComponent {
    AssetReference texture;
    Vector2f size{1, 1};
    Color tint{255, 255, 255, 255};
    std::int64_t sortingOrder{};
    bool visible{true};
    static auto Schema() {
        return ComponentSchema<SpriteRendererComponent>(SpriteRendererComponentTypeId, "Sprite Renderer")
            .Editable({.key = "texture",
                       .displayName = "Texture",
                       .kind = PropertyKind::AssetReference,
                       .defaultValue = AssetReference{},
                       .editorHint = "asset:Texture"},
                      &SpriteRendererComponent::texture)
            .Editable({.key = "size",
                       .displayName = "Size",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{1, 1},
                       .minimum = .001,
                       .maximum = 10000},
                      &SpriteRendererComponent::size)
            .Editable({.key = "tint",
                       .displayName = "Tint",
                       .kind = PropertyKind::Color,
                       .defaultValue = Color{255, 255, 255, 255}},
                      &SpriteRendererComponent::tint)
            .Editable({.key = "sortingOrder",
                       .displayName = "Sorting Order",
                       .kind = PropertyKind::Integer,
                       .defaultValue = std::int64_t{0},
                       .minimum = -100000,
                       .maximum = 100000},
                      &SpriteRendererComponent::sortingOrder)
            .Editable({.key = "visible", .displayName = "Visible", .kind = PropertyKind::Boolean, .defaultValue = true},
                      &SpriteRendererComponent::visible);
    }
};
inline std::array<Vector2f, 4> SpriteCorners(const SpriteRendererComponent& sprite, const Transform2DComponent& pose) {
    const auto half = sprite.size * .5f;
    return {PlaygroundToWorld(-half, pose), PlaygroundToWorld({half.x, -half.y}, pose), PlaygroundToWorld(half, pose),
            PlaygroundToWorld({-half.x, half.y}, pose)};
}
}  // namespace pipeframe

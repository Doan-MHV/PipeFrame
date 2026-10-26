#pragma once
#include <PipeFrame/Components/SpriteRendererComponent.h>
#include <PipeFrame/Environment/TilemapQueries.h>
#include <PipeFrame/Environment/VisualAssetModule.h>
#include <PipeFrame/Physics/ShapeQueries2D.h>
#include <PipeFrame/World/World.h>

#include <algorithm>
namespace pipeframe {
// Custom rendering worlds can submit the same component data to this shared layer.
// Submission order breaks sorting ties. Adjacent sprites sharing an asset are batched.
class SpriteRenderLayer final : public RenderLayer {
public:
    explicit SpriteRenderLayer(VisualAssetModule& assets) : assets(assets) {}
    void Clear() { sprites.clear(); }
    void Submit(const Transform2DComponent& pose, const SpriteRendererComponent& sprite) {
        if (sprite.visible && tilemap_query_detail::Valid(pose) && Finite(sprite.size) && sprite.size.x > 0 &&
            sprite.size.y > 0)
            sprites.push_back({pose, sprite});
    }
    void Draw(Canvas canvas, RenderState = {}) const override {
        std::stable_sort(sprites.begin(), sprites.end(),
                         [](const auto& a, const auto& b) { return a.sprite.sortingOrder < b.sprite.sortingOrder; });
        std::vector<Vertex2D> vertices;
        std::string current;
        RenderState state;
        const auto flush = [&] {
            if (!vertices.empty()) {
                canvas.Draw(vertices.data(), vertices.size(), PrimitiveTopology::Triangles, state);
                vertices.clear();
            }
        };
        for (const auto& item : sprites) {
            const auto& sprite = item.sprite;
            Vector2f extent{1, 1};
            RenderState next;
            if (!sprite.texture.assetId.empty()) {
                const auto* resource = assets.ResolveTexture(sprite.texture);
                if (!resource || !resource->error.empty()) continue;
                extent = {float(resource->size.x), float(resource->size.y)};
                next = resource->State();
            }
            if (current != sprite.texture.assetId) {
                flush();
                current = sprite.texture.assetId;
                state = next;
            }
            const auto corners = SpriteCorners(sprite, item.pose);
            const Vector2f uv[]{{0, 0}, {extent.x, 0}, extent, {0, extent.y}};
            for (int i : {0, 1, 2, 0, 2, 3})
                vertices.push_back({corners[i], sprite.tint, uv[i]});
        }
        flush();
    }

private:
    struct Item {
        Transform2DComponent pose;
        SpriteRendererComponent sprite;
    };
    VisualAssetModule& assets;
    mutable std::vector<Item> sprites;
};
}  // namespace pipeframe

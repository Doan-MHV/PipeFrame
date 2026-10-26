#pragma once
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Render/Canvas.h>

#include <vector>

namespace pipeframe {
inline Vector2f PlaygroundToWorld(Vector2f point, const Transform2DComponent& transform) {
    point = {point.x * transform.scale.x, point.y * transform.scale.y};
    const float c = std::cos(transform.rotation), s = std::sin(transform.rotation);
    return transform.position + Vector2f{point.x * c - point.y * s, point.x * s + point.y * c};
}
inline Rectanglef PlaygroundBounds(const Transform2DComponent& transform, const PlaygroundComponent& ground) {
    const auto size = ground.Size();
    auto low = transform.position, high = low;
    for (const auto local : {Vector2f{}, Vector2f{size.x, 0}, size, Vector2f{0, size.y}}) {
        const auto p = PlaygroundToWorld(local, transform);
        low = {std::min(low.x, p.x), std::min(low.y, p.y)};
        high = {std::max(high.x, p.x), std::max(high.y, p.y)};
    }
    return {low, high - low};
}
inline bool PlaygroundContains(Vector2f point, const Transform2DComponent& transform,
                               const PlaygroundComponent& ground) {
    if (transform.scale.x == 0 || transform.scale.y == 0) return false;
    point -= transform.position;
    const float c = std::cos(transform.rotation), s = std::sin(transform.rotation);
    const Vector2f local{(point.x * c + point.y * s) / transform.scale.x,
                         (-point.x * s + point.y * c) / transform.scale.y};
    return Rectanglef{{}, ground.Size()}.Contains(local);
}
inline void DrawPlayground(Canvas canvas, const Transform2DComponent& transform, const PlaygroundComponent& ground,
                           const RenderState& state = {}, Vector2f uvExtent = {}) {
    const auto size = ground.Size();
    const auto vertex = [&](Vector2f p, Color color) { return Vertex2D{PlaygroundToWorld(p, transform), color, {}}; };
    auto a = vertex({}, ground.color), b = vertex({size.x, 0}, ground.color), c = vertex(size, ground.color),
         d = vertex({0, size.y}, ground.color);
    b.textureCoordinate = {uvExtent.x, 0};
    c.textureCoordinate = uvExtent;
    d.textureCoordinate = {0, uvExtent.y};
    const Vertex2D surface[]{a, b, c, a, c, d};
    canvas.Draw(surface, 6, PrimitiveTopology::Triangles, state);
    if (!ground.showGrid) return;
    std::vector<Vertex2D> grid;
    grid.reserve(std::size_t(ground.columns + ground.rows + 2) * 2);
    const Color line{110, 125, 140, 100};
    for (std::int64_t x = 0; x <= ground.columns; ++x) {
        grid.push_back(vertex({float(x) * ground.cellSize, 0}, line));
        grid.push_back(vertex({float(x) * ground.cellSize, size.y}, line));
    }
    for (std::int64_t y = 0; y <= ground.rows; ++y) {
        grid.push_back(vertex({0, float(y) * ground.cellSize}, line));
        grid.push_back(vertex({size.x, float(y) * ground.cellSize}, line));
    }
    canvas.Draw(grid.data(), grid.size(), PrimitiveTopology::Lines);
}
}  // namespace pipeframe

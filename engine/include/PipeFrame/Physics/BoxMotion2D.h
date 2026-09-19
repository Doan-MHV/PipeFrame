#pragma once
#include <PipeFrame/Environment/TilemapQueries.h>
#include <PipeFrame/Physics/ShapeQueries2D.h>
namespace pipeframe {
// Translational continuous SAT for a convex box against boxes or segments.
// Rotation is supplied by the current pose; angular sweep and dynamic impulses are separate concerns.
inline std::optional<ShapeHit2D> SweepBox(const EnvironmentShape2D& box, Vector2f delta,
                                          const EnvironmentShape2D& target) {
    if (box.segment || !Finite(delta)) return {};
    for (auto p : box.points)
        if (!Finite(p)) return {};
    for (auto p : target.points)
        if (!Finite(p)) return {};
    float enter = -std::numeric_limits<float>::infinity(), exit = std::numeric_limits<float>::infinity();
    Vector2f normal{};
    bool strictOverlap = true;
    const auto axisTest = [&](Vector2f edge) {
        if (LengthSquared(edge) < 1e-12f) return true;
        auto axis = NormalizeOr(Vector2f{-edge.y, edge.x});
        float amin = ShapeDot(box.points[0], axis), amax = amin, bmin = ShapeDot(target.points[0], axis), bmax = bmin;
        for (auto p : box.points) {
            const auto v = ShapeDot(p, axis);
            amin = std::min(amin, v);
            amax = std::max(amax, v);
        }
        for (int i = 1; i < (target.segment ? 2 : 4); ++i) {
            const auto v = ShapeDot(target.points[i], axis);
            bmin = std::min(bmin, v);
            bmax = std::max(bmax, v);
        }
        strictOverlap &= amin < bmax - 1e-6f && amax > bmin + 1e-6f;
        const float speed = ShapeDot(delta, axis);
        if (std::abs(speed) < 1e-9f) return amax >= bmin && amin <= bmax;
        float first = (bmin - amax) / speed, last = (bmax - amin) / speed;
        if (first > last) std::swap(first, last);
        if (first > enter) {
            enter = first;
            normal = speed > 0 ? -axis : axis;
        }
        exit = std::min(exit, last);
        return enter <= exit;
    };
    for (int i = 0; i < 4; ++i)
        if (!axisTest(box.points[(i + 1) % 4] - box.points[i])) return {};
    for (int i = 0; i < (target.segment ? 1 : 4); ++i)
        if (!axisTest(target.points[(i + 1) % 4] - target.points[i])) return {};
    if (strictOverlap) return ShapeHit2D{0, box.points[0], {}};
    if (enter < 0 || enter > 1 || exit < 0 || ShapeDot(delta, normal) >= 0) return {};
    const auto center = (box.points[0] + box.points[2]) * .5f;
    return ShapeHit2D{enter, center + delta * enter, normal};
}
inline EnvironmentShape2D TranslateBox(EnvironmentShape2D shape, Vector2f offset) {
    for (auto& p : shape.points)
        p += offset;
    return shape;
}
inline std::optional<TilemapQueryHit> SweepBoxTilemap(const Tilemap2D& map, const Transform2DComponent& pose,
                                                      const EnvironmentShape2D& box, Vector2f delta, std::uint32_t mask,
                                                      std::uint64_t id, std::optional<Rectanglef> bounds = {}) {
    const auto center = (box.points[0] + box.points[2]) * .5f;
    const auto radius = Length(box.points[0] - center);
    const auto travel = Length(delta);
    const auto candidates =
        OverlapCircleTilemap(map, pose, {center + delta * .5f, radius + travel * .5f}, mask, id, bounds);
    std::optional<TilemapQueryHit> result;
    for (const auto& cell : candidates) {
        auto low = map.Origin() + Vector2f{float(cell.cell.column), float(cell.cell.row)} * map.CellSize();
        auto high = low + Vector2f{map.CellSize(), map.CellSize()};
        if (bounds) {
            low = {std::max(low.x, bounds->position.x), std::max(low.y, bounds->position.y)};
            high = {std::min(high.x, bounds->position.x + bounds->size.x),
                    std::min(high.y, bounds->position.y + bounds->size.y)};
        }
        auto shape = AxisAlignedBox(low, high);
        for (auto& p : shape.points)
            p = PlaygroundToWorld(p, pose);
        if (auto hit = SweepBox(box, delta, shape); hit && (!result || hit->fraction * travel < result->distance)) {
            result = cell;
            result->distance = hit->fraction * travel;
            result->point = hit->point;
            result->normal = hit->normal;
        }
    }
    return result;
}
}  // namespace pipeframe

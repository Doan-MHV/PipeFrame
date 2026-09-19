#pragma once
#include <PipeFrame/Environment/TilemapQueries.h>
#include <PipeFrame/Physics/ShapeQueries2D.h>

namespace pipeframe {
// Continuous world-space circle sweep against clipped, rotated tile rectangles.
// Starting penetrations return distance zero; this query does not depenetrate.
inline std::optional<TilemapQueryHit> SweepCircleTilemap(const Tilemap2D& map, const Transform2DComponent& transform,
                                                         Circle2D circle, Vector2f displacement,
                                                         std::uint32_t mask = ~std::uint32_t{},
                                                         std::uint64_t objectId = 0,
                                                         std::optional<Rectanglef> bounds = {}) {
    using namespace tilemap_query_detail;
    const float travel = Length(displacement);
    if (!Valid(transform) || !std::isfinite(travel) || travel <= 0 || !std::isfinite(circle.radius) ||
        circle.radius < 0)
        return {};
    const auto candidates = OverlapCircleTilemap(
        map, transform, {circle.center + displacement * .5f, circle.radius + travel * .5f}, mask, objectId, bounds);
    const auto origin = Rotate(circle.center - transform.position, -transform.rotation);
    const auto delta = Rotate(displacement, -transform.rotation);
    std::optional<TilemapQueryHit> nearest;
    double best = 2;
    for (const auto& candidate : candidates) {
        auto a = map.Origin() + Vector2f{float(candidate.cell.column), float(candidate.cell.row)} * map.CellSize();
        auto b = a + Vector2f{map.CellSize(), map.CellSize()};
        if (bounds) {
            a = {std::max(a.x, bounds->position.x), std::max(a.y, bounds->position.y)};
            b = {std::min(b.x, bounds->position.x + bounds->size.x),
                 std::min(b.y, bounds->position.y + bounds->size.y)};
        }
        a = {a.x * transform.scale.x, a.y * transform.scale.y};
        b = {b.x * transform.scale.x, b.y * transform.scale.y};
        const Vector2f low{std::min(a.x, b.x), std::min(a.y, b.y)}, high{std::max(a.x, b.x), std::max(a.y, b.y)};
        const auto hit = SweepShape({origin, circle.radius}, delta, AxisAlignedBox(low, high));
        if (hit && hit->fraction < best) {
            best = hit->fraction;
            auto result = candidate;
            result.distance = hit->fraction * travel;
            result.normal = Rotate(hit->normal, transform.rotation);
            result.point = transform.position + Rotate(hit->point, transform.rotation);
            nearest = result;
        }
    }
    return nearest;
}
}  // namespace pipeframe

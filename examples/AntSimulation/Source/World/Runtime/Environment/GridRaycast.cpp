#include "World/Runtime/Environment/GridRaycast.h"

#include <PipeFrame/Spatial/GridRaycast.h>

#include <algorithm>
#include <cmath>

#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

GridRaycastResult GridRaycast::Cast(const AntEnvironment &environment, const pipeframe::Vector2f start,
                                    const pipeframe::Vector2f direction, const float maximumDistance) {
    GridRaycastResult result;
    result.distance = std::max(0.0f, maximumDistance);
    if (!environment.IsInitialized() || maximumDistance <= 0.0f || !std::isfinite(start.x) || !std::isfinite(start.y))
        return result;

    const auto hit =
        pipeframe::RaycastGrid(start, direction, maximumDistance, {0.0f, 0.0f}, 1.0f, environment.GetWidth(),
                               environment.GetHeight(), [&environment](const pipeframe::GridCoordinate coordinate) {
                                   const auto *cell = environment.TryGetCell(coordinate.column, coordinate.row);
                                   return cell && cell->wall;
                               });
    if (!hit)
        return result;
    result.hit = true;
    result.distance = std::min(hit->distance, maximumDistance);
    result.cellPosition = {hit->cell.column, hit->cell.row};
    result.normal = {static_cast<float>(hit->normal.x), static_cast<float>(hit->normal.y)};
    result.cell = environment.TryGetCell(result.cellPosition.x, result.cellPosition.y);
    return result;
}

} // namespace ant_simulation

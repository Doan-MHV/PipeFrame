#ifndef PIPEFRAME_SPATIAL_GRID_RAYCAST_H
#define PIPEFRAME_SPATIAL_GRID_RAYCAST_H

#include <PipeFrame/Data/Grid2D.h>
#include <PipeFrame/Foundation/MathTypes.h>
#include <cmath>
#include <limits>
#include <optional>

namespace pipeframe {
struct GridRaycastHit { GridCoordinate cell{}; float distance{}; Vector2i normal{}; };

template <typename IsBlocked>
std::optional<GridRaycastHit> RaycastGrid(Vector2f origin, Vector2f direction, float maxDistance,
                                         Vector2f gridOrigin, float cellSize,
                                         int columns, int rows, IsBlocked isBlocked) {
    const float length = Length(direction);
    if (!std::isfinite(length) || length <= 0 || !std::isfinite(maxDistance) || maxDistance < 0 ||
        !std::isfinite(cellSize) || cellSize <= 0 || columns <= 0 || rows <= 0 ||
        !std::isfinite(origin.x) || !std::isfinite(origin.y) ||
        !std::isfinite(gridOrigin.x) || !std::isfinite(gridOrigin.y)) return std::nullopt;
    direction /= length;
    // Clip the ray to the grid before traversing; starting outside is a supported
    // sensor case and must not walk arbitrary out-of-bounds coordinates.
    double enter=0,exit=maxDistance;
    Vector2i entryNormal{};
    const auto slab=[&](double p,double d,double minimum,double maximum,Vector2i normal) {
        if(d==0)return p>=minimum && p<maximum;
        double near=(minimum-p)/d,far=(maximum-p)/d;
        if(near>far){std::swap(near,far);normal=-normal;}
        if(near>enter){enter=near;entryNormal=normal;}
        exit=std::min(exit,far);return enter<=exit;
    };
    if(!slab(origin.x,direction.x,gridOrigin.x,double(gridOrigin.x)+double(columns)*cellSize,{-1,0}) ||
       !slab(origin.y,direction.y,gridOrigin.y,double(gridOrigin.y)+double(rows)*cellSize,{0,-1}) || exit<0)
        return std::nullopt;
    const double x=(double(origin.x)+enter*direction.x-gridOrigin.x)/cellSize;
    const double y=(double(origin.y)+enter*direction.y-gridOrigin.y)/cellSize;
    GridCoordinate cell{int(std::clamp(std::floor(x),0.0,double(columns-1))),
                        int(std::clamp(std::floor(y),0.0,double(rows-1)))};
    // A ray on an outer face pointing away does not enter the half-open grid.
    if(enter==0 && ((x>=columns&&direction.x>=0)||(x<0&&direction.x<=0)||
                    (y>=rows&&direction.y>=0)||(y<0&&direction.y<=0)))return std::nullopt;
    const int stepX = direction.x < 0 ? -1 : 1, stepY = direction.y < 0 ? -1 : 1;
    const float inf = std::numeric_limits<float>::infinity();
    const float deltaX = direction.x == 0 ? inf : std::abs(cellSize/direction.x);
    const float deltaY = direction.y == 0 ? inf : std::abs(cellSize/direction.y);
    const float edgeX = gridOrigin.x + (direction.x < 0 ? cell.column : cell.column+1)*cellSize;
    const float edgeY = gridOrigin.y + (direction.y < 0 ? cell.row : cell.row+1)*cellSize;
    float nextX = direction.x == 0 ? inf : (edgeX-origin.x)/direction.x;
    float nextY = direction.y == 0 ? inf : (edgeY-origin.y)/direction.y;
    float distance = float(enter);
    Vector2i normal=entryNormal;
    while (distance <= exit && cell.column >= 0 && cell.column < columns && cell.row >= 0 && cell.row < rows) {
        if (isBlocked(cell)) return GridRaycastHit{cell, distance, normal};
        if (nextX < nextY) { distance=nextX; nextX+=deltaX; cell.column+=stepX; normal={-stepX,0}; }
        else { distance=nextY; nextY+=deltaY; cell.row+=stepY; normal={0,-stepY}; }
    }
    return std::nullopt;
}
} // namespace pipeframe
#endif

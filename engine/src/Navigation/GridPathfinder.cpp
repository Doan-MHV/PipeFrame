#include "GridPathfinder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Map/TerrainType.h"
#include "Map/TileMap.h"

namespace
{
struct GridCell
{
    int row = 0;
    int col = 0;
};

struct OpenNode
{
    int row = 0;
    int col = 0;
    int index = 0;
    float fScore = 0.0f;
};

struct Direction
{
    int row = 0;
    int col = 0;
    float cost = 1.0f;
};

bool CanMoveOnTerrain(MovementType movementType, TerrainType terrainType)
{
    switch (movementType)
    {
    case MovementType::Land:
        return terrainType == TerrainType::Land || terrainType == TerrainType::Runway;

    case MovementType::Water:
        return terrainType == TerrainType::Water;

    case MovementType::Air:
        return terrainType != TerrainType::Blocked;
    }

    return false;
}

int ToIndex(const TileMap& tileMap, int row, int col)
{
    return row * tileMap.GetCols() + col;
}

bool IsBlockedCell(
    const TileMap& tileMap,
    const GridPathOptions& options,
    int row,
    int col,
    int startIndex,
    int goalIndex
)
{
    if (!options.blockedCells)
    {
        return false;
    }

    const int index = ToIndex(tileMap, row, col);
    if (index == startIndex || index == goalIndex)
    {
        return false;
    }

    if (index < 0 || index >= static_cast<int>(options.blockedCells->size()))
    {
        return false;
    }

    return (*options.blockedCells)[index];
}

bool CanEnterCell(
    const TileMap& tileMap,
    MovementType movementType,
    const GridPathOptions& options,
    int row,
    int col,
    int startIndex,
    int goalIndex
)
{
    return CanMoveOnTerrain(movementType, tileMap.GetTile(row, col).terrain) &&
        !IsBlockedCell(tileMap, options, row, col, startIndex, goalIndex);
}

bool CanStandAtWorldPosition(
    const TileMap& tileMap,
    MovementType movementType,
    const GridPathOptions& options,
    const glm::vec2& position,
    int startIndex,
    int goalIndex
)
{
    int row = 0;
    int col = 0;

    if (!tileMap.WorldToGrid(position.x, position.y, row, col))
    {
        return false;
    }

    return CanEnterCell(tileMap, movementType, options, row, col, startIndex, goalIndex);
}

glm::vec2 GridToWorldCenter(const TileMap& tileMap, int row, int col);

bool CanStandAtCellCenter(
    const TileMap& tileMap,
    MovementType movementType,
    const GridPathOptions& options,
    int row,
    int col,
    int startIndex,
    int goalIndex
)
{
    if (!CanEnterCell(tileMap, movementType, options, row, col, startIndex, goalIndex))
    {
        return false;
    }

    if (options.clearanceRadius <= 0.0f)
    {
        return true;
    }

    const glm::vec2 center = GridToWorldCenter(tileMap, row, col);
    const float radius = options.clearanceRadius;
    const std::vector<glm::vec2> samples = {
        center,
        {center.x - radius, center.y},
        {center.x + radius, center.y},
        {center.x, center.y - radius},
        {center.x, center.y + radius},
        {center.x - radius, center.y - radius},
        {center.x + radius, center.y - radius},
        {center.x - radius, center.y + radius},
        {center.x + radius, center.y + radius},
    };

    for (const glm::vec2& sample : samples)
    {
        if (!CanStandAtWorldPosition(
                tileMap,
                movementType,
                options,
                sample,
                startIndex,
                goalIndex
            ))
        {
            return false;
        }
    }

    return true;
}

float Heuristic(const GridCell& from, const GridCell& to)
{
    return static_cast<float>(std::abs(from.row - to.row) + std::abs(from.col - to.col));
}

glm::vec2 GridToWorldCenter(const TileMap& tileMap, int row, int col)
{
    const float tileWorldSize = tileMap.GetTileSize() * tileMap.GetScale();

    return {
        col * tileWorldSize + tileWorldSize * 0.5f,
        row * tileWorldSize + tileWorldSize * 0.5f,
    };
}

std::vector<glm::vec2> ReconstructPath(
    const TileMap& tileMap,
    const std::vector<int>& cameFrom,
    int currentIndex
)
{
    std::vector<int> indices;
    int index = currentIndex;

    while (index >= 0)
    {
        indices.push_back(index);
        index = cameFrom[index];
    }

    std::reverse(indices.begin(), indices.end());

    std::vector<glm::vec2> points;
    points.reserve(indices.size());

    for (std::size_t i = 1; i < indices.size(); i++)
    {
        const int pointIndex = indices[i];
        const int row = pointIndex / tileMap.GetCols();
        const int col = pointIndex % tileMap.GetCols();
        points.push_back(GridToWorldCenter(tileMap, row, col));
    }

    return points;
}

bool CanMoveDiagonally(
    const TileMap& tileMap,
    MovementType movementType,
    const OpenNode& current,
    const Direction& direction,
    const GridPathOptions& options,
    int startIndex,
    int goalIndex
)
{
    if (direction.row == 0 || direction.col == 0)
    {
        return true;
    }

    const int sideARow = current.row + direction.row;
    const int sideACol = current.col;
    const int sideBRow = current.row;
    const int sideBCol = current.col + direction.col;

    if (!tileMap.IsInBounds(sideARow, sideACol) || !tileMap.IsInBounds(sideBRow, sideBCol))
    {
        return false;
    }

    return CanEnterCell(tileMap, movementType, options, sideARow, sideACol, startIndex, goalIndex) &&
        CanEnterCell(tileMap, movementType, options, sideBRow, sideBCol, startIndex, goalIndex);
}
}

GridPathResult GridPathfinder::FindPath(
    const TileMap& tileMap,
    const glm::vec2& from,
    const glm::vec2& to,
    MovementType movementType,
    const GridPathOptions& options
)
{
    GridPathResult result;

    if (options.maxNodes <= 0)
    {
        return result;
    }

    GridCell start;
    GridCell goal;

    if (!tileMap.WorldToGrid(from.x, from.y, start.row, start.col) ||
        !tileMap.WorldToGrid(to.x, to.y, goal.row, goal.col))
    {
        return result;
    }

    const int tileCount = tileMap.GetRows() * tileMap.GetCols();
    const int startIndex = ToIndex(tileMap, start.row, start.col);
    const int goalIndex = ToIndex(tileMap, goal.row, goal.col);

    if (!CanMoveOnTerrain(movementType, tileMap.GetTile(start.row, start.col).terrain) ||
        !CanMoveOnTerrain(movementType, tileMap.GetTile(goal.row, goal.col).terrain))
    {
        return result;
    }

    if (startIndex == goalIndex)
    {
        result.found = true;
        return result;
    }

    std::vector<OpenNode> open;
    std::vector<bool> openFlags(tileCount, false);
    std::vector<int> cameFrom(tileCount, -1);
    std::vector<float> gScore(tileCount, std::numeric_limits<float>::infinity());

    open.push_back({
        start.row,
        start.col,
        startIndex,
        Heuristic(start, goal),
    });
    openFlags[startIndex] = true;
    gScore[startIndex] = 0.0f;

    const std::vector<Direction> directions = options.allowDiagonal
        ? std::vector<Direction>{
            {-1, 0, 1.0f},
            {1, 0, 1.0f},
            {0, -1, 1.0f},
            {0, 1, 1.0f},
            {-1, -1, 1.4142135f},
            {-1, 1, 1.4142135f},
            {1, -1, 1.4142135f},
            {1, 1, 1.4142135f},
        }
        : std::vector<Direction>{
            {-1, 0, 1.0f},
            {1, 0, 1.0f},
            {0, -1, 1.0f},
            {0, 1, 1.0f},
        };

    int visitedNodes = 0;

    while (!open.empty() && visitedNodes < options.maxNodes)
    {
        auto bestIterator = std::min_element(
            open.begin(),
            open.end(),
            [](const OpenNode& a, const OpenNode& b)
            {
                return a.fScore < b.fScore;
            }
        );

        const OpenNode current = *bestIterator;
        open.erase(bestIterator);
        openFlags[current.index] = false;
        visitedNodes++;

        if (current.index == goalIndex)
        {
            result.found = true;
            result.points = ReconstructPath(tileMap, cameFrom, current.index);
            return result;
        }

        for (const Direction& direction : directions)
        {
            const int neighborRow = current.row + direction.row;
            const int neighborCol = current.col + direction.col;

            if (!tileMap.IsInBounds(neighborRow, neighborCol))
            {
                continue;
            }

            if (!CanStandAtCellCenter(
                    tileMap,
                    movementType,
                    options,
                    neighborRow,
                    neighborCol,
                    startIndex,
                    goalIndex
                ))
            {
                continue;
            }

            if (!CanMoveDiagonally(
                    tileMap,
                    movementType,
                    current,
                    direction,
                    options,
                    startIndex,
                    goalIndex
                ))
            {
                continue;
            }

            const int neighborIndex = ToIndex(tileMap, neighborRow, neighborCol);
            const float tentativeGScore = gScore[current.index] + direction.cost;

            if (tentativeGScore >= gScore[neighborIndex])
            {
                continue;
            }

            cameFrom[neighborIndex] = current.index;
            gScore[neighborIndex] = tentativeGScore;

            if (!openFlags[neighborIndex])
            {
                open.push_back({
                    neighborRow,
                    neighborCol,
                    neighborIndex,
                    tentativeGScore + Heuristic({neighborRow, neighborCol}, goal),
                });
                openFlags[neighborIndex] = true;
            }
        }
    }

    return result;
}

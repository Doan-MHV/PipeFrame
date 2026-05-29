#ifndef PIPEFRAME_GRIDRAYCASTER_H
#define PIPEFRAME_GRIDRAYCASTER_H

#include <algorithm>
#include <cmath>
#include <functional>

#include <glm/glm.hpp>

#include "Map/TileMap.h"

struct GridRaycastHit
{
    bool hit = false;
    float distance = 0.0f;
    int row = -1;
    int col = -1;
};

class GridRaycaster
{
public:
    using TilePredicate = std::function<bool(TerrainType)>;

    static GridRaycastHit CastTileMap(
        const TileMap& tileMap,
        glm::vec2 start,
        glm::vec2 direction,
        float maxDistance,
        const TilePredicate& isBlocked
    )
    {
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length <= 0.0001f || maxDistance <= 0.0f)
        {
            return {};
        }

        direction /= length;

        const float tileWorldSize = static_cast<float>(tileMap.GetTileSize()) * tileMap.GetScale();
        if (tileWorldSize <= 0.0f)
        {
            return {};
        }

        int row = 0;
        int col = 0;
        if (!tileMap.WorldToGrid(start.x, start.y, row, col))
        {
            return {true, 0.0f, row, col};
        }

        const int stepCol = direction.x < 0.0f ? -1 : 1;
        const int stepRow = direction.y < 0.0f ? -1 : 1;
        const float invX = 1.0f / (std::abs(direction.x) <= 0.0001f ? 0.0001f : direction.x);
        const float invY = 1.0f / (std::abs(direction.y) <= 0.0001f ? 0.0001f : direction.y);
        const float nextGridX = (direction.x < 0.0f ? static_cast<float>(col) : static_cast<float>(col + 1)) *
            tileWorldSize;
        const float nextGridY = (direction.y < 0.0f ? static_cast<float>(row) : static_cast<float>(row + 1)) *
            tileWorldSize;

        float tMaxX = (nextGridX - start.x) * invX;
        float tMaxY = (nextGridY - start.y) * invY;
        const float tDeltaX = std::abs(tileWorldSize * invX);
        const float tDeltaY = std::abs(tileWorldSize * invY);
        float distance = 0.0f;

        while (distance <= maxDistance)
        {
            if (tMaxX < tMaxY)
            {
                distance = tMaxX;
                tMaxX += tDeltaX;
                col += stepCol;
            }
            else
            {
                distance = tMaxY;
                tMaxY += tDeltaY;
                row += stepRow;
            }

            if (!tileMap.IsInBounds(row, col))
            {
                return {true, std::min(distance, maxDistance), row, col};
            }

            if (isBlocked(tileMap.GetTile(row, col).terrain))
            {
                return {true, std::min(distance, maxDistance), row, col};
            }
        }

        return {false, maxDistance, row, col};
    }

    static bool CanTravelTileMap(
        const TileMap& tileMap,
        glm::vec2 from,
        glm::vec2 to,
        float radius,
        const TilePredicate& isBlocked
    )
    {
        const glm::vec2 delta = to - from;
        const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (distance <= 0.0001f)
        {
            return CanOccupyTileMap(tileMap, to, radius, isBlocked);
        }

        const glm::vec2 direction = delta / distance;
        if (CastTileMap(tileMap, from, direction, distance, isBlocked).hit)
        {
            return false;
        }

        return CanOccupyTileMap(tileMap, to, radius, isBlocked);
    }

    static bool CanOccupyTileMap(
        const TileMap& tileMap,
        glm::vec2 position,
        float radius,
        const TilePredicate& isBlocked
    )
    {
        for (glm::vec2 offset : {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(radius, 0.0f),
            glm::vec2(-radius, 0.0f),
            glm::vec2(0.0f, radius),
            glm::vec2(0.0f, -radius)
        })
        {
            const TerrainType terrain = tileMap.GetTerrainAtWorldPosition(
                position.x + offset.x,
                position.y + offset.y
            );

            if (isBlocked(terrain))
            {
                return false;
            }
        }

        return true;
    }
};

#endif // PIPEFRAME_GRIDRAYCASTER_H

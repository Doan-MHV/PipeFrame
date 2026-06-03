#ifndef PIPEFRAME_TERRAINCONSTRAINTSYSTEM_H
#define PIPEFRAME_TERRAINCONSTRAINTSYSTEM_H

#include <algorithm>

#include "Components/BoxColliderComponent.h"
#include "Components/MovementComponent.h"
#include "Components/MovementStatusComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Map/TileMap.h"

class TerrainConstraintSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<MovementComponent>();
        RequireComponent<MovementTypeComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        for (Entity entity : GetSystemEntities())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& movement = entity.GetComponent<MovementComponent>();

            if (!movement.enabled || !movement.hasPreviousPosition)
            {
                continue;
            }

            if (!entity.HasComponent<MovementStatusComponent>())
            {
                entity.AddComponent<MovementStatusComponent>();
            }

            auto& status = entity.GetComponent<MovementStatusComponent>();
            status.wasBlocked = false;
            status.blockedX = false;
            status.blockedY = false;
            status.blockedByCollision = false;

            const glm::vec2 originalPosition = movement.previousPosition;
            const glm::vec2 targetPosition = transform.position;

            if (CanOccupyTerrain(entity, transform, context.tileMap, targetPosition.x, targetPosition.y))
            {
                continue;
            }

            const bool canMoveX = CanOccupyTerrain(
                entity,
                transform,
                context.tileMap,
                targetPosition.x,
                originalPosition.y
            );
            const bool canMoveY = CanOccupyTerrain(
                entity,
                transform,
                context.tileMap,
                originalPosition.x,
                targetPosition.y
            );

            transform.position = originalPosition;

            if (canMoveX)
            {
                transform.position.x = targetPosition.x;
            }
            else
            {
                status.blockedX = true;
            }

            if (canMoveY)
            {
                transform.position.y = targetPosition.y;
            }
            else
            {
                status.blockedY = true;
            }

            status.wasBlocked = status.blockedX || status.blockedY || status.blockedByCollision;
        }
    }

private:
    struct TerrainFootprint
    {
        float left = 0.0f;
        float top = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
    };

    TerrainFootprint GetTerrainFootprint(
        Entity entity,
        const TransformComponent& transform,
        float positionX,
        float positionY
    ) const
    {
        TerrainFootprint footprint{
            positionX,
            positionY,
            1.0f,
            1.0f
        };

        if (entity.HasComponent<BoxColliderComponent>())
        {
            const auto& collider = entity.GetComponent<BoxColliderComponent>();
            footprint.left = positionX + collider.offset.x;
            footprint.top = positionY + collider.offset.y;
            footprint.width = collider.width * transform.scale.x;
            footprint.height = collider.height * transform.scale.y;
        }
        else if (entity.HasComponent<SpriteComponent>())
        {
            const auto& sprite = entity.GetComponent<SpriteComponent>();
            footprint.width = sprite.width * transform.scale.x;
            footprint.height = sprite.height * transform.scale.y;
        }

        if (footprint.width < 0.0f)
        {
            footprint.left += footprint.width;
            footprint.width *= -1.0f;
        }

        if (footprint.height < 0.0f)
        {
            footprint.top += footprint.height;
            footprint.height *= -1.0f;
        }

        footprint.width = std::max(footprint.width, 1.0f);
        footprint.height = std::max(footprint.height, 1.0f);
        return footprint;
    }

    bool CanOccupyTerrain(
        Entity entity,
        const TransformComponent& transform,
        const TileMap& tileMap,
        float positionX,
        float positionY
    ) const
    {
        const auto& movementType = entity.GetComponent<MovementTypeComponent>();
        if (movementType.type == MovementType::Air)
        {
            return true;
        }

        const TerrainFootprint footprint = GetTerrainFootprint(entity, transform, positionX, positionY);
        constexpr float edgeEpsilon = 0.01f;

        int startRow = 0;
        int startCol = 0;
        int endRow = 0;
        int endCol = 0;

        if (!tileMap.WorldToGrid(footprint.left, footprint.top, startRow, startCol))
        {
            return false;
        }

        if (!tileMap.WorldToGrid(
                footprint.left + footprint.width - edgeEpsilon,
                footprint.top + footprint.height - edgeEpsilon,
                endRow,
                endCol
            ))
        {
            return false;
        }

        for (int row = startRow; row <= endRow; row++)
        {
            for (int col = startCol; col <= endCol; col++)
            {
                if (!CanMoveOnTerrain(movementType.type, tileMap.GetTile(row, col).terrain))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool CanMoveOnTerrain(MovementType movementType, TerrainType terrainType) const
    {
        switch (movementType)
        {
        case MovementType::Land:
            return terrainType == TerrainType::Land || terrainType == TerrainType::Runway;
        case MovementType::Water:
            return terrainType == TerrainType::Water;
        case MovementType::Air:
            return true;
        }

        return false;
    }
};

#endif // PIPEFRAME_TERRAINCONSTRAINTSYSTEM_H

#ifndef PIPEFRAME_SOFTCOLLISIONSYSTEM_H
#define PIPEFRAME_SOFTCOLLISIONSYSTEM_H

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/glm.hpp>

#include "Components/BoxColliderComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/SoftCollisionComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Map/TileMap.h"

class SoftCollisionSystem : public EntitySystem
{
public:
    SoftCollisionSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SoftCollisionComponent>();
    }

    void Update(const TileMap& tileMap)
    {
        const auto entities = GetSystemEntities();
        for (std::size_t i = 0; i < entities.size(); i++)
        {
            for (std::size_t j = i + 1; j < entities.size(); j++)
            {
                ResolvePair(entities[i], entities[j], tileMap);
            }
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

    void ResolvePair(Entity entityA, Entity entityB, const TileMap& tileMap) const
    {
        auto& transformA = entityA.GetComponent<TransformComponent>();
        auto& transformB = entityB.GetComponent<TransformComponent>();
        const auto& softA = entityA.GetComponent<SoftCollisionComponent>();
        const auto& softB = entityB.GetComponent<SoftCollisionComponent>();

        const glm::vec2 centerA = transformA.position;
        const glm::vec2 centerB = transformB.position;
        glm::vec2 away = centerA - centerB;
        float distance = glm::length(away);
        const float minDistance = std::max(softA.radius + softB.radius, 0.0f);

        if (minDistance <= 0.0f || distance >= minDistance)
        {
            return;
        }

        if (distance <= 0.001f)
        {
            away = glm::vec2(1.0f, 0.0f);
            distance = 1.0f;
        }

        const glm::vec2 normal = away / distance;
        const float overlap = minDistance - distance;
        const float strength = std::clamp(std::min(softA.pushStrength, softB.pushStrength), 0.0f, 1.0f);
        const glm::vec2 correction = normal * overlap * strength;

        if (softA.immovable && softB.immovable)
        {
            return;
        }

        if (softA.immovable)
        {
            TryMove(entityB, transformB, tileMap, -correction);
            return;
        }

        if (softB.immovable)
        {
            TryMove(entityA, transformA, tileMap, correction);
            return;
        }

        TryMove(entityA, transformA, tileMap, correction * 0.5f);
        TryMove(entityB, transformB, tileMap, -correction * 0.5f);
    }

    void TryMove(
        Entity entity,
        TransformComponent& transform,
        const TileMap& tileMap,
        glm::vec2 offset
    ) const
    {
        if (glm::length(offset) <= 0.001f)
        {
            return;
        }

        const glm::vec2 originalPosition = transform.position;
        const glm::vec2 targetPosition = originalPosition + offset;
        if (CanOccupyTerrain(entity, transform, tileMap, targetPosition.x, targetPosition.y))
        {
            transform.position = targetPosition;
            return;
        }

        const glm::vec2 xTarget = originalPosition + glm::vec2(offset.x, 0.0f);
        if (std::abs(offset.x) > 0.001f &&
            CanOccupyTerrain(entity, transform, tileMap, xTarget.x, xTarget.y))
        {
            transform.position = xTarget;
        }

        const glm::vec2 yTarget = transform.position + glm::vec2(0.0f, offset.y);
        if (std::abs(offset.y) > 0.001f &&
            CanOccupyTerrain(entity, transform, tileMap, yTarget.x, yTarget.y))
        {
            transform.position = yTarget;
        }
    }

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
        if (!entity.HasComponent<MovementTypeComponent>())
        {
            return true;
        }

        const auto& movement = entity.GetComponent<MovementTypeComponent>();
        if (movement.type == MovementType::Air)
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
                const TerrainType terrain = tileMap.GetTile(row, col).terrain;
                if (!CanMoveOnTerrain(movement.type, terrain))
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

#endif // PIPEFRAME_SOFTCOLLISIONSYSTEM_H

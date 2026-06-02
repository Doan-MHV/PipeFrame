#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include <algorithm>
#include <cmath>

#include "Components/BoxColliderComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/MovementStatusComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/CollisionEvent.h"
#include "Map/TileMap.h"

class MovementSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<RigidBodyComponent>();
        // RequireComponent<MovementTypeComponent>();
    }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<CollisionEvent>(context, &MovementSystem::OnCollision);
    }

    void OnCollision(CollisionEvent& collisionEvent)
    {
        Entity entityA = collisionEvent.entityA;
        Entity entityB = collisionEvent.entityB;

        if (entityA.BelongsToGroup("enemies") && entityB.BelongsToGroup("obstacles"))
        {
            OnEnemyHitsObstacle(entityA, entityB);
        }
        else if (entityB.BelongsToGroup("enemies") && entityA.BelongsToGroup("obstacles"))
        {
            OnEnemyHitsObstacle(entityB, entityA);
        }

        if (entityA.BelongsToGroup("obstacles"))
        {
            MarkCollisionBlocked(entityB);
        }

        if (entityB.BelongsToGroup("obstacles"))
        {
            MarkCollisionBlocked(entityA);
        }
    }

    void OnEnemyHitsObstacle(Entity enemy, Entity obstacle)
    {
        if (enemy.HasComponent<RigidBodyComponent>() && obstacle.HasComponent<SpriteComponent>())
        {
            auto& rigidBody = enemy.GetComponent<RigidBodyComponent>();
            auto& sprite = enemy.GetComponent<SpriteComponent>();

            if (rigidBody.velocity.x != 0)
            {
                rigidBody.velocity.x *= -1;
                sprite.flip = (sprite.flip == SDL_FLIP_NONE) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            }

            if (rigidBody.velocity.y != 0)
            {
                rigidBody.velocity.y *= -1;
                sprite.flip = (sprite.flip == SDL_FLIP_NONE) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
            }
        }
    }

    void MarkCollisionBlocked(Entity entity)
    {
        if (!entity.HasComponent<MovementStatusComponent>())
        {
            entity.AddComponent<MovementStatusComponent>();
        }

        auto& status = entity.GetComponent<MovementStatusComponent>();
        status.wasBlocked = true;
        status.blockedByCollision = true;
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


    void Update(EntitySystemContext& context) override
    {
        for (auto entity : GetSystemEntities())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

            if (!entity.HasComponent<MovementStatusComponent>())
            {
                entity.AddComponent<MovementStatusComponent>();
            }

            auto& movementStatus = entity.GetComponent<MovementStatusComponent>();
            movementStatus.wasBlocked = false;
            movementStatus.blockedX = false;
            movementStatus.blockedY = false;
            movementStatus.blockedByCollision = false;

            const float nextX = transform.position.x + rigidbody.velocity.x * context.deltaTime;
            const float nextY = transform.position.y + rigidbody.velocity.y * context.deltaTime;

            if (CanOccupyTerrain(entity, transform, context.tileMap, nextX, transform.position.y))
            {
                transform.position.x = nextX;
            }
            else if (std::abs(rigidbody.velocity.x) > 0.001f)
            {
                movementStatus.blockedX = true;
            }

            if (CanOccupyTerrain(entity, transform, context.tileMap, transform.position.x, nextY))
            {
                transform.position.y = nextY;
            }
            else if (std::abs(rigidbody.velocity.y) > 0.001f)
            {
                movementStatus.blockedY = true;
            }

            movementStatus.wasBlocked = movementStatus.blockedX ||
                movementStatus.blockedY ||
                movementStatus.blockedByCollision;

            if (entity.HasTag("player"))
            {
                const int mapWidth = context.tileMap.GetWorldWidth();
                const int mapHeight = context.tileMap.GetWorldHeight();
                int paddingLeft = 10;
                int paddingRight = 50;
                int paddingTop = 10;
                int paddingBottom = 50;

                transform.position.x = transform.position.x < paddingLeft ? paddingLeft : transform.position.x;
                transform.position.x = transform.position.x > mapWidth - paddingRight
                                           ? mapWidth - paddingRight
                                           : transform.position.x;
                transform.position.y = transform.position.y < paddingTop ? paddingTop : transform.position.y;
                transform.position.y = transform.position.y > mapHeight - paddingBottom
                                           ? mapHeight - paddingBottom
                                           : transform.position.y;
            }

            int cullingMargin = 100;
            const int mapWidth = context.tileMap.GetWorldWidth();
            const int mapHeight = context.tileMap.GetWorldHeight();

            bool isEntityOutsideMap = (
                transform.position.x < -cullingMargin ||
                transform.position.x > mapWidth + cullingMargin ||
                transform.position.y < -cullingMargin ||
                transform.position.y > mapHeight + cullingMargin
            );

            if (isEntityOutsideMap && !entity.HasTag("player"))
            {
                entity.Kill();
            }
        }
    }

private:
    struct TerrainFootprint
    {
        float left;
        float top;
        float width;
        float height;
    };

    TerrainFootprint GetTerrainFootprint(
        Entity entity,
        const TransformComponent& transform,
        float positionX,
        float positionY
    ) const
    {
        TerrainFootprint footprint = {
            positionX,
            positionY,
            1.0f,
            1.0f,
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
};

#endif

#ifndef PIPEFRAME_DESTROYWHENOFFSCREENSYSTEM_H
#define PIPEFRAME_DESTROYWHENOFFSCREENSYSTEM_H

#include "Collision/BoxColliderGeometry.h"
#include "Components/BoxColliderComponent.h"
#include "Components/DestroyWhenOffscreenComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Game/CameraHelpers.h"

class DestroyWhenOffscreenSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<DestroyWhenOffscreenComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        for (Entity entity : GetSystemEntities())
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& destroyWhenOffscreen = entity.GetComponent<DestroyWhenOffscreenComponent>();

            const SDL_FRect entityBounds = GetEntityBounds(entity, transform);
            const SDL_FRect bounds = destroyWhenOffscreen.useCameraBounds
                ? context.camera
                : SDL_FRect{
                    0.0f,
                    0.0f,
                    static_cast<float>(context.tileMap.GetWorldWidth()),
                    static_cast<float>(context.tileMap.GetWorldHeight())
                };

            const SDL_FRect expandedBounds = ExpandRect(bounds, destroyWhenOffscreen.margin);

            const bool outsideLeft = entityBounds.x + entityBounds.w < expandedBounds.x;
            const bool outsideRight = entityBounds.x > expandedBounds.x + expandedBounds.w;
            const bool outsideAbove = entityBounds.y + entityBounds.h < expandedBounds.y;
            const bool outsideBelow = entityBounds.y > expandedBounds.y + expandedBounds.h;

            if ((destroyWhenOffscreen.destroyLeft && outsideLeft) ||
                (destroyWhenOffscreen.destroyRight && outsideRight) ||
                (destroyWhenOffscreen.destroyAbove && outsideAbove) ||
                (destroyWhenOffscreen.destroyBelow && outsideBelow))
            {
                entity.Kill();
            }
        }
    }

private:
    SDL_FRect GetEntityBounds(Entity entity, const TransformComponent& transform) const
    {
        SDL_FRect bounds = {
            transform.position.x,
            transform.position.y,
            1.0f,
            1.0f
        };

        if (entity.HasComponent<BoxColliderComponent>())
        {
            bounds = GetBoxColliderAABB(GetBoxColliderGeometry(entity));
        }
        else if (entity.HasComponent<SpriteComponent>())
        {
            const auto& sprite = entity.GetComponent<SpriteComponent>();
            bounds.w = sprite.width * transform.scale.x;
            bounds.h = sprite.height * transform.scale.y;
        }

        if (bounds.w < 0.0f)
        {
            bounds.x += bounds.w;
            bounds.w = -bounds.w;
        }

        if (bounds.h < 0.0f)
        {
            bounds.y += bounds.h;
            bounds.h = -bounds.h;
        }

        return bounds;
    }
};

#endif // PIPEFRAME_DESTROYWHENOFFSCREENSYSTEM_H

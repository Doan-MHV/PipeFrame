

#ifndef PIPEFRAME_COLLISIONSYSTEM_H
#define PIPEFRAME_COLLISIONSYSTEM_H
#include "Components/BoxColliderComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/CollisionEvent.h"

class CollisionSystem : public EntitySystem
{
public:
    CollisionSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<BoxColliderComponent>();
    }

    void Update(std::unique_ptr<EventBus>& eventBus)
    {
        auto entities = GetSystemEntities();

        for (auto i = entities.begin(); i != entities.end(); ++i)
        {
            Entity entityA = *i;
            const auto aTransform = entityA.GetComponent<TransformComponent>();
            const auto aCollider = entityA.GetComponent<BoxColliderComponent>();

            for (auto j = std::next(i); j != entities.end(); ++j)
            {
                Entity entityB = *j;
                const auto bTransform = entityB.GetComponent<TransformComponent>();

                if (const auto bCollider = entityB.GetComponent<BoxColliderComponent>(); CheckAABBCollision(
                    aTransform.position.x + aCollider.offset.x,
                    aTransform.position.y + aCollider.offset.y,
                    aCollider.width * aTransform.scale.x, aCollider.height * aTransform.scale.y,
                    bTransform.position.x + bCollider.offset.x,
                    bTransform.position.y + bCollider.offset.y,
                    bCollider.width * bTransform.scale.x, bCollider.height * bTransform.scale.y))
                {
                    eventBus->EmitEvent<CollisionEvent>(entityA, entityB);
                }
            }
        }
    }

    static bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW,
                                   double bH)
    {
        return aX < bX + bW &&
            aX + aW > bX &&
            aY < bY + bH &&
            aY + aH > bY;
    }
};

#endif //PIPEFRAME_COLLISIONSYSTEM_H

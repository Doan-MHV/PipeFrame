#ifndef PIPEFRAME_MOVEMENTSYSTEM_H
#define PIPEFRAME_MOVEMENTSYSTEM_H

#include "Components/MovementComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"

class MovementSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<RigidBodyComponent>();
        RequireComponent<MovementComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        for (Entity entity : GetSystemEntities())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            const auto& rigidBody = entity.GetComponent<RigidBodyComponent>();
            auto& movement = entity.GetComponent<MovementComponent>();

            if (!movement.enabled)
            {
                continue;
            }

            movement.previousPosition = transform.position;
            movement.hasPreviousPosition = true;

            transform.position.x += rigidBody.velocity.x * static_cast<float>(context.deltaTime);
            transform.position.y += rigidBody.velocity.y * static_cast<float>(context.deltaTime);
        }
    }
};

#endif // PIPEFRAME_MOVEMENTSYSTEM_H

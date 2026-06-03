#ifndef FLAPPY_BIRD_BIRD_SYSTEM_H
#define FLAPPY_BIRD_BIRD_SYSTEM_H

#include "Components/BirdComponent.h"
#include "Components/MovementComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/EntitySystem.h"
#include "Events/KeyPressedEvent.h"

class BirdSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<BirdComponent>();
        RequireComponent<TransformComponent>();
        RequireComponent<RigidBodyComponent>();
        RequireComponent<MovementComponent>();
    }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<KeyPressedEvent>(context, &BirdSystem::OnKeyPressed);
    }

    void Update(EntitySystemContext& context) override
    {
        const float deltaTime = static_cast<float>(context.deltaTime);

        for (auto entity : GetSystemEntities())
        {
            auto& bird = entity.GetComponent<BirdComponent>();
            auto& rigidBody = entity.GetComponent<RigidBodyComponent>();
            auto& transform = entity.GetComponent<TransformComponent>();

            if (!bird.isAlive)
            {
                rigidBody.velocity.y = 0.0f;
                continue;
            }

            rigidBody.velocity.y += bird.gravity * deltaTime;

            if (rigidBody.velocity.y > bird.maxFallSpeed)
            {
                rigidBody.velocity.y = bird.maxFallSpeed;
            }

            transform.rotation = rigidBody.velocity.y * bird.rotationStrength;
        }
    }

private:
    void OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.symbol != SDLK_SPACE)
        {
            return;
        }

        for (auto entity : GetSystemEntities())
        {
            auto& bird = entity.GetComponent<BirdComponent>();
            auto& rigidBody = entity.GetComponent<RigidBodyComponent>();

            if (!bird.isAlive)
            {
                continue;
            }

            rigidBody.velocity.y = bird.jumpVelocity;
        }
    }
};

#endif

#ifndef FLAPPY_BIRD_BIRD_SYSTEM_H
#define FLAPPY_BIRD_BIRD_SYSTEM_H

#include "Components/BirdComponent.h"
#include "Components/MovementComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/EntitySystem.h"
#include "Events/GameOverEvent.h"
#include "Events/KeyPressedEvent.h"

class BirdSystem : public EntitySystem {
    PF_SYSTEM_QUERY(PF_QUERY_FIELD(BirdComponent, bird), PF_QUERY_FIELD(TransformComponent, transform),
                    PF_QUERY_FIELD(RigidBodyComponent, rigidBody), PF_QUERY_FIELD(MovementComponent, movement))

  public:
    void Loaded() override { RequireSystemQuery(); }

    void SubscribeToEvents(EntitySystemContext &context) override {
        Listen<KeyPressedEvent>(context, &BirdSystem::OnKeyPressed);
        Listen<GameOverEvent>(context, &BirdSystem::OnGameOver);
    }

    void Update(EntitySystemContext &context) override {
        const float deltaTime = static_cast<float>(context.deltaTime);

        ForEachSystemQuery([deltaTime](Query q) {
            if (!q.bird.isAlive) {
                q.rigidBody.velocity.y = 0.0f;
                return;
            }

            q.rigidBody.velocity.y += q.bird.gravity * deltaTime;

            if (q.rigidBody.velocity.y > q.bird.maxFallSpeed) {
                q.rigidBody.velocity.y = q.bird.maxFallSpeed;
            }

            q.transform.rotation = q.rigidBody.velocity.y * q.bird.rotationStrength;
        });
    }

  private:
    void OnKeyPressed(KeyPressedEvent &event) {
        if (event.symbol != SDLK_SPACE) {
            return;
        }

        ForEachSystemQuery([](Query q) {
            if (!q.bird.isAlive) {
                return;
            }

            q.rigidBody.velocity.y = q.bird.jumpVelocity;
        });
    }

    void OnGameOver(GameOverEvent &event) {
        if (!HasEntity(event.entity)) {
            return;
        }

        Query q = GetSystemQuery(event.entity);
        q.bird.isAlive = false;
        q.rigidBody.velocity = glm::vec2(0.0f);

        Logger::Log("Game over: " + event.reason);
    }
};

#endif

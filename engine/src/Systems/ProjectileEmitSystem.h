

#ifndef PIPEFRAME_PROJECTILEEMITSYSTEM_H
#define PIPEFRAME_PROJECTILEEMITSYSTEM_H
#include "Components/BoxColliderComponent.h"
#include "Components/MovementComponent.h"
#include "Components/ProjectileComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/KeyPressedEvent.h"

class ProjectileEmitSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<ProjectileEmitterComponent>();
        RequireComponent<TransformComponent>();
    }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<KeyPressedEvent>(context, &ProjectileEmitSystem::OnKeyPressed);
    }

    void OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.symbol == SDLK_SPACE)
        {
            for (auto entity : GetSystemEntities())
            {
                if (entity.HasTag("player"))
                {
                    const auto projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
                    const auto transform = entity.GetComponent<TransformComponent>();
                    const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

                    glm::vec2 projectilePosition = transform.position;
                    int facingRow = -1;
                    if (entity.HasComponent<SpriteComponent>())
                    {
                        auto sprite = entity.GetComponent<SpriteComponent>();
                        projectilePosition.x += (transform.scale.x * sprite.width / 2);
                        projectilePosition.y += (transform.scale.y * sprite.height / 2);

                        if (sprite.height > 0)
                        {
                            facingRow = static_cast<int>(sprite.srcRect.y / static_cast<float>(sprite.height));
                        }
                    }

                    glm::vec2 projectileVelocity = projectileEmitter.projectileVelocity;
                    int directionX = 0;
                    int directionY = 0;
                    if (rigidbody.velocity.x > 0) directionX = +1;
                    if (rigidbody.velocity.x < 0) directionX = -1;
                    if (rigidbody.velocity.y > 0) directionY = +1;
                    if (rigidbody.velocity.y < 0) directionY = -1;

                    if (directionX == 0 && directionY == 0)
                    {
                        switch (facingRow)
                        {
                        case 0:
                            directionY = -1;
                            break;
                        case 1:
                            directionX = +1;
                            break;
                        case 2:
                            directionY = +1;
                            break;
                        case 3:
                            directionX = -1;
                            break;
                        default:
                            directionY = -1;
                            break;
                        }
                    }

                    projectileVelocity.x = projectileEmitter.projectileVelocity.x * directionX;
                    projectileVelocity.y = projectileEmitter.projectileVelocity.y * directionY;

                    Entity projectile = entity.registry->CreateEntity();
                    projectile.Group("projectiles");
                    projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(3.0, 3.0), 0.0);
                    projectile.AddComponent<RigidBodyComponent>(projectileVelocity);
                    projectile.AddComponent<MovementComponent>();
                    projectile.AddComponent<SpriteComponent>("bullet-texture", 4, 4, 4);
                    projectile.AddComponent<BoxColliderComponent>(4, 4);
                    projectile.AddComponent<ProjectileComponent>(projectileEmitter.isFriendly,
                                                                 projectileEmitter.hitPercentDamage,
                                                                 projectileEmitter.projectileDuration);
                }
            }
        }
    }

    void Update(EntitySystemContext& context) override
    {
        for (auto entity : GetSystemEntities())
        {
            auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
            const auto transform = entity.GetComponent<TransformComponent>();

            if (projectileEmitter.repeatFrequency == 0)
            {
                continue;
            }

            if (SDL_GetTicks() - projectileEmitter.lastEmissionTime > projectileEmitter.repeatFrequency)
            {
                glm::vec2 projectilePosition = transform.position;
                if (entity.HasComponent<SpriteComponent>())
                {
                    const auto sprite = entity.GetComponent<SpriteComponent>();
                    projectilePosition.x += (transform.scale.x * sprite.width / 2);
                    projectilePosition.y += (transform.scale.y * sprite.height / 2);
                }

                Entity projectile = context.registry.CreateEntity();
                projectile.Group("projectiles");
                projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(3.0, 3.0), 0.0);
                projectile.AddComponent<RigidBodyComponent>(projectileEmitter.projectileVelocity);
                projectile.AddComponent<MovementComponent>();
                projectile.AddComponent<SpriteComponent>("bullet-texture", 4, 4, 4);
                projectile.AddComponent<BoxColliderComponent>(4, 4);
                projectile.AddComponent<ProjectileComponent>(projectileEmitter.isFriendly,
                                                             projectileEmitter.hitPercentDamage,
                                                             projectileEmitter.projectileDuration);

                projectileEmitter.lastEmissionTime = SDL_GetTicks();
            }
        }
    }
};

#endif //PIPEFRAME_PROJECTILEEMITSYSTEM_H

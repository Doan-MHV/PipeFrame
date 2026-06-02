

#ifndef PIPEFRAME_DAMAGESYSTEM_H
#define PIPEFRAME_DAMAGESYSTEM_H
#include "Components/BoxColliderComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ProjectileComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/CollisionEvent.h"

class DamageSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<BoxColliderComponent>();
    }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<CollisionEvent>(context, &DamageSystem::OnCollision);
    }

    void OnCollision(CollisionEvent& collisionEvent)
    {
        Entity entityA = collisionEvent.entityA;
        Entity entityB = collisionEvent.entityB;

        if (entityA.BelongsToGroup("projectiles") && entityB.HasTag("player"))
        {
            OnProjectileHitsPlayer(entityA, entityB); // "a" is the projectile, "b" is the player
        }

        if (entityB.BelongsToGroup("projectiles") && entityA.HasTag("player"))
        {
            OnProjectileHitsPlayer(entityB, entityA); // "b" is the projectile, "a" is the player
        }

        if (entityA.BelongsToGroup("projectiles") && entityB.BelongsToGroup("enemies"))
        {
            OnProjectileHitsEnemy(entityA, entityB); // "a" is the projectile, "b" is the enemy
        }

        if (entityB.BelongsToGroup("projectiles") && entityA.BelongsToGroup("enemies"))
        {
            OnProjectileHitsEnemy(entityB, entityA); // "b" is the projectile, "a" is the enemy
        }
    }

    void OnProjectileHitsPlayer(Entity projectile, Entity player)
    {
        const auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

        if (!projectileComponent.isFriendly)
        {
            auto& health = player.GetComponent<HealthComponent>();

            health.healthPercentage -= projectileComponent.hitPercentDamage;
            if (health.healthPercentage <= 0)
            {
                player.Kill();
            }

            projectile.Kill();
        }
    }

    void OnProjectileHitsEnemy(Entity projectile, Entity enemy)
    {
        const auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

        if (projectileComponent.isFriendly)
        {
            auto& health = enemy.GetComponent<HealthComponent>();

            health.healthPercentage -= projectileComponent.hitPercentDamage;
            if (health.healthPercentage <= 0)
            {
                enemy.Kill();
            }

            projectile.Kill();
        }
    }
};

#endif //PIPEFRAME_DAMAGESYSTEM_H



#ifndef PIPEFRAME_PROJECTILELIFECYCLESYSTEM_H
#define PIPEFRAME_PROJECTILELIFECYCLESYSTEM_H
#include "Components/ProjectileComponent.h"
#include "ECS/ECS.h"

class ProjectileLifecycleSystem : public EntitySystem
{
public:
    ProjectileLifecycleSystem()
    {
        RequireComponent<ProjectileComponent>();
    }

    void Update()
    {
        for (auto entity : GetSystemEntities())
        {
            auto projectile = entity.GetComponent<ProjectileComponent>();

            if (SDL_GetTicks() - projectile.startTime > projectile.duration)
            {
                entity.Kill();
            }
        }
    }
};

#endif //PIPEFRAME_PROJECTILELIFECYCLESYSTEM_H

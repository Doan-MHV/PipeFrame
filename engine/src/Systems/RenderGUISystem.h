

#ifndef PIPEFRAME_RENDERGUISYSTEM_H
#define PIPEFRAME_RENDERGUISYSTEM_H
#include <imgui.h>
#include <SDL3/SDL_rect.h>

#include "Components/BoxColliderComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"

class RenderGUISystem : public EntitySystem
{
public:
    RenderGUISystem() = default;

    void Update(EntitySystemContext& context) override
    {
        (void)context;
    }
};

#endif //PIPEFRAME_RENDERGUISYSTEM_H

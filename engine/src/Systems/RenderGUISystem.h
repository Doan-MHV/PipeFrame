

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

    void Update(SDL_Renderer* renderer, const std::unique_ptr<Registry>& registry, const SDL_FRect& camera)
    {
    }
};

#endif //PIPEFRAME_RENDERGUISYSTEM_H

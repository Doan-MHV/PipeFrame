

#ifndef PIPEFRAME_CAMERAMOVEMENTSYSTEM_H
#define PIPEFRAME_CAMERAMOVEMENTSYSTEM_H
#include <algorithm>

#include <SDL3/SDL_rect.h>

#include "Components/CameraFollowComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Map/TileMap.h"

class CameraMovementSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<CameraFollowComponent>();
        RequireComponent<TransformComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        for (auto entity : GetSystemEntities())
        {
            auto transform = entity.GetComponent<TransformComponent>();

            // Center camera on the followed entity
            context.camera.x = transform.position.x - (context.camera.w / 2.0f);
            context.camera.y = transform.position.y - (context.camera.h / 2.0f);

            // Maximum valid camera positions
            float maxCameraX = std::max(0.0f, static_cast<float>(context.tileMap.GetWorldWidth()) - context.camera.w);
            float maxCameraY = std::max(0.0f, static_cast<float>(context.tileMap.GetWorldHeight()) - context.camera.h);

            // Clamp camera inside map bounds
            context.camera.x = std::max(0.0f, std::min(context.camera.x, maxCameraX));
            context.camera.y = std::max(0.0f, std::min(context.camera.y, maxCameraY));
        }
    }
};

#endif //PIPEFRAME_CAMERAMOVEMENTSYSTEM_H

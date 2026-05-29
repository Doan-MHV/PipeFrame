

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
    CameraMovementSystem()
    {
        RequireComponent<CameraFollowComponent>();
        RequireComponent<TransformComponent>();
    }

    void Update(SDL_FRect& camera, const TileMap& tileMap)
    {
        for (auto entity : GetSystemEntities())
        {
            auto transform = entity.GetComponent<TransformComponent>();

            // Center camera on the followed entity
            camera.x = transform.position.x - (camera.w / 2.0f);
            camera.y = transform.position.y - (camera.h / 2.0f);

            // Maximum valid camera positions
            float maxCameraX = std::max(0.0f, static_cast<float>(tileMap.GetWorldWidth()) - camera.w);
            float maxCameraY = std::max(0.0f, static_cast<float>(tileMap.GetWorldHeight()) - camera.h);

            // Clamp camera inside map bounds
            camera.x = std::max(0.0f, std::min(camera.x, maxCameraX));
            camera.y = std::max(0.0f, std::min(camera.y, maxCameraY));
        }
    }
};

#endif //PIPEFRAME_CAMERAMOVEMENTSYSTEM_H

#ifndef SPRITERENDERSYSTEM_H
#define SPRITERENDERSYSTEM_H

#include "Assets/AssetRegistry.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include <SDL3/SDL.h>

class SpriteRenderSystem : public EntitySystem
{
public:
    SpriteRenderSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpriteComponent>();
    }

    void Update(SDL_Renderer* renderer, std::unique_ptr<AssetRegistry>& assetRegistry, const SDL_FRect& camera)
    {
        // Create a vector with both Sprite and Transform component of all entities
        struct RenderableEntity
        {
            TransformComponent transformComponent;
            SpriteComponent spriteComponent;
        };
        std::vector<RenderableEntity> renderableEntities;
        for (auto entity : GetSystemEntities())
        {
            RenderableEntity renderableEntity;
            renderableEntity.spriteComponent = entity.GetComponent<SpriteComponent>();
            renderableEntity.transformComponent = entity.GetComponent<TransformComponent>();

            // Check if the entity sprite is outside the camera view
            bool isOutsideCameraView = (
                renderableEntity.transformComponent.position.x + (renderableEntity.transformComponent.scale.x *
                    renderableEntity.spriteComponent.width) < camera.x ||
                renderableEntity.transformComponent.position.x > camera.x + camera.w ||
                renderableEntity.transformComponent.position.y + (renderableEntity.transformComponent.scale.y *
                    renderableEntity.spriteComponent.height) < camera.y ||
                renderableEntity.transformComponent.position.y > camera.y + camera.h
            );

            // Cull sprites that are outside the camera view (and are not fixed)
            if (isOutsideCameraView && !renderableEntity.spriteComponent.isFixed)
            {
                continue;
            }

            renderableEntities.emplace_back(renderableEntity);
        }

        // Sort the vector by the z-index value
        std::sort(renderableEntities.begin(), renderableEntities.end(),
                  [](const RenderableEntity& a, const RenderableEntity& b)
                  {
                      return a.spriteComponent.zIndex < b.spriteComponent.zIndex;
                  });

        // Loop all entities that the system is interested in
        for (const auto& entity : renderableEntities)
        {
            const auto transform = entity.transformComponent;
            const auto sprite = entity.spriteComponent;
            if (sprite.assetId.empty() || sprite.width <= 0 || sprite.height <= 0)
            {
                continue;
            }

            SDL_Texture* texture = assetRegistry->GetTexture(sprite.assetId);
            if (!texture)
            {
                continue;
            }

            // Set the source rectangle of our original sprite texture
            SDL_FRect srcRect = sprite.srcRect;
            if (srcRect.w <= 0.0f || srcRect.h <= 0.0f)
            {
                float textureWidth = 0.0f;
                float textureHeight = 0.0f;

                if (SDL_GetTextureSize(texture, &textureWidth, &textureHeight))
                {
                    srcRect.x = 0.0f;
                    srcRect.y = 0.0f;
                    srcRect.w = textureWidth;
                    srcRect.h = textureHeight;
                }
                else
                {
                    continue;
                }
            }

            SDL_FRect dstRect = {
                transform.position.x - (sprite.isFixed ? 0 : camera.x),
                transform.position.y - (sprite.isFixed ? 0 : camera.y),
                sprite.width * transform.scale.x,
                sprite.height * transform.scale.y
            };

            SDL_RenderTextureRotated(
                renderer,
                texture,
                &srcRect,
                &dstRect,
                transform.rotation,
                nullptr,
                sprite.flip
            );
        }
    }
};

#endif

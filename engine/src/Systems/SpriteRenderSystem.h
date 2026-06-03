#ifndef SPRITERENDERSYSTEM_H
#define SPRITERENDERSYSTEM_H

#include "Assets/AssetRegistry.h"
#include "Components/LayerComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Game/CameraHelpers.h"
#include <SDL3/SDL.h>
#include <cmath>

class SpriteRenderSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpriteComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        RenderLayerPass(context, false);
    }

    void RenderLayerPass(EntitySystemContext& context, bool drawBeforeTileMap)
    {
        // Create a vector with both Sprite and Transform component of all entities
        struct RenderableEntity
        {
            TransformComponent transformComponent;
            SpriteComponent spriteComponent;
            LayerComponent layerComponent;
            bool hasLayerComponent = false;
        };
        std::vector<RenderableEntity> renderableEntities;
        for (auto entity : GetSystemEntities())
        {
            RenderableEntity renderableEntity;
            renderableEntity.spriteComponent = entity.GetComponent<SpriteComponent>();
            renderableEntity.transformComponent = entity.GetComponent<TransformComponent>();
            renderableEntity.hasLayerComponent = entity.HasComponent<LayerComponent>();

            if (renderableEntity.hasLayerComponent)
            {
                renderableEntity.layerComponent = entity.GetComponent<LayerComponent>();
            }

            if (renderableEntity.layerComponent.drawBeforeTileMap != drawBeforeTileMap)
            {
                continue;
            }

            renderableEntities.emplace_back(renderableEntity);
        }

        // Sort the vector by the z-index value
        std::sort(renderableEntities.begin(), renderableEntities.end(),
                  [](const RenderableEntity& a, const RenderableEntity& b)
                  {
                      if (a.layerComponent.order != b.layerComponent.order)
                      {
                          return a.layerComponent.order < b.layerComponent.order;
                      }

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

            SDL_Texture* texture = context.assetRegistry.GetTexture(sprite.assetId);
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

            DrawSprite(context, texture, srcRect, transform, sprite, entity.layerComponent);
        }
    }

private:
    void DrawSprite(
        EntitySystemContext& context,
        SDL_Texture* texture,
        const SDL_FRect& srcRect,
        const TransformComponent& transform,
        const SpriteComponent& sprite,
        const LayerComponent& layer
    ) const
    {
        const float width = sprite.width * transform.scale.x;
        const float height = sprite.height * transform.scale.y;

        if (width <= 0.0f || height <= 0.0f)
        {
            return;
        }

        const float screenX = transform.position.x - (sprite.isFixed ? 0.0f : context.camera.x * layer.parallaxX);
        const float screenY = transform.position.y - (sprite.isFixed ? 0.0f : context.camera.y * layer.parallaxY);

        if (!layer.repeatX && !layer.repeatY)
        {
            SDL_FRect dstRect = {
                screenX,
                screenY,
                width,
                height
            };

            const SDL_FRect screenBounds = {0.0f, 0.0f, context.camera.w, context.camera.h};
            if (!sprite.isFixed && IsRectOutside(dstRect, screenBounds))
            {
                return;
            }

            SDL_RenderTextureRotated(
                context.renderer,
                texture,
                &srcRect,
                &dstRect,
                transform.rotation,
                nullptr,
                sprite.flip
            );
            return;
        }

        const float startX = layer.repeatX ? GetRepeatStart(screenX, width) : screenX;
        const float startY = layer.repeatY ? GetRepeatStart(screenY, height) : screenY;
        const float endX = layer.repeatX ? context.camera.w : screenX + width;
        const float endY = layer.repeatY ? context.camera.h : screenY + height;

        for (float y = startY; y < endY; y += height)
        {
            for (float x = startX; x < endX; x += width)
            {
                SDL_FRect dstRect = {x, y, width, height};
                SDL_RenderTextureRotated(
                    context.renderer,
                    texture,
                    &srcRect,
                    &dstRect,
                    transform.rotation,
                    nullptr,
                    sprite.flip
                );

                if (!layer.repeatX)
                {
                    break;
                }
            }

            if (!layer.repeatY)
            {
                break;
            }
        }
    }

    float GetRepeatStart(float screenPosition, float size) const
    {
        if (size <= 0.0f)
        {
            return screenPosition;
        }

        float start = std::fmod(screenPosition, size);
        if (start > 0.0f)
        {
            start -= size;
        }

        return start;
    }
};

#endif

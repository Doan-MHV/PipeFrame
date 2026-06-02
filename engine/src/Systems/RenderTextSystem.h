

#ifndef PIPEFRAME_RENDERTEXTSYSTEM_H
#define PIPEFRAME_RENDERTEXTSYSTEM_H
#include <SDL3/SDL_render.h>

#include "Assets/AssetRegistry.h"
#include "Components/TextLabelComponent.h"
#include "ECS/ECS.h"

class RenderTextSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TextLabelComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        for (auto entity : GetSystemEntities())
        {
            const auto textLabel = entity.GetComponent<TextLabelComponent>();

            SDL_Surface* surface = TTF_RenderText_Blended(context.assetRegistry.GetFont(textLabel.assetId),
                                                          textLabel.text.c_str(), textLabel.text.size(),
                                                          textLabel.color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(context.renderer, surface);
            SDL_DestroySurface(surface);

            float labelWidth = 0.0f;
            float labelHeight = 0.0f;

            SDL_GetTextureSize(texture, &labelWidth, &labelHeight);

            SDL_FRect dstRect = {
                textLabel.position.x - (textLabel.isFixed ? 0.0f : context.camera.x),
                textLabel.position.y - (textLabel.isFixed ? 0.0f : context.camera.y),
                labelWidth,
                labelHeight
            };

            SDL_RenderTexture(context.renderer, texture, nullptr, &dstRect);

            SDL_DestroyTexture(texture);
        }
    }
};

#endif //PIPEFRAME_RENDERTEXTSYSTEM_H

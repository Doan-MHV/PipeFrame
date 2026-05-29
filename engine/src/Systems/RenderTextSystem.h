

#ifndef PIPEFRAME_RENDERTEXTSYSTEM_H
#define PIPEFRAME_RENDERTEXTSYSTEM_H
#include <SDL3/SDL_render.h>

#include "Assets/AssetRegistry.h"
#include "Components/TextLabelComponent.h"
#include "ECS/ECS.h"

class RenderTextSystem : public EntitySystem
{
public:
    RenderTextSystem()
    {
        RequireComponent<TextLabelComponent>();
    }

    void Update(SDL_Renderer* renderer, std::unique_ptr<AssetRegistry>& assetRegistry, const SDL_FRect& camera)
    {
        for (auto entity : GetSystemEntities())
        {
            const auto textLabel = entity.GetComponent<TextLabelComponent>();

            SDL_Surface* surface = TTF_RenderText_Blended(assetRegistry->GetFont(textLabel.assetId),
                                                          textLabel.text.c_str(), textLabel.text.size(),
                                                          textLabel.color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);

            float labelWidth = 0.0f;
            float labelHeight = 0.0f;

            SDL_GetTextureSize(texture, &labelWidth, &labelHeight);

            SDL_FRect dstRect = {
                textLabel.position.x - (textLabel.isFixed ? 0.0f : camera.x),
                textLabel.position.y - (textLabel.isFixed ? 0.0f : camera.y),
                labelWidth,
                labelHeight
            };

            SDL_RenderTexture(renderer, texture, nullptr, &dstRect);

            SDL_DestroyTexture(texture);
        }
    }
};

#endif //PIPEFRAME_RENDERTEXTSYSTEM_H

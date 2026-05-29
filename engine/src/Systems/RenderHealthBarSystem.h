

#ifndef PIPEFRAME_RENDERHEALTHBARSYSTEM_H
#define PIPEFRAME_RENDERHEALTHBARSYSTEM_H
#include "Assets/AssetRegistry.h"
#include "Components/HealthComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"

class RenderHealthBarSystem : public EntitySystem
{
public:
    RenderHealthBarSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpriteComponent>();
        RequireComponent<HealthComponent>();
    };

    void Update(SDL_Renderer* renderer, const std::unique_ptr<AssetRegistry>& assetRegistry, const SDL_FRect& camera)
    {
        for (auto entity : GetSystemEntities())
        {
            const auto transform = entity.GetComponent<TransformComponent>();
            const auto sprite = entity.GetComponent<SpriteComponent>();
            const auto health = entity.GetComponent<HealthComponent>();

            SDL_Color healthBarColor = {255, 255, 255};

            if (health.healthPercentage >= 0 && health.healthPercentage < 40)
            {
                // 0-40 = red
                healthBarColor = {255, 0, 0};
            }
            if (health.healthPercentage >= 40 && health.healthPercentage < 80)
            {
                // 40-80 = yellow
                healthBarColor = {255, 255, 0};
            }
            if (health.healthPercentage >= 80 && health.healthPercentage <= 100)
            {
                // 80-100 = green
                healthBarColor = {0, 255, 0};
            }

            float healthBarWidth = 15;
            float healthBarHeight = 3;
            float healthBarPosX = (transform.position.x + (sprite.width * transform.scale.x)) - camera.x;
            float healthBarPosY = (transform.position.y) - camera.y;

            SDL_FRect healthBarRectangle = {
                healthBarPosX,
                healthBarPosY,
                healthBarWidth * (static_cast<float>(health.healthPercentage) / 50),
                healthBarHeight
            };
            SDL_SetRenderDrawColor(
                renderer,
                healthBarColor.r,
                healthBarColor.g,
                healthBarColor.b,
                healthBarColor.a
            );
            SDL_RenderFillRect(renderer, &healthBarRectangle);

            std::string healthText = std::to_string(health.healthPercentage);
            SDL_Surface* surface = TTF_RenderText_Blended(
                assetRegistry->GetFont("pico8-font-5"),
                healthText.c_str(),
                healthText.size(),
                healthBarColor
            );

            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);

            float labelWidth = 0;
            float labelHeight = 0;
            SDL_GetTextureSize(texture, &labelWidth, &labelHeight);

            SDL_FRect healthBarTextRectangle = {
                healthBarPosX,
                healthBarPosY + 5.0f,
                labelWidth,
                labelHeight
            };

            SDL_RenderTexture(renderer, texture, nullptr, &healthBarTextRectangle);

            SDL_DestroyTexture(texture);
        }
    }
};

#endif //PIPEFRAME_RENDERHEALTHBARSYSTEM_H

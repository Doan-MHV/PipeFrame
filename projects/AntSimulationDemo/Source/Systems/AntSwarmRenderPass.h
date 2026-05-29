#ifndef PIPEFRAME_ANT_SWARM_RENDER_PASS_H
#define PIPEFRAME_ANT_SWARM_RENDER_PASS_H

#include <cmath>

#include <SDL3/SDL.h>

#include "Assets/AssetRegistry.h"
#include "Simulation/ProjectRuntime.h"
#include "Simulations/AntSwarmSimulation.h"

class AntSwarmRenderPass : public SimulationRenderPass<AntSwarmSimulation>
{
public:
    void Render(
        const AntSwarmSimulation& simulation,
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        const SDL_FRect& camera
    ) override
    {
        SDL_Texture* antTexture = assetRegistry.GetTexture("pezza-ant-full-texture");
        if (!antTexture)
        {
            antTexture = assetRegistry.GetTexture("ant-texture");
        }

        if (!antTexture)
        {
            return;
        }

        SDL_Texture* foodTexture = assetRegistry.GetTexture("pezza-circle-texture");

        SDL_FRect srcRect{220.0f, 285.0f, 620.0f, 450.0f};

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        for (const AntAgent& ant : simulation.Agents())
        {
            constexpr float Width = 34.0f;
            constexpr float Height = 25.0f;
            const float bob = std::sin(ant.animationPhase) * 0.75f;
            const SDL_FRect dstRect{
                ant.position.x - camera.x - Width * 0.5f,
                ant.position.y - camera.y - Height * 0.5f + bob,
                Width,
                Height
            };

            if (dstRect.x + dstRect.w < 0.0f ||
                dstRect.x > camera.w ||
                dstRect.y + dstRect.h < 0.0f ||
                dstRect.y > camera.h)
            {
                continue;
            }

            constexpr float Pi = 3.14159265358979323846f;
            const float angleRadians = std::atan2(ant.direction.y, ant.direction.x);
            const double rotation = angleRadians * 180.0 / Pi;

            if (ant.state == AntState::ToHomeWithFood)
            {
                SDL_SetTextureColorMod(antTexture, 255, 232, 140);
            }
            else
            {
                SDL_SetTextureColorMod(antTexture, 255, 255, 255);
            }
            SDL_RenderTextureRotated(renderer, antTexture, &srcRect, &dstRect, rotation, nullptr, SDL_FLIP_NONE);

            if (ant.state == AntState::ToHomeWithFood)
            {
                DrawFoodPayload(renderer, foodTexture, ant, camera);
            }

            if (ant.id == simulation.GetSelectedAgentId())
            {
                DrawSelection(renderer, ant, camera);
            }
        }

        SDL_SetTextureColorMod(antTexture, 255, 255, 255);
    }

private:
    static void DrawFoodPayload(
        SDL_Renderer* renderer,
        SDL_Texture* foodTexture,
        const AntAgent& ant,
        const SDL_FRect& camera
    )
    {
        const glm::vec2 forward = SafeNormalize(ant.direction, glm::vec2(1.0f, 0.0f));
        const glm::vec2 position = ant.position + forward * 11.0f;
        SDL_FRect dstRect{
            position.x - camera.x - 4.0f,
            position.y - camera.y - 4.0f,
            8.0f,
            8.0f
        };

        if (foodTexture)
        {
            SDL_SetTextureColorMod(foodTexture, 255, 210, 72);
            SDL_SetTextureAlphaMod(foodTexture, 235);
            SDL_RenderTexture(renderer, foodTexture, nullptr, &dstRect);
            SDL_SetTextureColorMod(foodTexture, 255, 255, 255);
            SDL_SetTextureAlphaMod(foodTexture, 255);
            return;
        }

        SDL_SetRenderDrawColor(renderer, 255, 210, 72, 235);
        SDL_RenderFillRect(renderer, &dstRect);
    }

    static void DrawSelection(SDL_Renderer* renderer, const AntAgent& ant, const SDL_FRect& camera)
    {
        SDL_SetRenderDrawColor(renderer, 255, 232, 90, 255);
        SDL_FRect selectionRect{
            ant.position.x - camera.x - 20.0f,
            ant.position.y - camera.y - 20.0f,
            40.0f,
            40.0f
        };
        SDL_RenderRect(renderer, &selectionRect);
    }

    static glm::vec2 SafeNormalize(glm::vec2 value, glm::vec2 fallback)
    {
        const float length = std::sqrt(value.x * value.x + value.y * value.y);
        if (length <= 0.0001f)
        {
            return fallback;
        }

        return value / length;
    }
};

#endif // PIPEFRAME_ANT_SWARM_RENDER_PASS_H

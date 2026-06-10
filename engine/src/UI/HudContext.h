#ifndef PIPEFRAME_HUD_CONTEXT_H
#define PIPEFRAME_HUD_CONTEXT_H

#include <string>

#include <SDL3/SDL_rect.h>
#include <glm/glm.hpp>

#include "UI/HudAnchor.h"
#include "UI/HudStyle.h"

struct SDL_Renderer;
class AssetRegistry;

class HudContext
{
public:
    HudContext(
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        int screenWidth,
        int screenHeight
    );

    void DrawText(
        const std::string& text,
        HudAnchor anchor,
        glm::vec2 offset,
        const HudTextStyle& style = {}
    );

    void DrawImage(
        const std::string& textureId,
        HudAnchor anchor,
        glm::vec2 offset,
        glm::vec2 size
    );

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

private:
    SDL_Renderer* renderer = nullptr;
    AssetRegistry& assetRegistry;
    int screenWidth = 0;
    int screenHeight = 0;

    glm::vec2 ResolveAnchor(HudAnchor anchor, glm::vec2 size, glm::vec2 offset) const;
};

#endif // PIPEFRAME_HUD_CONTEXT_H

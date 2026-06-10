#include "UI/HudContext.h"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Assets/AssetRegistry.h"
#include "Logger/Logger.h"

HudContext::HudContext(
    SDL_Renderer* renderer,
    AssetRegistry& assetRegistry,
    int screenWidth,
    int screenHeight
)
    : renderer(renderer),
      assetRegistry(assetRegistry),
      screenWidth(screenWidth),
      screenHeight(screenHeight)
{
}

void HudContext::DrawText(
    const std::string& text,
    HudAnchor anchor,
    glm::vec2 offset,
    const HudTextStyle& style
)
{
    if (!renderer || text.empty())
    {
        return;
    }

    TTF_Font* font = assetRegistry.GetFont(style.fontId);
    if (!font)
    {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), style.color);
    if (!surface)
    {
        Logger::Err("Failed to render HUD text '" + text + "': " + SDL_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture)
    {
        Logger::Err("Failed to create HUD text texture '" + text + "': " + SDL_GetError());
        return;
    }

    float width = 0.0f;
    float height = 0.0f;
    SDL_GetTextureSize(texture, &width, &height);

    const glm::vec2 position = ResolveAnchor(anchor, {width, height}, offset);
    SDL_FRect dstRect = {position.x, position.y, width, height};
    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
    SDL_DestroyTexture(texture);
}

void HudContext::DrawImage(
    const std::string& textureId,
    HudAnchor anchor,
    glm::vec2 offset,
    glm::vec2 size
)
{
    if (!renderer || textureId.empty() || size.x <= 0.0f || size.y <= 0.0f)
    {
        return;
    }

    SDL_Texture* texture = assetRegistry.GetTexture(textureId);
    if (!texture)
    {
        return;
    }

    const glm::vec2 position = ResolveAnchor(anchor, size, offset);
    SDL_FRect dstRect = {position.x, position.y, size.x, size.y};
    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
}

glm::vec2 HudContext::ResolveAnchor(HudAnchor anchor, glm::vec2 size, glm::vec2 offset) const
{
    glm::vec2 position{0.0f, 0.0f};
    const float screenW = static_cast<float>(screenWidth);
    const float screenH = static_cast<float>(screenHeight);

    switch (anchor)
    {
    case HudAnchor::TopLeft:
        position = {0.0f, 0.0f};
        break;
    case HudAnchor::TopCenter:
        position = {(screenW - size.x) * 0.5f, 0.0f};
        break;
    case HudAnchor::TopRight:
        position = {screenW - size.x, 0.0f};
        break;
    case HudAnchor::CenterLeft:
        position = {0.0f, (screenH - size.y) * 0.5f};
        break;
    case HudAnchor::Center:
        position = {(screenW - size.x) * 0.5f, (screenH - size.y) * 0.5f};
        break;
    case HudAnchor::CenterRight:
        position = {screenW - size.x, (screenH - size.y) * 0.5f};
        break;
    case HudAnchor::BottomLeft:
        position = {0.0f, screenH - size.y};
        break;
    case HudAnchor::BottomCenter:
        position = {(screenW - size.x) * 0.5f, screenH - size.y};
        break;
    case HudAnchor::BottomRight:
        position = {screenW - size.x, screenH - size.y};
        break;
    }

    return position + offset;
}

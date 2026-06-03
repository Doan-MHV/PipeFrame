#ifndef PIPEFRAME_CAMERAHELPERS_H
#define PIPEFRAME_CAMERAHELPERS_H

#include <SDL3/SDL_rect.h>
#include <glm/glm.hpp>

inline glm::vec2 WorldToScreen(const SDL_FRect& camera, glm::vec2 worldPosition)
{
    return {
        worldPosition.x - camera.x,
        worldPosition.y - camera.y
    };
}

inline glm::vec2 ScreenToWorld(const SDL_FRect& camera, glm::vec2 screenPosition)
{
    return {
        screenPosition.x + camera.x,
        screenPosition.y + camera.y
    };
}

inline SDL_FRect ExpandRect(const SDL_FRect& rect, float margin)
{
    return {
        rect.x - margin,
        rect.y - margin,
        rect.w + margin * 2.0f,
        rect.h + margin * 2.0f
    };
}

inline bool RectsOverlap(const SDL_FRect& a, const SDL_FRect& b)
{
    return a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y;
}

inline bool IsRectOutside(const SDL_FRect& rect, const SDL_FRect& bounds)
{
    return !RectsOverlap(rect, bounds);
}

#endif // PIPEFRAME_CAMERAHELPERS_H

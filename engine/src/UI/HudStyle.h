#ifndef PIPEFRAME_HUD_STYLE_H
#define PIPEFRAME_HUD_STYLE_H

#include <string>

#include <SDL3/SDL_pixels.h>

struct HudTextStyle
{
    std::string fontId = "pico8-font-24";
    SDL_Color color = {255, 255, 255, 255};
};

#endif // PIPEFRAME_HUD_STYLE_H

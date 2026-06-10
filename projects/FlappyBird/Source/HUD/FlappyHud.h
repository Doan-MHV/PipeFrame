#ifndef FLAPPY_BIRD_HUD_H
#define FLAPPY_BIRD_HUD_H

#include <string>

#include <SDL3/SDL_pixels.h>

#include "State/FlappyGameState.h"
#include "UI/HudAnchor.h"
#include "UI/HudContext.h"
#include "UI/HudStyle.h"

class FlappyHud
{
public:
    void Render(HudContext& context, const FlappyGameState& state)
    {
        HudTextStyle textStyle;
        textStyle.fontId = "pico8-font-24";
        textStyle.color = SDL_Color{255, 255, 255, 255};

        context.DrawText(
            "Score: " + std::to_string(state.score),
            HudAnchor::TopCenter,
            {0.0f, 32.0f},
            textStyle
        );

        context.DrawText(
            state.gameOver ? "Game Over" : "Space: Jump",
            HudAnchor::BottomLeft,
            {24.0f, -32.0f},
            textStyle
        );
    }
};

#endif // FLAPPY_BIRD_HUD_H

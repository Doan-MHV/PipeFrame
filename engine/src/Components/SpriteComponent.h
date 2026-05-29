#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <string>
#include <SDL3/SDL.h>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct SpriteComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, PF::DisplayName("Texture"), PF::JsonName("texture_asset_id"))
    std::string assetId;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 4096, 1)
    int width;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 4096, 1)
    int height;

    PF_PROPERTY(PF::Edit, PF::Save, -10000, 10000, 1)
    int zIndex;

    PF_ENUM_PROPERTY(PF::Edit, PF::Save, "None", "Horizontal", "Vertical", "Horizontal + Vertical")
    SDL_FlipMode flip;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool isFixed;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f, PF::DisplayName("Source Rect"))
    SDL_FRect srcRect;

    SpriteComponent(
        std::string assetId = "",
        float width = 32,
        float height = 32,
        float zIndex = 1,
        bool isFixed = false,
        float srcRectX = 0,
        float srcRectY = 0,
        float srcRectWidth = 0,
        float srcRectHeight = 0
    )
    {
        this->assetId = assetId;
        this->width = width;
        this->height = height;
        this->zIndex = zIndex;
        this->flip = SDL_FLIP_NONE;
        this->isFixed = isFixed;
        this->srcRect = {
            srcRectX,
            srcRectY,
            srcRectWidth > 0 ? srcRectWidth : width,
            srcRectHeight > 0 ? srcRectHeight : height
        };
    }
};

#endif

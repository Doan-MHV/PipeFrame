

#ifndef PIPEFRAME_TEXTLABELCOMPONENT_H
#define PIPEFRAME_TEXTLABELCOMPONENT_H

#include <string>
#include <glm/vec2.hpp>
#include <SDL3/SDL_pixels.h>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct TextLabelComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 position;

    PF_PROPERTY(PF::Edit, PF::Save)
    std::string text;

    PF_PROPERTY(PF::Edit, PF::Save, PF::DisplayName("Font Asset"))
    std::string assetId;

    PF_PROPERTY(PF::Edit, PF::Save)
    SDL_Color color;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool isFixed;

    TextLabelComponent(glm::vec2 position = glm::vec2(0), const std::string& text = "", const std::string& assetId = "",
                       const SDL_Color& color = {0, 0, 0}, bool isFixed = true)
    {
        this->position = position;
        this->text = text;
        this->assetId = assetId;
        this->color = color;
        this->isFixed = isFixed;
    }
};

#endif //PIPEFRAME_TEXTLABELCOMPONENT_H



#ifndef PIPEFRAME_BOXCOLLIDERCOMPONENT_H
#define PIPEFRAME_BOXCOLLIDERCOMPONENT_H

#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("boxcollider"))
struct BoxColliderComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 1, 4096, 1)
    int width;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 4096, 1)
    int height;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 offset;

    PF_PROPERTY(PF::Edit, PF::Save, PF::DisplayName("Match Sprite Size"))
    bool matchSpriteSize;

    PF_PROPERTY(PF::Edit, PF::Save, PF::DisplayName("Rotate With Transform"))
    bool rotateWithTransform;

    BoxColliderComponent(
        int width = 10,
        int height = 10,
        glm::vec2 offset = glm::vec2(0),
        bool matchSpriteSize = false,
        bool rotateWithTransform = false
    )
    {
        this->width = width;
        this->height = height;
        this->offset = offset;
        this->matchSpriteSize = matchSpriteSize;
        this->rotateWithTransform = rotateWithTransform;
    }
};

#endif //PIPEFRAME_BOXCOLLIDERCOMPONENT_H

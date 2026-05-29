#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::NotAddable, PF::NotRemovable)
struct TransformComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 position;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 0.01f)
    glm::vec2 scale;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0, 0.0, 1.0)
    double rotation;

    TransformComponent(
        glm::vec2 position = glm::vec2(0, 0),
        glm::vec2 scale = glm::vec2(1, 1),
        double rotation = 0.0
    )
    {
        this->position = position;
        this->scale = scale;
        this->rotation = rotation;
    }
};

#endif

#ifndef PIPEFRAME_MOVEMENTCOMPONENT_H
#define PIPEFRAME_MOVEMENTCOMPONENT_H

#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("movement_control"))
struct MovementComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    bool enabled = true;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly)
    glm::vec2 previousPosition = glm::vec2(0.0f);

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly)
    bool hasPreviousPosition = false;

    MovementComponent(bool enabled = true)
    {
        this->enabled = enabled;
    }
};

#endif // PIPEFRAME_MOVEMENTCOMPONENT_H

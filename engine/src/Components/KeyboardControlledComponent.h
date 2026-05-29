

#ifndef PIPEFRAME_KEYBOARDCONTROLLEDCOMPONENT_H
#define PIPEFRAME_KEYBOARDCONTROLLEDCOMPONENT_H
#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("keyboard_controller"), PF::DisplayName("Keyboard Control"))
struct KeyboardControlledComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 upVelocity;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 rightVelocity;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 downVelocity;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 leftVelocity;

    KeyboardControlledComponent(
        glm::vec2 upVelocity = glm::vec2(0.0f, -50.0f),
        glm::vec2 rightVelocity = glm::vec2(50.0f, 0.0f),
        glm::vec2 downVelocity = glm::vec2(0.0f, 50.0f),
        glm::vec2 leftVelocity = glm::vec2(-50.0f, 0.0f)
    )
    {
        this->upVelocity = upVelocity;
        this->rightVelocity = rightVelocity;
        this->downVelocity = downVelocity;
        this->leftVelocity = leftVelocity;
    }
};

#endif //PIPEFRAME_KEYBOARDCONTROLLEDCOMPONENT_H

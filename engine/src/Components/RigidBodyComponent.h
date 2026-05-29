#ifndef RIGIDBODYCOMPONENT_H
#define RIGIDBODYCOMPONENT_H

#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("rigidbody"))
struct RigidBodyComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 velocity;

    RigidBodyComponent(glm::vec2 velocity = glm::vec2(0.0, 0.0))
    {
        this->velocity = velocity;
    }
};

#endif

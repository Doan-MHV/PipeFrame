#ifndef PIPEFRAME_RIGIDBODYCOMPONENT_H
#define PIPEFRAME_RIGIDBODYCOMPONENT_H

#include <glm/glm.hpp>

struct RigidBodyComponent
{
    glm::vec2 velocity;

    RigidBodyComponent(const glm::vec2 velocity = glm::vec2(0.0))
    {
        this->velocity = velocity;
    }
};

#endif //PIPEFRAME_RIGIDBODYCOMPONENT_H

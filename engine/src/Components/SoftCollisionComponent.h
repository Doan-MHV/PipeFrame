#ifndef PIPEFRAME_SOFTCOLLISIONCOMPONENT_H
#define PIPEFRAME_SOFTCOLLISIONCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct SoftCollisionComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 256.0f, 1.0f)
    float radius = 16.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 1.0f, 0.05f)
    float pushStrength = 0.65f;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool immovable = false;

    SoftCollisionComponent(
        float radius = 16.0f,
        float pushStrength = 0.65f,
        bool immovable = false
    )
    {
        this->radius = radius;
        this->pushStrength = pushStrength;
        this->immovable = immovable;
    }
};

#endif // PIPEFRAME_SOFTCOLLISIONCOMPONENT_H

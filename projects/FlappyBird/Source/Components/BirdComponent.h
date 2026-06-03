#ifndef BirdComponent_H
#define BirdComponent_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct BirdComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    float gravity = 900.0f;

    PF_PROPERTY(PF::Edit, PF::Save)
    float jumpVelocity = -360.0f;

    PF_PROPERTY(PF::Edit, PF::Save)
    float maxFallSpeed = 520.0f;

    PF_PROPERTY(PF::Edit, PF::Save)
    float rotationStrength = 0.05f;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool isAlive = true;
};

#endif // BirdComponent_H

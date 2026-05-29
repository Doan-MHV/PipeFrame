#ifndef PIPEFRAME_MOVEMENTSTATUSCOMPONENT_H
#define PIPEFRAME_MOVEMENTSTATUSCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::Hidden, PF::NotSerializable)
struct MovementStatusComponent
{
    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly)
    bool wasBlocked = false;

    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly)
    bool blockedX = false;

    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly)
    bool blockedY = false;

    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly)
    bool blockedByCollision = false;

    MovementStatusComponent(
        bool wasBlocked = false,
        bool blockedX = false,
        bool blockedY = false,
        bool blockedByCollision = false
    )
    {
        this->wasBlocked = wasBlocked;
        this->blockedX = blockedX;
        this->blockedY = blockedY;
        this->blockedByCollision = blockedByCollision;
    }
};

#endif // PIPEFRAME_MOVEMENTSTATUSCOMPONENT_H

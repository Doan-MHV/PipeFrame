#ifndef PIPEFRAME_DESTROYWHENOFFSCREENCOMPONENT_H
#define PIPEFRAME_DESTROYWHENOFFSCREENCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct DestroyWhenOffscreenComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 4096.0f, 1.0f)
    float margin = 128.0f;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool useCameraBounds = true;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool destroyLeft = true;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool destroyRight = true;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool destroyAbove = true;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool destroyBelow = true;

    DestroyWhenOffscreenComponent(
        float margin = 128.0f,
        bool useCameraBounds = true,
        bool destroyLeft = true,
        bool destroyRight = true,
        bool destroyAbove = true,
        bool destroyBelow = true
    )
    {
        this->margin = margin;
        this->useCameraBounds = useCameraBounds;
        this->destroyLeft = destroyLeft;
        this->destroyRight = destroyRight;
        this->destroyAbove = destroyAbove;
        this->destroyBelow = destroyBelow;
    }
};

#endif // PIPEFRAME_DESTROYWHENOFFSCREENCOMPONENT_H

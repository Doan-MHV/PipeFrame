#ifndef PIPEFRAME_HAZARDCOMPONENT_H
#define PIPEFRAME_HAZARDCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("hazard"))
struct HazardComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    bool deadly = true;

    HazardComponent(bool deadly = true)
    {
        this->deadly = deadly;
    }
};

#endif // PIPEFRAME_HAZARDCOMPONENT_H

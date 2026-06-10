#ifndef PipeComponent_H
#define PipeComponent_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct PipeComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    bool deadly = true;
};

#endif // PipeComponent_H

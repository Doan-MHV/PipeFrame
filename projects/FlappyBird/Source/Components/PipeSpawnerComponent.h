#ifndef PipeSpawnerComponent_H
#define PipeSpawnerComponent_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct PipeSpawnerComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)
    int value = 0;
};

#endif // PipeSpawnerComponent_H

#ifndef PIPEFRAME_EXAMPLECOMPONENT_H
#define PIPEFRAME_EXAMPLECOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct ExampleComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)
    int value = 10;
};

#endif // PIPEFRAME_EXAMPLECOMPONENT_H

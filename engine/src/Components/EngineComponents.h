#ifndef PIPEFRAME_ENGINECOMPONENTS_H
#define PIPEFRAME_ENGINECOMPONENTS_H

#include "Generated/EngineComponents.generated.h"

inline void RegisterEngineComponents(ComponentRegistry& registry)
{
    RegisterGeneratedEngineComponents(registry);
}

#endif // PIPEFRAME_ENGINECOMPONENTS_H

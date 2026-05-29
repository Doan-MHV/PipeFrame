

#ifndef PIPEFRAME_PERSISTENTIDCOMPONENT_H
#define PIPEFRAME_PERSISTENTIDCOMPONENT_H

#include <string>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::Hidden, PF::NotSerializable)
struct PersistentIdComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    std::string value;

    PersistentIdComponent(std::string value = "") : value(value)
    {
    }
};

#endif //PIPEFRAME_PERSISTENTIDCOMPONENT_H

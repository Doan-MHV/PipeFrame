#ifndef PIPEFRAME_TRIGGERCOMPONENT_H
#define PIPEFRAME_TRIGGERCOMPONENT_H

#include <string>
#include <utility>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::JsonName("trigger"))
struct TriggerComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    std::string action;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool once = true;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly)
    bool activated = false;

    TriggerComponent(std::string action = "", bool once = true)
    {
        this->action = std::move(action);
        this->once = once;
        this->activated = false;
    }
};

#endif // PIPEFRAME_TRIGGERCOMPONENT_H

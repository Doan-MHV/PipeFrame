#ifndef PIPEFRAME_ATTRIBUTESCOMPONENT_H
#define PIPEFRAME_ATTRIBUTESCOMPONENT_H

#include <nlohmann/json.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct AttributesComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    nlohmann::json values;

    AttributesComponent(nlohmann::json values = nlohmann::json::object())
    {
        this->values = values.is_object() ? values : nlohmann::json::object();
    }
};

#endif // PIPEFRAME_ATTRIBUTESCOMPONENT_H

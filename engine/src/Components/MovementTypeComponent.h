#ifndef PIPEFRAME_MOVEMENTTYPECOMPONENT_H
#define PIPEFRAME_MOVEMENTTYPECOMPONENT_H

#include <string>

#include "Reflection/ComponentAnnotations.h"

enum class MovementType
{
    Land,
    Water,
    Air
};

PF_COMPONENT(PF::Engine, PF::JsonName("movement"))
struct MovementTypeComponent
{
    PF_ENUM_PROPERTY(PF::Edit, PF::Save, "land", "water", "air")
    MovementType type;

    MovementTypeComponent(MovementType type = MovementType::Land)
    {
        this->type = type;
    }
};

inline std::string MovementTypeToString(MovementType type)
{
    switch (type)
    {
    case MovementType::Land:
        return "land";
    case MovementType::Water:
        return "water";
    case MovementType::Air:
        return "air";
    }

    return "land";
}

inline MovementType ParseMovementType(const std::string& value)
{
    if (value == "water")
    {
        return MovementType::Water;
    }

    if (value == "air")
    {
        return MovementType::Air;
    }

    return MovementType::Land;
}

#endif //PIPEFRAME_MOVEMENTTYPECOMPONENT_H

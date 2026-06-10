#ifndef LevelExitComponent_H
#define LevelExitComponent_H

#include <string>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct LevelExitComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    std::string targetLevel = "Level2.json";
};

#endif // LevelExitComponent_H

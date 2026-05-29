#ifndef PIPEFRAME_FOOD_SOURCE_COMPONENT_H
#define PIPEFRAME_FOOD_SOURCE_COMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct FoodSourceComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 100000, 1)
    int foodAmount = 25;
};

#endif // PIPEFRAME_FOOD_SOURCE_COMPONENT_H

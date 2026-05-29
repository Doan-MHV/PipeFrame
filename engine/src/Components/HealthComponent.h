

#ifndef PIPEFRAME_HEALTHCOMPONENT_H
#define PIPEFRAME_HEALTHCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct HealthComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1, PF::DisplayName("Health"))
    int healthPercentage;

    HealthComponent(int healthPercentage = 0)
    {
        this->healthPercentage = healthPercentage;
    }
};

#endif //PIPEFRAME_HEALTHCOMPONENT_H

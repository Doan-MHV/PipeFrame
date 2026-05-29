

#ifndef PIPEFRAME_PROJECTILECOMPONENT_H
#define PIPEFRAME_PROJECTILECOMPONENT_H
#include <SDL3/SDL_timer.h>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::Hidden, PF::NotSerializable)
struct ProjectileComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    bool isFriendly;

    PF_PROPERTY(PF::Edit, PF::Save, 0, 1000, 1, PF::DisplayName("Damage"))
    int hitPercentDamage;

    PF_PROPERTY(PF::Edit, PF::Save, 0, 60000, 100)
    int duration;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly, 0, 0, 1)
    int startTime;

    ProjectileComponent(bool isFriendly = false, int hitPercentDamage = 0, int duration = 0)
    {
        this->isFriendly = isFriendly;
        this->hitPercentDamage = hitPercentDamage;
        this->duration = duration;
        this->startTime = SDL_GetTicks();
    }
};

#endif //PIPEFRAME_PROJECTILECOMPONENT_H

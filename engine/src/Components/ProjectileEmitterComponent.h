

#ifndef PIPEFRAME_PROJECTILEEMITTERCOMPONENT_H
#define PIPEFRAME_PROJECTILEEMITTERCOMPONENT_H
#include <glm/vec2.hpp>
#include <SDL3/SDL_timer.h>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::NotAddable, PF::NotRemovable, PF::Hidden)
struct ProjectileEmitterComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 projectileVelocity;

    PF_SECONDS_PROPERTY(PF::Edit, PF::Save, 0.0f, 60.0f, 0.1f)
    int repeatFrequency;

    PF_SECONDS_PROPERTY(PF::Edit, PF::Save, 0.0f, 60.0f, 0.1f)
    int projectileDuration;

    PF_PROPERTY(PF::Edit, PF::Save, 0, 1000, 1, PF::JsonName("hit_percentage_damage"), PF::DisplayName("Damage"))
    int hitPercentDamage;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool isFriendly;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly, 0, 0, 1)
    int lastEmissionTime;

    ProjectileEmitterComponent(glm::vec2 projectileVelocity = glm::vec2(0), int repeatFrequency = 0,
                               int projectileDuration = 10000, int hitPercentDamage = 10, bool isFriendly = false)
    {
        this->projectileVelocity = projectileVelocity;
        this->repeatFrequency = repeatFrequency;
        this->projectileDuration = projectileDuration;
        this->hitPercentDamage = hitPercentDamage;
        this->isFriendly = isFriendly;
        this->lastEmissionTime = SDL_GetTicks();
    }
};

#endif //PIPEFRAME_PROJECTILEEMITTERCOMPONENT_H

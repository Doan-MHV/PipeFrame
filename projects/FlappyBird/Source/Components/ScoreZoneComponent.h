#ifndef FLAPPY_BIRD_SCORE_ZONE_COMPONENT_H
#define FLAPPY_BIRD_SCORE_ZONE_COMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct ScoreZoneComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 1, 100, 1)
    int scoreValue = 1;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool scored = false;
};

#endif // FLAPPY_BIRD_SCORE_ZONE_COMPONENT_H

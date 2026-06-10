#ifndef FLAPPY_BIRD_SCORE_COMPONENT_H
#define FLAPPY_BIRD_SCORE_COMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct ScoreComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 1000000, 1)
    int score = 0;

    PF_PROPERTY(PF::Edit, PF::Save, 0, 1000000, 1)
    int bestScore = 0;
};

#endif // FLAPPY_BIRD_SCORE_COMPONENT_H

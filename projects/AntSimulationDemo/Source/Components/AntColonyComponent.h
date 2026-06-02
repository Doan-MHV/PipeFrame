#ifndef PIPEFRAME_ANT_COLONY_COMPONENT_H
#define PIPEFRAME_ANT_COLONY_COMPONENT_H

#include <glm/glm.hpp>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct AntColonyComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 10000, 1)
    int maxAnts = 100;

    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly, 0, 10000, 1)
    int spawnedAnts = 0;

    PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly, 0, 100000, 1)
    int storedFood = 0;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0, 10.0, 0.01)
    double spawnInterval = 0.05;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly, 0.0, 10.0, 0.01)
    double spawnTimer = 0.05;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 0.0f, 1.0f)
    glm::vec2 spawnOffset = glm::vec2(32.0f, 0.0f);
};

#endif // PIPEFRAME_ANT_COLONY_COMPONENT_H

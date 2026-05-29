#ifndef PIPEFRAME_ENTITYIDGENERATOR_H
#define PIPEFRAME_ENTITYIDGENERATOR_H

#include <string>

#include "Components/PersistentIdComponent.h"
#include "ECS/ECS.h"

inline std::string BuildUniqueEntityId(Registry& registry, const std::string& baseName)
{
    std::string candidate = baseName;
    int index = 1;

    bool isUnique = false;
    while (!isUnique)
    {
        isUnique = true;

        for (Entity entity : registry.GetAllEntities())
        {
            if (entity.HasComponent<PersistentIdComponent>() &&
                entity.GetComponent<PersistentIdComponent>().value == candidate)
            {
                isUnique = false;
                break;
            }
        }

        if (!isUnique)
        {
            candidate = baseName + "_" + std::to_string(index);
            index++;
        }
    }

    return candidate;
}

#endif // PIPEFRAME_ENTITYIDGENERATOR_H

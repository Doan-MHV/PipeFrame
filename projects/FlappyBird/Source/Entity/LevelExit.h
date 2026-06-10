#ifndef LevelExit_H
#define LevelExit_H

#include <glm/glm.hpp>

#include "Components/TransformComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct LevelExit
{
    static Entity Create(Registry& registry, glm::vec2 position)
    {
        Entity entity = registry.CreateEntity();
        entity.AddComponent<EditorEntityComponent>();
        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "LevelExit"));
        entity.AddComponent<TransformComponent>(position);
        return entity;
    }
};

#endif // LevelExit_H

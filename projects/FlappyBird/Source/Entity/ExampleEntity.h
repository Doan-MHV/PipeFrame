#ifndef PIPEFRAME_EXAMPLEENTITY_H
#define PIPEFRAME_EXAMPLEENTITY_H

#include <glm/glm.hpp>

#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct ExampleEntity
{
    static Entity Create(Registry& registry, glm::vec2 position)
    {
        Entity entity = registry.CreateEntity();
        entity.AddComponent<EditorEntityComponent>();
        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "ExampleEntity"));
        entity.AddComponent<TransformComponent>(position);
        return entity;
    }
};

#endif // PIPEFRAME_EXAMPLEENTITY_H

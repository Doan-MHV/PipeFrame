#ifndef PIPEFRAME_COLONY_H
#define PIPEFRAME_COLONY_H

#include <glm/glm.hpp>

#include "Components/AntColonyComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

inline Entity CreateColonyEntity(Registry& registry, glm::vec2 position)
{
    Entity entity = registry.CreateEntity();
    entity.Group("colonies");
    entity.AddComponent<EditorEntityComponent>();
    entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "colony"));
    entity.AddComponent<TransformComponent>(position);
    entity.AddComponent<SpriteComponent>(
        "marker-texture",
        48,
        48,
        1,
        false,
        100.0f,
        0.0f,
        100.0f,
        100.0f
    );
    entity.AddComponent<BoxColliderComponent>(48, 48);
    entity.AddComponent<AttributesComponent>();
    entity.GetComponent<AttributesComponent>().values["stored_food"] = 0;
    entity.AddComponent<AntColonyComponent>();
    return entity;
}

#endif // PIPEFRAME_COLONY_H

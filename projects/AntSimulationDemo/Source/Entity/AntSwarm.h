#ifndef PIPEFRAME_ANT_SWARM_H
#define PIPEFRAME_ANT_SWARM_H

#include <glm/glm.hpp>

#include "Components/AntSwarmComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

inline Entity CreateAntSwarmEntity(Registry& registry, glm::vec2 position)
{
    Entity entity = registry.CreateEntity();
    entity.Group("swarms");
    entity.AddComponent<EditorEntityComponent>();
    entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "ant_swarm"));
    entity.AddComponent<TransformComponent>(position);
    entity.AddComponent<SpriteComponent>(
        "marker-texture",
        40,
        40,
        1,
        false,
        0.0f,
        0.0f,
        100.0f,
        100.0f
    );
    entity.AddComponent<BoxColliderComponent>(40, 40);
    entity.AddComponent<AntSwarmComponent>();
    return entity;
}

#endif // PIPEFRAME_ANT_SWARM_H

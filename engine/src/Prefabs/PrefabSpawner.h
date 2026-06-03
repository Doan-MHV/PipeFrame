#ifndef PIPEFRAME_PREFABSPAWNER_H
#define PIPEFRAME_PREFABSPAWNER_H

#include <string>

#include <glm/glm.hpp>

#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Prefabs/PrefabRegistry.h"
#include "Project/EntityIdGenerator.h"
#include "Simulation/ProjectRuntime.h"

struct PrefabSpawnOptions
{
    std::string persistentIdPrefix;
    glm::vec2 position = glm::vec2(0.0f);
    bool overridePosition = true;
};

inline Entity SpawnPrefab(
    ProjectRuntimeContext& context,
    const std::string& prefabId,
    const PrefabSpawnOptions& options = {}
)
{
    const std::string idPrefix = options.persistentIdPrefix.empty()
        ? prefabId
        : options.persistentIdPrefix;

    Entity entity = context.prefabRegistry.InstantiatePrefab(
        prefabId,
        context.registry,
        BuildUniqueEntityId(context.registry, idPrefix),
        &context.componentRegistry
    );

    if (entity.GetId() < 0)
    {
        return entity;
    }

    if (options.overridePosition && entity.HasComponent<TransformComponent>())
    {
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.position = options.position;
    }

    return entity;
}

inline Entity SpawnPrefab(
    ProjectRuntimeContext& context,
    const std::string& prefabId,
    glm::vec2 position
)
{
    PrefabSpawnOptions options;
    options.position = position;
    return SpawnPrefab(context, prefabId, options);
}

#endif // PIPEFRAME_PREFABSPAWNER_H

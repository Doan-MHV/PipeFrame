#include "Core/EditorEntityActions.h"

#include <nlohmann/json.hpp>

#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/TransformComponent.h"
#include "Core/EditorViewModels.h"
#include "ECS/ECS.h"
#include "Game/EntitySerializer.h"
#include "Game/LevelLoader.h"
#include "Prefabs/PrefabRegistry.h"
#include "Reflection/EditorMetadata.h"

Entity CreateEditorEntity(
    const std::unique_ptr<Registry>& registry,
    const SDL_FRect& camera
)
{
    if (!registry)
    {
        return Entity(-1);
    }

    Entity entity = registry->CreateEntity();
    entity.AddComponent<EditorEntityComponent>();
    entity.AddComponent<PersistentIdComponent>(
        BuildUniquePersistentId(registry, "entity_" + std::to_string(entity.GetId()))
    );
    entity.AddComponent<TransformComponent>(GetViewportCenterWorldPosition(camera));
    return entity;
}

Entity DuplicateEditorEntity(
    const std::unique_ptr<Registry>& registry,
    Entity sourceEntity,
    const ComponentRegistry& componentRegistry,
    const SDL_FRect& camera
)
{
    if (!registry || sourceEntity.GetId() < 0)
    {
        return Entity(-1);
    }

    nlohmann::json entityJson = EntitySerializer::SerializeEntity(
        sourceEntity,
        &componentRegistry
    );
    const std::string baseName = entityJson.value("id", GetEntityDisplayName(sourceEntity)) + "_copy";
    entityJson["id"] = BuildUniquePersistentId(registry, baseName);

    if (entityJson.contains("components") &&
        entityJson["components"].contains("transform") &&
        entityJson["components"]["transform"].is_object())
    {
        auto& transform = entityJson["components"]["transform"];
        const glm::vec2 viewportCenter = GetViewportCenterWorldPosition(camera);
        transform["position"]["x"] = viewportCenter.x;
        transform["position"]["y"] = viewportCenter.y;
    }

    LevelLoader loader;
    return loader.LoadEntityFromJson(entityJson, registry, &componentRegistry);
}

bool DeleteEditorEntity(Entity entity)
{
    if (entity.GetId() < 0)
    {
        return false;
    }

    entity.Kill();
    return true;
}

Entity CreateEntityFromPrefab(
    const std::unique_ptr<Registry>& registry,
    const PrefabRegistry& prefabRegistry,
    const ComponentRegistry& componentRegistry,
    const std::string& prefabId,
    const SDL_FRect& camera
)
{
    if (!registry || prefabId.empty())
    {
        return Entity(-1);
    }

    const std::string persistentId = BuildUniquePersistentId(registry, prefabId);
    Entity entity = prefabRegistry.InstantiatePrefab(
        prefabId,
        registry,
        persistentId,
        &componentRegistry
    );

    if (entity.GetId() >= 0 && entity.HasComponent<TransformComponent>())
    {
        entity.GetComponent<TransformComponent>().position = GetViewportCenterWorldPosition(camera);
    }

    return entity;
}

Entity CreateEntityFromProjectClass(
    const std::unique_ptr<Registry>& registry,
    const ClassRegistry& classRegistry,
    const std::string& classTypeName,
    const SDL_FRect& camera
)
{
    if (!registry || classTypeName.empty())
    {
        return Entity(-1);
    }

    return classRegistry.CreateEntity(
        classTypeName,
        *registry,
        GetViewportCenterWorldPosition(camera)
    );
}

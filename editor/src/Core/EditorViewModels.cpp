#include "Core/EditorViewModels.h"

#include <algorithm>

#include "Assets/AssetRegistry.h"
#include "Components/PersistentIdComponent.h"
#include "ECS/ECS.h"
#include "Prefabs/PrefabRegistry.h"
#include "Reflection/EditorMetadata.h"

Entity FindEntityById(const std::unique_ptr<Registry>& registry, int entityId)
{
    if (!registry)
    {
        return Entity(-1);
    }

    for (Entity entity : registry->GetAllEntities())
    {
        if (entity.GetId() == entityId)
        {
            return entity;
        }
    }

    return Entity(-1);
}

std::string GetEntityDisplayName(Entity entity)
{
    if (entity.GetId() < 0)
    {
        return "None";
    }

    if (entity.HasComponent<PersistentIdComponent>())
    {
        return entity.GetComponent<PersistentIdComponent>().value;
    }

    return "entity_" + std::to_string(entity.GetId());
}

bool IsPersistentIdUnique(const std::unique_ptr<Registry>& registry, const std::string& persistentId)
{
    if (!registry)
    {
        return true;
    }

    for (Entity entity : registry->GetAllEntities())
    {
        if (!entity.HasComponent<PersistentIdComponent>())
        {
            continue;
        }

        if (entity.GetComponent<PersistentIdComponent>().value == persistentId)
        {
            return false;
        }
    }

    return true;
}

std::string BuildUniquePersistentId(const std::unique_ptr<Registry>& registry, const std::string& baseName)
{
    if (IsPersistentIdUnique(registry, baseName))
    {
        return baseName;
    }

    int index = 1;
    std::string candidate;

    do
    {
        candidate = baseName + "_" + std::to_string(index);
        index++;
    }
    while (!IsPersistentIdUnique(registry, candidate));

    return candidate;
}

glm::vec2 GetViewportCenterWorldPosition(const SDL_FRect& camera)
{
    return glm::vec2(
        camera.x + camera.w * 0.5f,
        camera.y + camera.h * 0.5f
    );
}

std::vector<EditorEntityRow> BuildEntityRows(
    const std::unique_ptr<Registry>& registry,
    int selectedEntityId
)
{
    std::vector<EditorEntityRow> rows;

    if (!registry)
    {
        return rows;
    }

    for (Entity entity : registry->GetAllEntities())
    {
        EditorEntityRow row;
        row.entityId = entity.GetId();
        row.name = GetEntityDisplayName(entity);
        row.tag = entity.registry->GetEntityTag(entity);
        row.group = entity.registry->GetEntityGroup(entity);
        row.selected = row.entityId == selectedEntityId;
        row.label = row.name;

        if (!row.tag.empty())
        {
            row.label += " [" + row.tag + "]";
        }
        else if (!row.group.empty())
        {
            row.label += " [" + row.group + "]";
        }

        rows.push_back(std::move(row));
    }

    return rows;
}

EditorContentSnapshot BuildContentSnapshot(
    const AssetRegistry* assetRegistry,
    const PrefabRegistry* prefabRegistry,
    const ClassRegistry* classRegistry
)
{
    EditorContentSnapshot snapshot;

    if (assetRegistry)
    {
        for (const std::string& textureId : assetRegistry->GetTextureIds())
        {
            EditorTextureAssetRow row;
            row.id = textureId;

            if (const TextureInfo* info = assetRegistry->GetTextureInfo(textureId))
            {
                row.filePath = info->filePath;
                row.spriteSheet = info->sprite.mode == TextureSpriteMode::SpriteSheet;
                row.defaultDisplayWidth = info->sprite.defaultDisplayWidth;
                row.defaultDisplayHeight = info->sprite.defaultDisplayHeight;
            }

            snapshot.textures.push_back(std::move(row));
        }
    }

    if (prefabRegistry)
    {
        for (const std::string& prefabId : prefabRegistry->GetPrefabIds())
        {
            EditorPrefabRow row;
            row.id = prefabId;

            if (const PrefabDefinition* prefab = prefabRegistry->GetPrefab(prefabId))
            {
                const nlohmann::json& prefabJson = prefab->entityJson;
                row.tag = prefabJson.value("tag", "");
                row.group = prefabJson.value("group", "");

                if (prefabJson.contains("components") && prefabJson["components"].is_object())
                {
                    for (auto it = prefabJson["components"].begin(); it != prefabJson["components"].end(); ++it)
                    {
                        row.components.push_back(it.key());
                    }

                    std::sort(row.components.begin(), row.components.end());
                }
            }

            snapshot.prefabs.push_back(std::move(row));
        }
    }

    if (classRegistry)
    {
        for (const EntityClassMetadata& metadata : classRegistry->GetEntityClasses())
        {
            snapshot.projectClasses.push_back(
                EditorProjectClassRow{
                    metadata.typeName,
                    metadata.displayName,
                    metadata.category
                }
            );
        }
    }

    return snapshot;
}

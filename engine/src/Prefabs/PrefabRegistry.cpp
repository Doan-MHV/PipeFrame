#include "PrefabRegistry.h"

#include <algorithm>
#include <fstream>

#include "Game/LevelLoader.h"
#include "Logger/Logger.h"

bool PrefabRegistry::LoadFromDirectory(const std::filesystem::path& prefabDirectory)
{
    prefabs.clear();

    if (!std::filesystem::exists(prefabDirectory))
    {
        Logger::Warn("Prefab directory does not exist: " + prefabDirectory.string());
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(prefabDirectory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
        {
            continue;
        }

        std::ifstream input(entry.path());
        if (!input)
        {
            Logger::Err("Failed to open prefab file: " + entry.path().string());
            continue;
        }

        nlohmann::json prefabJson;
        try
        {
            input >> prefabJson;
        }
        catch (const std::exception& exception)
        {
            Logger::Err(
                "Failed to parse prefab file: " +
                entry.path().string() +
                " error: " +
                exception.what()
            );
            continue;
        }

        if (prefabJson.contains("entity") && prefabJson["entity"].is_object())
        {
            prefabJson = prefabJson["entity"];
        }

        if (!prefabJson.is_object() ||
            !prefabJson.contains("components") ||
            !prefabJson["components"].is_object())
        {
            Logger::Err("Skipping invalid prefab file: " + entry.path().string());
            continue;
        }

        PrefabDefinition prefab;
        prefab.id = prefabJson.value("id", entry.path().stem().string());
        prefab.entityJson = prefabJson;

        prefabs[prefab.id] = prefab;
        Logger::Log("Loaded prefab: " + prefab.id);
    }

    return true;
}

std::vector<std::string> PrefabRegistry::GetPrefabIds() const
{
    std::vector<std::string> ids;
    ids.reserve(prefabs.size());

    for (const auto& prefab : prefabs)
    {
        ids.push_back(prefab.first);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

bool PrefabRegistry::HasPrefab(const std::string& prefabId) const
{
    return prefabs.contains(prefabId);
}

const PrefabDefinition* PrefabRegistry::GetPrefab(const std::string& prefabId) const
{
    const auto prefab = prefabs.find(prefabId);
    if (prefab == prefabs.end())
    {
        return nullptr;
    }

    return &prefab->second;
}

Entity PrefabRegistry::InstantiatePrefab(
    const std::string& prefabId,
    const std::unique_ptr<Registry>& registry,
    const std::string& persistentId
) const
{
    return InstantiatePrefab(prefabId, *registry, persistentId, nullptr);
}

Entity PrefabRegistry::InstantiatePrefab(
    const std::string& prefabId,
    const std::unique_ptr<Registry>& registry,
    const std::string& persistentId,
    const ComponentRegistry* componentRegistry
) const
{
    return InstantiatePrefab(prefabId, *registry, persistentId, componentRegistry);
}

Entity PrefabRegistry::InstantiatePrefab(
    const std::string& prefabId,
    Registry& registry,
    const std::string& persistentId
) const
{
    return InstantiatePrefab(prefabId, registry, persistentId, nullptr);
}

Entity PrefabRegistry::InstantiatePrefab(
    const std::string& prefabId,
    Registry& registry,
    const std::string& persistentId,
    const ComponentRegistry* componentRegistry
) const
{
    const auto prefab = prefabs.find(prefabId);

    if (prefab == prefabs.end())
    {
        Logger::Err("Cannot instantiate missing prefab: " + prefabId);
        return Entity(-1);
    }

    nlohmann::json entityJson = prefab->second.entityJson;
    entityJson["id"] = persistentId.empty() ? prefabId : persistentId;

    LevelLoader loader;
    return loader.LoadEntityFromJson(entityJson, registry, componentRegistry);
}

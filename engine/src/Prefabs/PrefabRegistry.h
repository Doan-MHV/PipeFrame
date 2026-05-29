#ifndef PIPEFRAME_PREFABREGISTRY_H
#define PIPEFRAME_PREFABREGISTRY_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "ECS/Entity.h"

class Registry;
class ComponentRegistry;

struct PrefabDefinition
{
    std::string id;
    nlohmann::json entityJson;
};

class PrefabRegistry
{
private:
    std::unordered_map<std::string, PrefabDefinition> prefabs;

public:
    bool LoadFromDirectory(const std::filesystem::path& prefabDirectory);
    std::vector<std::string> GetPrefabIds() const;
    bool HasPrefab(const std::string& prefabId) const;
    const PrefabDefinition* GetPrefab(const std::string& prefabId) const;

    Entity InstantiatePrefab(
        const std::string& prefabId,
        const std::unique_ptr<Registry>& registry,
        const std::string& persistentId
    ) const;

    Entity InstantiatePrefab(
        const std::string& prefabId,
        const std::unique_ptr<Registry>& registry,
        const std::string& persistentId,
        const ComponentRegistry* componentRegistry
    ) const;

    Entity InstantiatePrefab(
        const std::string& prefabId,
        Registry& registry,
        const std::string& persistentId
    ) const;

    Entity InstantiatePrefab(
        const std::string& prefabId,
        Registry& registry,
        const std::string& persistentId,
        const ComponentRegistry* componentRegistry
    ) const;
};

#endif // PIPEFRAME_PREFABREGISTRY_H

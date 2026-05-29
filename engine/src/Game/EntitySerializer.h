

#ifndef PIPEFRAME_ENTITYSERIALIZER_H
#define PIPEFRAME_ENTITYSERIALIZER_H

#include <memory>
#include <nlohmann/json.hpp>

#include "ECS/ECS.h"

class ComponentRegistry;

class EntitySerializer
{
public:
    static nlohmann::json SerializeEntity(Entity entity);
    static nlohmann::json SerializeEntity(
        Entity entity,
        const ComponentRegistry* componentRegistry
    );
    static nlohmann::json SerializeEntities(const std::unique_ptr<Registry>& registry);
    static nlohmann::json SerializeEntities(
        const std::unique_ptr<Registry>& registry,
        const ComponentRegistry* componentRegistry
    );
    static bool SaveEntities(const std::unique_ptr<Registry>& registry, const std::string& filePath);
    static bool SaveEntities(
        const std::unique_ptr<Registry>& registry,
        const std::string& filePath,
        const ComponentRegistry* componentRegistry
    );
};


#endif //PIPEFRAME_ENTITYSERIALIZER_H

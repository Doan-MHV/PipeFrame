#ifndef PIPEFRAME_ENTITYFACTORY_H
#define PIPEFRAME_ENTITYFACTORY_H

#include <string>

#include <glm/glm.hpp>

#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct SpriteEntityOptions
{
    std::string idPrefix = "sprite";
    std::string textureAssetId;
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 scale = glm::vec2(1.0f);
    int width = 32;
    int height = 32;
    int zIndex = 1;
    bool isFixed = false;
    bool save = true;
};

inline Entity CreateSpriteEntity(Registry& registry, const SpriteEntityOptions& options)
{
    Entity entity = registry.CreateEntity();
    entity.AddComponent<EditorEntityComponent>(options.save);
    entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, options.idPrefix));
    entity.AddComponent<TransformComponent>(options.position, options.scale, 0.0);
    entity.AddComponent<SpriteComponent>(
        options.textureAssetId,
        options.width,
        options.height,
        options.zIndex,
        options.isFixed
    );

    return entity;
}

#endif // PIPEFRAME_ENTITYFACTORY_H

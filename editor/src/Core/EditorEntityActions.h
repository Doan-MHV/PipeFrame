#ifndef PIPEFRAME_EDITORENTITYACTIONS_H
#define PIPEFRAME_EDITORENTITYACTIONS_H

#include <memory>
#include <string>

#include <SDL3/SDL_rect.h>

class ComponentRegistry;
class Entity;
class Registry;
class PrefabRegistry;
class ClassRegistry;

Entity CreateEditorEntity(
    const std::unique_ptr<Registry>& registry,
    const SDL_FRect& camera
);

Entity DuplicateEditorEntity(
    const std::unique_ptr<Registry>& registry,
    Entity sourceEntity,
    const ComponentRegistry& componentRegistry,
    const SDL_FRect& camera
);

bool DeleteEditorEntity(Entity entity);

Entity CreateEntityFromPrefab(
    const std::unique_ptr<Registry>& registry,
    const PrefabRegistry& prefabRegistry,
    const ComponentRegistry& componentRegistry,
    const std::string& prefabId,
    const SDL_FRect& camera
);

Entity CreateEntityFromProjectClass(
    const std::unique_ptr<Registry>& registry,
    const ClassRegistry& classRegistry,
    const std::string& classTypeName,
    const SDL_FRect& camera
);

#endif // PIPEFRAME_EDITORENTITYACTIONS_H

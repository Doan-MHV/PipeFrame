#ifndef PIPEFRAME_EDITORVIEWMODELS_H
#define PIPEFRAME_EDITORVIEWMODELS_H

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <SDL3/SDL_rect.h>

class AssetRegistry;
class Registry;
class PrefabRegistry;
class ClassRegistry;
class Entity;

struct EditorEntityRow
{
    int entityId = -1;
    std::string name;
    std::string tag;
    std::string group;
    std::string label;
    bool selected = false;
};

struct EditorPrefabRow
{
    std::string id;
    std::string tag;
    std::string group;
    std::vector<std::string> components;
};

struct EditorProjectClassRow
{
    std::string typeName;
    std::string displayName;
    std::string category;
};

struct EditorTextureAssetRow
{
    std::string id;
    std::string filePath;
    bool spriteSheet = false;
    int defaultDisplayWidth = 32;
    int defaultDisplayHeight = 32;
};

struct EditorContentSnapshot
{
    std::vector<EditorTextureAssetRow> textures;
    std::vector<EditorPrefabRow> prefabs;
    std::vector<EditorProjectClassRow> projectClasses;
};

Entity FindEntityById(const std::unique_ptr<Registry>& registry, int entityId);
std::string GetEntityDisplayName(Entity entity);
bool IsPersistentIdUnique(const std::unique_ptr<Registry>& registry, const std::string& persistentId);
std::string BuildUniquePersistentId(const std::unique_ptr<Registry>& registry, const std::string& baseName);
glm::vec2 GetViewportCenterWorldPosition(const SDL_FRect& camera);

std::vector<EditorEntityRow> BuildEntityRows(
    const std::unique_ptr<Registry>& registry,
    int selectedEntityId
);

EditorContentSnapshot BuildContentSnapshot(
    const AssetRegistry* assetRegistry,
    const PrefabRegistry* prefabRegistry,
    const ClassRegistry* classRegistry
);

#endif // PIPEFRAME_EDITORVIEWMODELS_H

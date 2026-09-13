#ifndef PIPEFRAME_SCENE_DOCUMENT_H
#define PIPEFRAME_SCENE_DOCUMENT_H

#include "SceneTypes.h"

#include <optional>
#include <string>
#include <vector>

namespace pipeframe::editor {

class SceneDocument {
public:
    const std::vector<SceneObjectData> &
    GetObjects() const;
    const SceneSettings &GetSettings() const;
    bool SetSettings(SceneSettings settings);
    const std::vector<SceneConnectionData> &GetConnections() const;
    bool AddConnection(SceneConnectionData connection);
    bool RemoveConnection(std::uint64_t connectionId);

    const SceneObjectData *FindObject(
        SceneObjectId objectId) const;

    std::vector<const SceneObjectData *> GetChildren(SceneObjectId parentId) const;
    SceneTransform GetWorldTransform(SceneObjectId objectId) const;

    SceneObjectId CreateObject(
        std::string name,
        SceneObjectTypeId typeId,
        SceneTransform transform = {},
        PropertyMap properties = {});

    SceneObjectId CreateObject(SceneObjectData object);

    bool RestoreObject(SceneObjectData object);

    bool RemoveObject(SceneObjectId objectId);

    bool RenameObject(SceneObjectId objectId, std::string name);
    bool SetParent(SceneObjectId objectId, SceneObjectId parentId, std::int32_t siblingOrder = -1);
    bool SetLayer(SceneObjectId objectId, std::string layer);
    bool SetTags(SceneObjectId objectId, std::vector<std::string> tags);
    bool SetVisible(SceneObjectId objectId, bool visible);
    bool SetLocked(SceneObjectId objectId, bool locked);
    bool AddComponent(SceneObjectId objectId, SceneComponentData component);
    bool RemoveComponent(SceneObjectId objectId, const std::string &componentTypeId);
    bool SetComponentProperty(SceneObjectId objectId, const std::string &componentTypeId,
                              std::string key, PropertyValue value);
    bool ReplaceObjectData(SceneObjectId objectId, SceneObjectData replacement);
    bool SetPrefabLinks(SceneObjectId objectId, std::vector<PrefabInstanceLink> links);
    SceneObjectId DuplicateObject(SceneObjectId objectId, bool includeChildren = true);
    std::vector<SceneObjectId> FindObjects(const std::string &query = {},
                                           const std::string &layer = {},
                                           const std::string &tag = {},
                                           bool includeHidden = true) const;
    std::vector<std::string> Validate() const;

    bool SetTransform(
        SceneObjectId objectId,
        SceneTransform transform);

    bool SetProperty(
        SceneObjectId objectId,
        std::string key,
        PropertyValue value);

    bool RemoveProperty(
        SceneObjectId objectId,
        const std::string &key);

    const PropertyValue *FindProperty(
        SceneObjectId objectId,
        const std::string &key) const;

    void Clear();

    bool IsDirty() const;

    void MarkClean();

private:
    SceneObjectData *FindMutableObject(
        SceneObjectId objectId);

    bool IsDescendant(SceneObjectId candidate, SceneObjectId ancestor) const;
    SceneObjectId DuplicateRecursive(SceneObjectId objectId, SceneObjectId newParentId);

    std::vector<SceneObjectData> objects;
    SceneSettings settings;
    std::vector<SceneConnectionData> connections;

    SceneObjectId nextObjectId = 1;
    bool dirty = false;
};

} // namespace pipeframe::editor

#endif

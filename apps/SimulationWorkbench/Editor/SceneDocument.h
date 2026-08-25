#ifndef PIPEFRAME_SCENE_DOCUMENT_H
#define PIPEFRAME_SCENE_DOCUMENT_H

#include "SceneTypes.h"

#include <string>
#include <vector>

namespace pipeframe::editor {

class SceneDocument {
  public:
    const std::vector<SceneObjectData> &GetObjects() const;

    const SceneObjectData *FindObject(SceneObjectId objectId) const;

    SceneObjectId CreateObject(std::string name, SceneObjectType type, SceneTransform transform);

    bool RestoreObject(SceneObjectData object);

    bool RemoveObject(SceneObjectId objectId);

    bool SetTransform(SceneObjectId objectId, SceneTransform transform);

    void Clear();

    bool IsDirty() const;
    void MarkClean();

  private:
    std::vector<SceneObjectData> objects;

    SceneObjectId nextObjectId = 1;
    bool dirty = false;
};

} // namespace pipeframe::editor

#endif

#include "SceneDocument.h"

#include <algorithm>
#include <utility>

namespace pipeframe::editor {

const std::vector<SceneObjectData> &SceneDocument::GetObjects() const { return objects; }

const SceneObjectData *SceneDocument::FindObject(SceneObjectId objectId) const {
    const auto iterator = std::find_if(objects.begin(), objects.end(),
                                       [objectId](const SceneObjectData &object) { return object.id == objectId; });

    return iterator != objects.end() ? &*iterator : nullptr;
}

SceneObjectId SceneDocument::CreateObject(std::string name, SceneObjectType type, SceneTransform transform) {
    const SceneObjectId objectId = nextObjectId++;

    objects.push_back({
        objectId,
        std::move(name),
        type,
        transform,
    });

    dirty = true;

    return objectId;
}

bool SceneDocument::RestoreObject(SceneObjectData object) {
    if (FindObject(object.id) != nullptr) {
        return false;
    }

    nextObjectId = std::max(nextObjectId, object.id + 1);

    objects.push_back(std::move(object));
    dirty = true;

    return true;
}

bool SceneDocument::RemoveObject(SceneObjectId objectId) {
    const std::size_t removedCount =
        std::erase_if(objects, [objectId](const SceneObjectData &object) { return object.id == objectId; });

    if (removedCount == 0) {
        return false;
    }

    dirty = true;

    return true;
}

bool SceneDocument::SetTransform(SceneObjectId objectId, SceneTransform transform) {
    auto iterator = std::find_if(objects.begin(), objects.end(),
                                 [objectId](const SceneObjectData &object) { return object.id == objectId; });

    if (iterator == objects.end()) {
        return false;
    }

    if (iterator->transform.position == transform.position && iterator->transform.rotation == transform.rotation) {
        return false;
    }

    iterator->transform = transform;
    dirty = true;

    return true;
}

void SceneDocument::Clear() {
    if (objects.empty()) {
        return;
    }

    objects.clear();
    nextObjectId = 1;
    dirty = true;
}

bool SceneDocument::IsDirty() const { return dirty; }

void SceneDocument::MarkClean() { dirty = false; }

} // namespace pipeframe::editor
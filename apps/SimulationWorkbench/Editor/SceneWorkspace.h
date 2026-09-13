#ifndef PIPEFRAME_SCENE_WORKSPACE_H
#define PIPEFRAME_SCENE_WORKSPACE_H

#include "SceneDocument.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pipeframe::editor {

using SceneId = std::uint64_t;

struct WorkspaceScene {
    SceneId id{};
    std::string name;
    SceneDocument document;
    bool loaded{false};
};

class SceneWorkspace final {
public:
    SceneId CreateScene(std::string name, SceneDocument initial = {});
    SceneId DuplicateScene(SceneId sourceId, std::string name);
    bool RemoveScene(SceneId sceneId);
    bool SetActiveScene(SceneId sceneId);
    bool SetAdditivelyLoaded(SceneId sceneId, bool loaded);
    std::optional<SceneId> GetActiveSceneId() const;
    const WorkspaceScene *FindScene(SceneId sceneId) const;
    WorkspaceScene *FindScene(SceneId sceneId);
    const std::vector<WorkspaceScene> &GetScenes() const;
    std::vector<const WorkspaceScene *> GetLoadedScenes() const;

private:
    SceneId nextId{1};
    std::optional<SceneId> activeSceneId;
    std::vector<WorkspaceScene> scenes;
};

} // namespace pipeframe::editor
#endif

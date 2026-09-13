#include "SceneWorkspace.h"

#include <algorithm>
#include <ranges>
#include <utility>

namespace pipeframe::editor {

SceneId SceneWorkspace::CreateScene(std::string name, SceneDocument initial) {
    if (name.empty() || std::ranges::any_of(scenes, [&](const auto &scene) { return scene.name == name; })) return 0;
    const SceneId id = nextId++;
    scenes.push_back({id, std::move(name), std::move(initial), !activeSceneId.has_value()});
    if (!activeSceneId) activeSceneId = id;
    return id;
}

SceneId SceneWorkspace::DuplicateScene(const SceneId sourceId, std::string name) {
    const auto *source = FindScene(sourceId);
    if (!source) return 0;
    return CreateScene(std::move(name), source->document);
}

bool SceneWorkspace::RemoveScene(const SceneId sceneId) {
    if (activeSceneId == sceneId) return false;
    return std::erase_if(scenes, [=](const auto &scene) { return scene.id == sceneId; }) > 0;
}

bool SceneWorkspace::SetActiveScene(const SceneId sceneId) {
    auto *scene = FindScene(sceneId);
    if (!scene || activeSceneId == sceneId) return false;
    activeSceneId = sceneId;
    scene->loaded = true;
    return true;
}

bool SceneWorkspace::SetAdditivelyLoaded(const SceneId sceneId, const bool loaded) {
    auto *scene = FindScene(sceneId);
    if (!scene || (!loaded && activeSceneId == sceneId) || scene->loaded == loaded) return false;
    scene->loaded = loaded;
    return true;
}

std::optional<SceneId> SceneWorkspace::GetActiveSceneId() const { return activeSceneId; }

const WorkspaceScene *SceneWorkspace::FindScene(const SceneId id) const {
    const auto scene = std::ranges::find(scenes, id, &WorkspaceScene::id);
    return scene == scenes.end() ? nullptr : &*scene;
}

WorkspaceScene *SceneWorkspace::FindScene(const SceneId id) {
    const auto scene = std::ranges::find(scenes, id, &WorkspaceScene::id);
    return scene == scenes.end() ? nullptr : &*scene;
}

const std::vector<WorkspaceScene> &SceneWorkspace::GetScenes() const { return scenes; }

std::vector<const WorkspaceScene *> SceneWorkspace::GetLoadedScenes() const {
    std::vector<const WorkspaceScene *> result;
    for (const auto &scene : scenes) if (scene.loaded) result.push_back(&scene);
    return result;
}

} // namespace pipeframe::editor

#pragma once
#include <PipeFrame/ECS/Scene.h>

#include <span>
#include <unordered_map>
#include <vector>

namespace pipeframe {
// Index of borrowed views, never a second component owner. Rebuilt after external
// structural changes. Returned spans/references expire at the next cache rebuild.
template <class View, class Tag>
class SceneViewCache {
public:
    explicit SceneViewCache(BehaviourScene& scene) : scene(scene) {}
    BehaviourScene& GetScene() const { return scene; }
    ecs::World& GetWorld() const { return scene.Components(); }
    View& Track(SceneObject object) {
        // Instantiate may relocate components, but existing views keep only handles.
        indices[object.GetEntity()] = views.size();
        views.emplace_back(object);
        revision = GetWorld().StructureRevision();
        return views.back();
    }
    View* Find(ecs::Entity id) const {
        Refresh();
        const auto it = indices.find(id);
        return it == indices.end() ? nullptr : &views[it->second];
    }
    std::span<View> Items() const {
        Refresh();
        return views;
    }
    std::size_t Size() const { return GetWorld().template Components<Tag>().size(); }
    template <class Predicate>
    std::size_t RemoveWhere(Predicate predicate) {
        std::vector<ecs::Entity> removed;
        for (const auto& view : Items())
            if (predicate(view)) removed.push_back(view.GetId());
        for (auto id : removed)
            scene.Destroy(id);
        return removed.size();
    }
    void Clear() {
        for (auto id : GetWorld().template EntitiesWith<Tag>())
            scene.Destroy(id);
        Refresh();
    }
    void Refresh() const {
        if (revision == GetWorld().StructureRevision()) return;
        views.clear();
        indices.clear();
        for (auto id : GetWorld().template EntitiesWith<Tag>()) {
            indices[id] = views.size();
            views.emplace_back(scene.GetObject(id));
        }
        revision = GetWorld().StructureRevision();
    }

private:
    BehaviourScene& scene;
    mutable std::uint64_t revision{~std::uint64_t{0}};
    mutable std::vector<View> views;
    mutable std::unordered_map<ecs::Entity, std::size_t> indices;
};
}  // namespace pipeframe

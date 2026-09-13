#pragma once
#include <PipeFrame/ECS/Scene.h>
#include <tuple>
namespace pipeframe {
// Borrowed, typed scene access with lazy pointer caching. No component copies.
// Structural changes force re-resolution; stale entities/scenes throw before
// pointers are read. A view instance is thread-confined; separate copies can be
// used by workers under the world's no-structural-mutation batch contract.
template<class... Components> class ComponentView {
public:
    explicit ComponentView(SceneObject object) : object(object) {}
    SceneObject GetObject() const { return object; }
    ecs::Entity GetEntity() const { ValidateCache(); return object.entity; }
    template<class T> T &Require() const {
        ValidateCache();
        auto *&pointer = std::get<T *>(pointers);
        if (!pointer) pointer = object.GetComponent<T>();
        if (!pointer) throw std::logic_error("Required scene component is missing");
        return *pointer;
    }
protected:
    SceneObject object;
private:
    void ValidateCache() const {
        // Only structural edits can invalidate an entity or relocate components.
        // Check scene lifetime every time, and entity generation once per version.
        const auto next = object.StorageVersion();
        if (next == revision) return;
        if (!object.IsValid()) throw std::logic_error("Scene object no longer exists");
        pointers = {}; revision = next;
    }
    mutable std::tuple<Components *...> pointers{};
    mutable std::uint64_t revision{~std::uint64_t{0}};
};
}

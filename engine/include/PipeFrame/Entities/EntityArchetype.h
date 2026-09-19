#pragma once
#include <PipeFrame/ECS/World.h>

namespace pipeframe {
class SceneObject;
// Reusable object recipe. Build adds components to a newly allocated identity.
// A failed build removes the entire partial object. External side effects in
// Build remain the responsibility of the recipe author.
class EntityArchetype {
public:
    virtual ~EntityArchetype() = default;
    // Called only by scene instantiation, after component construction.
    virtual void OnInstantiated(const SceneObject&) const {}
    ecs::Entity Instantiate(ecs::World& world) const {
        const auto entity = world.Create();
        try {
            Build(world, entity);
        } catch (...) {
            world.Destroy(entity);
            throw;
        }
        return entity;
    }

protected:
    virtual void Build(ecs::World& world, ecs::Entity entity) const = 0;
};
// Data-only recipes need no project-specific subclass or lifecycle boilerplate.
template <class... Components>
class ComponentEntity final : public EntityArchetype {
    void Build(ecs::World& world, ecs::Entity entity) const override { (world.Add<Components>(entity), ...); }
};
}  // namespace pipeframe

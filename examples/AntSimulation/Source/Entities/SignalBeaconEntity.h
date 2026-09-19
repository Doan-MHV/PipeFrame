#pragma once
#include "Behaviours/SignalBeaconBehaviour.h"
#include <PipeFrame/Entities/EntityArchetype.h>
namespace ant_simulation {
class SignalBeaconEntity final : public pipeframe::EntityArchetype {
  public:
    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity entity) const override {
        world.Add<pipeframe::Transform2DComponent>(entity);
        world.Add<SignalBeaconComponent>(entity);
    }
    void OnInstantiated(const pipeframe::SceneObject &object) const override { object.Attach<SignalBeaconBehaviour>(); }
};
} // namespace ant_simulation

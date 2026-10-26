#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Entities/EntityArchetype.h>

#include "../Components/InfraredSensorComponent.h"

class InfraredSensorEntity final : public pipeframe::EntityArchetype {
protected:
  void Build(pipeframe::ecs::World& world, pipeframe::ecs::Entity entity) const override {
    world.Add<pipeframe::Transform2DComponent>(entity);
    world.Add<InfraredSensorComponent>(entity);
  }
};

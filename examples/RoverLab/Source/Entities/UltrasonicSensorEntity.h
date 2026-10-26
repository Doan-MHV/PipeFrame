#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Entities/EntityArchetype.h>

#include "../Components/UltrasonicSensorComponent.h"

class UltrasonicSensorEntity final : public pipeframe::EntityArchetype {
protected:
    void Build(pipeframe::ecs::World& world, pipeframe::ecs::Entity entity) const override {
        // Compose this object with world.Add<YourComponent>(entity).
        world.Add<pipeframe::Transform2DComponent>(entity);
        world.Add<UltrasonicSensorComponent>(entity);
    }
};

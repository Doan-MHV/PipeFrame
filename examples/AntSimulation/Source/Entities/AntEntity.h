#pragma once
#include <PipeFrame/Entities/EntityArchetype.h>
#include "World/Runtime/AntView.h"

namespace ant_simulation {
// Runtime spawning and future editor recipes share this composition point.
class AntEntity final : public pipeframe::EntityArchetype {
public:
    AntEntity(ColonyId colony, AntRole role, pipeframe::Vector2f position,
              float angle, float markerOffset, const AntConfiguration &configuration)
        : colony(colony), role(role), position(position), angle(angle),
          markerOffset(markerOffset), configuration(configuration) {}
    void OnInstantiated(const pipeframe::SceneObject &object) const override {
        AntView(object).Initialize(object.GetEntity(), colony, role, position, angle, markerOffset, configuration);
    }
public:
    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity entity) const override {
        world.Add<AntIdentityComponent>(entity);
        world.Add<pipeframe::Transform2DComponent>(entity);
        world.Add<pipeframe::Motion2DComponent>(entity);
        world.Add<AntPoseComponent>(entity);
        world.Add<pipeframe::EnergyComponent>(entity);
        world.Add<ForagingComponent>(entity);
        world.Add<AntEncounterComponent>(entity);
    }
    ColonyId colony;
    AntRole role;
    pipeframe::Vector2f position;
    float angle, markerOffset;
    AntConfiguration configuration;
};
}

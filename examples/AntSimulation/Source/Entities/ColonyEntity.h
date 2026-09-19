#pragma once
#include "Components/ColonySettingsComponent.h"
#include "World/Runtime/ColonyView.h"
#include <PipeFrame/Components/RandomStateComponent.h>
#include <PipeFrame/Entities/EntityArchetype.h>
namespace ant_simulation {
class ColonyEntity final : public pipeframe::EntityArchetype {
  public:
    ColonyEntity(ColonyId id, pipeframe::Vector2f position, pipeframe::Color color,
                 const AntConfiguration &configuration, std::size_t samples = 60)
        : id(id), position(position), color(color), configuration(configuration), samples(samples) {}
    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity entity) const override {
        world.Add<ColonyStateComponent>(entity);
        world.Add<ColonyHistoryComponent>(entity);
        world.Add<pipeframe::Transform2DComponent>(entity);
        world.Add<ColonySettingsComponent>(entity);
        world.Add<pipeframe::RandomStateComponent>(entity);
    }
    void OnInstantiated(const pipeframe::SceneObject &object) const override {
        ColonyView(object).Initialize(id, position, color, configuration, samples);
        auto &settings = *object.GetComponent<ColonySettingsComponent>();
        settings.population = configuration.colonyInitialAntCount;
        settings.radius = configuration.colonyRadius;
        settings.speed = configuration.antSpeed;
        settings.red = color.r;
        settings.green = color.g;
        settings.blue = color.b;
    }

  private:
    ColonyId id;
    pipeframe::Vector2f position;
    pipeframe::Color color;
    AntConfiguration configuration;
    std::size_t samples;
};
} // namespace ant_simulation

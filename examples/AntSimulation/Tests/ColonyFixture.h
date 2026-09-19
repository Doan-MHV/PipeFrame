#pragma once
#include "Entities/ColonyEntity.h"
namespace ant_simulation {
struct ColonyFixtureScene {
    pipeframe::BehaviourScene scene;
};
class ColonyFixture : private ColonyFixtureScene, public ColonyView {
  public:
    ColonyFixture(ColonyId id, pipeframe::Vector2f position, pipeframe::Color color,
                  const AntConfiguration &configuration, std::size_t samples = 60)
        : ColonyView(scene.Instantiate(ColonyEntity(id, position, color, configuration, samples))) {}
};
} // namespace ant_simulation

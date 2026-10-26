#pragma once
#include "Entities/AntEntity.h"
namespace ant_simulation {
struct AntFixtureScene {
    pipeframe::BehaviourScene scene;
};
class AntFixture : private AntFixtureScene, public AntView {
    static pipeframe::SceneObject Create(pipeframe::BehaviourScene &scene, AntId id, ColonyId colony, AntRole role,
                                         pipeframe::Vector2f position, float angle, float markerOffset,
                                         const AntConfiguration &configuration) {
        scene.Components().Create(id);
        const AntEntity recipe(colony, role, position, angle, markerOffset, configuration);
        recipe.Build(scene.Components(), id);
        const auto object = scene.GetObject(id);
        recipe.OnInstantiated(object);
        return object;
    }

  public:
    AntFixture(AntId id, ColonyId colony, AntRole role, pipeframe::Vector2f position, float angle, float markerOffset,
               const AntConfiguration &configuration)
        : AntView(Create(scene, id, colony, role, position, angle, markerOffset, configuration)) {}
};
} // namespace ant_simulation

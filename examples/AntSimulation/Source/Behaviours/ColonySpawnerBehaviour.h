#ifndef ANT_COLONY_BEHAVIOUR_H
#define ANT_COLONY_BEHAVIOUR_H
#include <PipeFrame/ECS/Scene.h>
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
namespace ant_simulation {
// Attached to each colony entity. PipeFrame dispatches this hook and resolves
// the component by identity, so dense storage relocation is safe.
class ColonySpawnerBehaviour final : public pipeframe::Behaviour {
public:
    explicit ColonySpawnerBehaviour(ColonyLifecycleSystem &system) : system(system) {}
    void FixedUpdate(float delta) override {
        auto colony = ColonyView(GetObject());
        system.UpdateColony(colony, delta);
    }
    void OnDestroy() override {
        if (auto *colony = GetComponent<ColonyStateComponent>()) system.OnColonyDestroyed(colony->id);
    }
private:
    ColonyLifecycleSystem &system;
};
}
#endif

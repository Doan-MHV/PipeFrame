#include "Runtime/AntRegistration.h"
#include "World/AntWorld.h"

#include "Components/ColonySettingsComponent.h"
#include "Components/FoodSourceComponent.h"
#include "Components/SignalBeaconComponent.h"
#include "Components/SimulationSettingsComponent.h"
#include "Entities/SignalBeaconEntity.h"
#include "Runtime/AntTypeIds.h"
#include <array>

namespace ant_simulation {
std::vector<pipeframe::SceneComponentTypeDescriptor> CreateAntComponentTypes() {
    return {ColonySettingsComponent::Schema().Describe(), FoodSourceComponent::Schema().Describe(),
            SimulationSettingsComponent::Schema().Describe(), SignalBeaconComponent::Schema().Describe()};
}

} // namespace ant_simulation

#include "Components/AntEncounterComponent.h"
#include "Components/AntIdentityComponent.h"
#include "Components/AntPoseComponent.h"
#include "Components/ColonyHistoryComponent.h"
#include "Components/ColonyStateComponent.h"
#include "Components/ForagingComponent.h"
#include <PipeFrame/Components/CommonComponentSchemas.h>
namespace ant_simulation {
void BindAntFactories(pipeframe::EntityRegistry &registry, AntWorld &world) {
    registry.BindFactory(
        ColonyTypeId,
        [&world](pipeframe::BehaviourScene &, const pipeframe::SceneObjectData &data) {
            world.CreateColony(data.id, data.transform.position, {239, 71, 111});
            return world.GetColonyLifecycleSystem().GetColonyObject(data.id);
        },
        [&world](pipeframe::SceneObject, const pipeframe::SceneObjectData &data) {
            world.GetColonyLifecycleSystem().ApplySettings(data.id, true);
        });
}
void RegisterAntComponents(pipeframe::ComponentRegistry &registry) {
    pipeframe::RegisterCommonComponents(registry);
    registry.Register(pipeframe::PlaygroundComponent::Schema());
    registry.Register(pipeframe::TilemapComponent::Schema());
    registry.Register(ColonySettingsComponent::Schema());
    registry.Register(SignalBeaconComponent::Schema());
    registry.Register(FoodSourceComponent::Schema());
    registry.Register(SimulationSettingsComponent::Schema());
    registry.Register(ColonyStateComponent::Schema());
    registry.Register(AntIdentityComponent::Schema());
    registry.Register(ForagingComponent::Schema());
    registry.Register(AntPoseComponent::Schema());
    registry.Register(AntEncounterComponent::Schema());
    registry.Register(ColonyHistoryComponent::Schema());
}
} // namespace ant_simulation

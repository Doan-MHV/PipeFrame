#ifndef ANT_AUTHORING_H
#define ANT_AUTHORING_H

#include "Components/FoodSourceComponent.h"
#include "Components/SimulationSettingsComponent.h"
#include "Entities/AntEntity.h"
#include "Entities/ColonyEntity.h"
#include "Entities/SignalBeaconEntity.h"
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Project/ComponentRegistry.h>
#include <PipeFrame/Project/EntityRegistry.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <vector>

namespace ant_simulation {

inline const pipeframe::EntityRegistry &AntEntityTypes() {
    static const auto types = [] {
        pipeframe::EntityRegistry registry;
        registry.Register<AntEntity>({"ant.agent", "ANT", {}, {}}, false);
        registry.Register<ColonyEntity>(
            {"ant.colony", "ANT COLONY", ColonySettingsComponent::Schema().Describe().properties, {"ant.colony"}});
        registry.Register<SignalBeaconEntity>({SignalBeaconTypeId,
                                               "SIGNAL BEACON",
                                               SignalBeaconComponent::Schema().Describe().properties,
                                               {SignalBeaconTypeId}});
        registry.Register<pipeframe::ComponentEntity<pipeframe::Transform2DComponent, FoodSourceComponent>>(
            {FoodSourceTypeId, "FOOD SOURCE", FoodSourceComponent::Schema().Describe().properties, {FoodSourceTypeId}});
        registry.Register<pipeframe::ComponentEntity<pipeframe::Transform2DComponent, SimulationSettingsComponent>>(
            {SimulationSettingsTypeId,
             "SIMULATION SETTINGS",
             SimulationSettingsComponent::Schema().Describe().properties,
             {SimulationSettingsTypeId}});
        registry.Register<pipeframe::ComponentEntity<pipeframe::Transform2DComponent, pipeframe::PlaygroundComponent,
                                                     pipeframe::TilemapComponent>>(
            {pipeframe::PlaygroundEntityTypeId,
             "PLAYGROUND",
             {},
             {pipeframe::PlaygroundComponentTypeId, pipeframe::TilemapComponentTypeId}});
        return registry;
    }();
    return types;
}
inline void RegisterAntEntities(pipeframe::EntityRegistry &registry) { registry = AntEntityTypes(); }

class AntWorld;
void BindAntFactories(pipeframe::EntityRegistry &registry, AntWorld &world);

void RegisterAntComponents(pipeframe::ComponentRegistry &registry);

// These descriptors are the public authoring surface of the Ant project.
// The workbench builds its Add Object menu and Inspector from this metadata.
std::vector<pipeframe::SceneComponentTypeDescriptor> CreateAntComponentTypes();

} // namespace ant_simulation
#endif

#include "Integration/AntProject.h"
#if __has_include("Integration/GeneratedRegistration.h")
#include "Integration/GeneratedRegistration.h"
#define PIPEFRAME_ANT_GENERATED_REGISTRATION 1
#endif
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>

#include "Components/AntEncounterComponent.h"
#include "Components/AntIdentityComponent.h"
#include "Components/AntPoseComponent.h"
#include "Components/ColonyHistoryComponent.h"
#include "Components/ColonyStateComponent.h"
#include "Components/FoodSourceComponent.h"
#include "Components/ForagingComponent.h"
#include "Components/SimulationSettingsComponent.h"
#include "Editor/AntFoodBrush.h"
#include "Entities/AntEntity.h"
#include "Entities/ColonyEntity.h"
#include "Entities/SignalBeaconEntity.h"
#include "Integration/AntTypeIds.h"
#include "World/AntWorld.h"
#include "World/Physics/AntPhysicsWorld.h"
#include "World/Simulation/AntSimulationWorld.h"
#include "World/Simulation/Systems/AntSystemIds.h"

namespace ant_simulation {
using namespace pipeframe;

void AntProject::Configure(ProjectDefinition<AntWorld>& project) {
  project.CommonComponents()
    .Components<PlaygroundComponent, TilemapComponent>()
    .AuthoringComponents<ColonySettingsComponent,
      FoodSourceComponent,
      SimulationSettingsComponent,
      SignalBeaconComponent>()
    .Components<ColonyStateComponent,
      AntIdentityComponent,
      ForagingComponent,
      AntPoseComponent,
      AntEncounterComponent,
      ColonyHistoryComponent>();

  project.Entity<AntEntity>({"ant.agent", "ANT", {}, {}}, false)
    .Entity<ColonyEntity>({ColonyTypeId,
      "ANT COLONY",
      ColonySettingsComponent::Schema().Describe().properties,
      {ColonyTypeId}})
    .Entity<SignalBeaconEntity>({SignalBeaconTypeId,
      "SIGNAL BEACON",
      SignalBeaconComponent::Schema().Describe().properties,
      {SignalBeaconTypeId}})
    .Entity<ComponentEntity<Transform2DComponent, FoodSourceComponent>>({FoodSourceTypeId,
      "FOOD SOURCE",
      FoodSourceComponent::Schema().Describe().properties,
      {FoodSourceTypeId}})
    .Entity<ComponentEntity<Transform2DComponent, SimulationSettingsComponent>>(
      {SimulationSettingsTypeId,
        "SIMULATION SETTINGS",
        SimulationSettingsComponent::Schema().Describe().properties,
        {SimulationSettingsTypeId}})
    .Entity<ComponentEntity<Transform2DComponent, TilemapComponent>>(
      {MapEntityTypeId, "MAP", {}, {Transform2DComponentTypeId, TilemapComponentTypeId}})
    .Entity<ComponentEntity<Transform2DComponent, PlaygroundComponent, TilemapComponent>>(
      {PlaygroundEntityTypeId,
        "PLAYGROUND",
        {},
        {PlaygroundComponentTypeId, TilemapComponentTypeId}});

  project.EntityFactory(
    ColonyTypeId,
    [](AntWorld& world, BehaviourScene&, const SceneObjectData& data) {
      world.CreateColony(data.id, data.transform.position, {239, 71, 111});
      return world.GetColonyLifecycleSystem().GetColonyObject(data.id);
    },
    [](AntWorld& world, SceneObject, const SceneObjectData& data) {
      world.GetColonyLifecycleSystem().ApplySettings(data.id, true);
    });
  project.Brush<AntFoodBrush>("Ant");

#ifdef PIPEFRAME_ANT_GENERATED_REGISTRATION
  pipeframe_generated::Register(project);
#endif

  // World modules register their systems. These descriptors expose the same
  // ordered phases to the editor's plugin scheduler and standalone runners.
  project
    .Phase({.id = contract::ColonyLifecycleSystem,
             .phase = SystemPhase::FixedPrePhysics,
             .reads = {ColonyTypeId, FoodSourceTypeId},
             .writes = {ColonyTypeId, FoodSourceTypeId},
             .requiredServices = {std::string(SpatialServiceId),
               std::string(EventServiceId),
               std::string(RandomServiceId),
               std::string(JobServiceId),
               std::string(LogServiceId),
               std::string(ProfilingServiceId)}},
      &AntWorld::Prepare)
    .Phase({.id = contract::MovementSystem,
             .phase = SystemPhase::FixedPhysics,
             .dependencies = {contract::ColonyLifecycleSystem},
             .reads = {ColonyTypeId},
             .writes = {ColonyTypeId},
             .requiredServices = {std::string(PhysicsServiceId),
               std::string(SpatialServiceId),
               std::string(ProfilingServiceId)}},
      &AntWorld::Move)
    .Phase({.id = contract::BehaviorSystem,
             .phase = SystemPhase::FixedBehavior,
             .dependencies = {contract::MovementSystem},
             .reads = {ColonyTypeId},
             .writes = {ColonyTypeId, FoodSourceTypeId},
             .requiredServices = {std::string(RandomServiceId),
               std::string(EventServiceId),
               std::string(ProfilingServiceId)}},
      &AntWorld::Forage)
    .Phase({.id = contract::CleanupSystem,
             .phase = SystemPhase::FixedCleanup,
             .dependencies = {contract::BehaviorSystem},
             .reads = {ColonyTypeId},
             .writes = {ColonyTypeId},
             .requiredServices = {std::string(PhysicsServiceId),
               std::string(EventServiceId),
               std::string(ProfilingServiceId)}},
      &AntWorld::Cleanup);
}

const ProjectDefinition<AntWorld>& AntProject::Definition() {
  static const auto definition = [] {
    ProjectDefinition<AntWorld> project;
    Configure(project);
    return project;
  }();
  return definition;
}
const EntityRegistry& AntProject::EntityTypes() {
  static const auto types = [] {
    ComponentRegistry components;
    EntityRegistry entities;
    Definition().Install(components, entities);
    return entities;
  }();
  return types;
}
}  // namespace ant_simulation

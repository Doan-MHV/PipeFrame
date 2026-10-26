#pragma once
#include <PipeFrame/World/World.h>

#include "World/Simulation/AntQuery.h"
#include "World/Simulation/Environment/AntEnvironment.h"
#include "World/Simulation/Systems/AntCleanupSystem.h"
#include "World/Simulation/Systems/AntForagingSystem.h"
#include "World/Simulation/Systems/ColonyLifecycleSystem.h"
namespace ant_simulation {
class AntSimulationWorld final : public pipeframe::SimulationWorld {
public:
  AntSimulationWorld(const AntConfiguration& configuration, std::uint32_t seed)
    : ants(GetScene()),
      colonies(environment, ants, configuration, seed),
      foraging(ants, colonies, environment, configuration, seed) {
    using pipeframe::UpdatePhase;
    AddSystem(environment, UpdatePhase::BeforeScene);
    AddSystem(colonies, UpdatePhase::BeforePhysics);
    AddSystem(foraging, UpdatePhase::AfterPhysics);
  }
  void BindPhysics(AntBodySystem& bodies, const AntConfiguration& configuration) {
    if (cleanup) throw std::logic_error("Ant physics is already bound");
    cleanup = std::make_unique<AntCleanupSystem>(ants, colonies, bodies, configuration.worldSize);
    AddSystem(*cleanup, pipeframe::UpdatePhase::Cleanup, [this](AntCleanupResult result) {
      lastCleanupResult = result;
    });
  }
  AntEnvironment environment;
  AntQuery ants;
  ColonyLifecycleSystem colonies;
  AntForagingSystem foraging;
  std::unique_ptr<AntCleanupSystem> cleanup;
  AntCleanupResult lastCleanupResult;
};
}  // namespace ant_simulation

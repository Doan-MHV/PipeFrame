#include "World/Runtime/Systems/AntForagingSystem.h"

namespace ant_simulation {
AntForagingSystem::AntForagingSystem(AntQuery &store, ColonyLifecycleSystem &colonies,
                                    AntEnvironment &environment, const AntConfiguration &config,
                                    std::uint32_t seed)
    : store(store), colonies(colonies), worker(environment, config), random(seed) {}

void AntForagingSystem::Update(float delta) {
    const auto energyAccess = store.GetWorld().BorrowComponents<pipeframe::EnergyComponent>();
    const auto encounters = store.GetWorld().BorrowComponents<AntEncounterComponent>();
    const auto foragingAccess = store.GetWorld().BorrowComponents<ForagingComponent>();
    for (AntView &ant : store.GetAnts()) {
        auto *energy = energyAccess.Get(ant.GetId());
        auto *encounter = encounters.Get(ant.GetId());
        if (!energy || !encounter || energy->IsDepleted() || encounter->opponentId) continue;
        auto *colony = colonies.FindColony(ant.GetColonyId());
        if (!colony) { energy->Deplete(); continue; }
        auto *foraging = foragingAccess.Get(ant.GetId());
        if (foraging && ant.GetRole() != AntRole::Soldier)
            worker.Update(ant, *foraging, *colony, delta, random);
    }
}
} // namespace ant_simulation

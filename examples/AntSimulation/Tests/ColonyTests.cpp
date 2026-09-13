#include "ColonyFixture.h"
#include "World/Runtime/AntView.h"
#include "Components/AntIdentityComponent.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/ColonyView.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/Marker.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool NearlyEqual(const float first, const float second, const float tolerance = 0.0001f) {
    return std::abs(first - second) <= tolerance;
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    configuration.worldSize = {
        32,
        32,
    };

    configuration.colonyPosition = {
        16.0f,
        16.0f,
    };

    configuration.colonyInitialAntCount = 3;
    configuration.explorerProbability = 1.0f;

    AntEnvironment environment;
    std::string errorMessage;

    Require(environment.Initialize(configuration, errorMessage), "Colony environment should initialize.");

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    ColonyLifecycleSystem colonySystem(environment, antStore, configuration, 42);

    ColonyView &colony = colonySystem.CreateColony(100, {16.0f, 16.0f},
                                               pipeframe::Color{
                                                   80,
                                                   120,
                                                   220,
                                               });

    Require(colony.GetId() == 100, "Colony should retain its stable ID.");

    Require(NearlyEqual(colony.GetRadius(), configuration.colonyRadius), "Colony should begin with configured radius.");

    Require(NearlyEqual(colony.GetReserve(), 3.0f * configuration.antCost),
            "Initial reserve should fund the configured population.");

    const AntWorldCell *homeCell = environment.TryGetCell(16, 16);

    Require(homeCell != nullptr, "Colony home cell should exist.");

    const Marker &homeMarker = homeCell->GetMarker(MarkerKind::ToHome);

    Require(homeMarker.persistent, "Initial colony marker should be persistent.");

    Require(homeMarker.colonyId == colony.GetId(), "Initial home marker should belong to its colony.");

    Require(homeMarker.intensity == std::numeric_limits<float>::max(),
            "Initial home marker should use permanent maximum intensity.");

    Require(colony.AcquireNameSuffix(5) == 1, "First duplicate name should use suffix one.");

    Require(colony.AcquireNameSuffix(5) == 2, "Second duplicate name should use suffix two.");

    antStore.GetScene().FixedUpdate(1.0f / 60.0f);
    colonySystem.Update(1.0f / 60.0f);

    Require(antStore.GetCount() == 1, "Colony should create at most one ant per update.");

    const AntView firstAnt = antStore.GetAnts().front();

    Require(firstAnt.GetColonyId() == colony.GetId(), "Spawned ant should belong to its colony.");

    Require(firstAnt.GetRole() == AntRole::Explorer, "Explorer probability one should create explorers.");

    Require(firstAnt.Identity().color == colony.GetColor(), "Spawned ant should use colony color.");

    Require(firstAnt.Identity().nameIndex < ColonyLifecycleSystem::AntNameCount, "Spawned ant name index should be valid.");

    Require(firstAnt.Identity().nameSuffix >= 1, "Spawned ant should have a display-name suffix.");

    Require(NearlyEqual(colony.GetReserve(), 2.0f * configuration.antCost), "Spawning should consume one ant cost.");

    antStore.GetScene().FixedUpdate(1.0f / 60.0f);
    colonySystem.Update(1.0f / 60.0f);

    antStore.GetScene().FixedUpdate(1.0f / 60.0f);
    colonySystem.Update(1.0f / 60.0f);

    Require(antStore.GetCount() == 3, "Funded colony should create its configured ants.");

    antStore.GetScene().FixedUpdate(1.0f / 60.0f);
    colonySystem.Update(1.0f / 60.0f);

    Require(antStore.GetCount() == 3, "Empty reserve should stop spawning.");

    Require(colony.GetAntCount() == 3, "Colony should count its living ants.");

    Require(NearlyEqual(colony.GetRadius(), configuration.colonyRadius + 3.0f * 0.002f),
            "Colony radius should grow with population.");

    colony.AddFood(5.0f);

    Require(NearlyEqual(colony.GetFoodQuantity(), 5.0f), "Delivered food should update colony statistics.");

    Require(NearlyEqual(colony.GetReserve(), 5.0f), "Delivered food should replenish colony reserve.");

    ColonyFixture rateTestColony(999, {0.0f, 0.0f}, pipeframe::Color::White, configuration, 2);

    rateTestColony.UpdateCollectionRate();
    rateTestColony.AddFood(2.0f);
    rateTestColony.UpdateCollectionRate();

    Require(NearlyEqual(rateTestColony.GetCollectionRate(), 2.0f),
            "Collection rate should measure food gained in its sample window.");

    static_assert(std::is_nothrow_move_constructible_v<AntView>);
    Require(antStore.GetWorld().Components<pipeframe::EnergyComponent>().size() == antStore.GetCount(),
            "Every scene ant must have a separately queryable EnergyComponent component.");
    const AntId firstId = antStore.GetAnts()[0].GetId();

    const AntId secondId = antStore.GetAnts()[1].GetId();

    Require(firstId != secondId, "Ant store should generate unique stable IDs.");

    AntView alias = antStore.GetAnts()[0];
    const auto snapshotEnergy = alias.GetEnergyComponent();
    const auto snapshotEncounter = alias.GetEncounterComponent();
    const auto snapshotForaging = alias.GetForagingComponent();
    auto *firstEnergy = antStore.GetWorld().Get<pipeframe::EnergyComponent>(firstId);
    const float previousEnergy = firstEnergy->current;
    firstEnergy->Consume(0.25f);
    Require(antStore.Find(firstId)->GetEnergy() == firstEnergy->current && snapshotEnergy.current == previousEnergy,
            "ECS energy is authoritative while copied Ant values are detached snapshots.");
    Require(antStore.GetWorld().Components<AntEncounterComponent>().size() == antStore.GetCount(),
            "Each scene ant owns a separately queryable encounter component.");
    auto *encounter = antStore.GetWorld().Get<AntEncounterComponent>(firstId);
    encounter->opponentId = secondId;
    encounter->enemyTimer = 2.0f;
    Require(antStore.Find(firstId)->IsInEncounter() && !snapshotEncounter.opponentId,
            "Live encounter edits are authoritative; copied ants remain detached.");
    antStore.Find(firstId)->EndEncounter();
    Require(!encounter->opponentId && encounter->enemyTimer == -1.0f,
            "Compatibility operations update the same ECS encounter state.");
    antStore.Find(secondId)->BeginEncounter(firstId);
    Require(antStore.GetWorld().Components<ForagingComponent>().size() == antStore.GetCount(),
            "Each scene ant owns separately queryable foraging state.");
    auto *foraging = antStore.GetWorld().Get<ForagingComponent>(firstId);
    foraging->state = ForagingState::ToHomeWithFood;
    foraging->target = {17.0f, 23.0f};
    Require(antStore.Find(firstId)->IsCarryingFood() && snapshotForaging.state != ForagingState::ToHomeWithFood,
            "Live foraging changes affect behavior while snapshots stay detached.");
    Require(antStore.Find(firstId)->GetTarget() == foraging->target,
            "Target reads resolve authoritative ECS state.");
    antStore.Find(secondId)->GetForagingComponent().collectedFood = 9;
    antStore.GetAnts()[0].Kill();
    Require(alias.IsDead(), "Copied views refer to the same scene entity.");

    Require(antStore.RemoveDead() == 1, "Ant store should remove dead ants.");

    Require(antStore.GetWorld().Get<pipeframe::EnergyComponent>(firstId) == nullptr,
            "Destroying an ant removes its energy component.");
    Require(antStore.GetWorld().Get<AntEncounterComponent>(firstId) == nullptr,
            "Destroying an ant removes encounter state.");
    Require(antStore.Find(secondId)->GetEncounterComponent().opponentId == firstId,
            "Dense compaction preserves the moved ant's encounter binding.");
    antStore.Find(secondId)->EndEncounter();
    Require(antStore.GetWorld().Get<ForagingComponent>(firstId) == nullptr,
            "Destroying an ant removes its foraging component.");
    Require(antStore.Find(secondId)->GetForagingComponent().collectedFood == 9,
            "Dense relocation preserves the surviving ant's foraging binding.");
    Require(antStore.Find(firstId) == nullptr, "Removed ant ID should become invalid.");

    Require(antStore.Find(secondId)->GetEnergy() == antStore.GetWorld().Get<pipeframe::EnergyComponent>(secondId)->current,
            "Dense compaction must preserve energy binding for the moved ant.");
    Require(antStore.Find(secondId) != nullptr, "Removing another ant must preserve stable IDs.");

    colonySystem.RecountAntsAndUpdateRadii();

    Require(colony.GetAntCount() == 2, "Recount should reflect removed ants.");

    // Domain colony IDs and engine entity identities share storage safely.
    auto &sharedScene = antStore.GetScene();
    Require(sharedScene.Components().Components<ColonyStateComponent>().size() == colonySystem.GetColonyCount(),
            "Colonies must occupy the same engine world as ants.");
    const auto unrelated = sharedScene.CreateObject();
    struct Unrelated { int value = 42; };
    unrelated.AddComponent<Unrelated>();
    colonySystem.CreateColony(secondId, {4, 4}, {255, 255, 255});
    (void)colonySystem.RemoveColony(secondId);
    Require(antStore.Find(secondId)->GetEnergy() == antStore.GetWorld().Get<pipeframe::EnergyComponent>(secondId)->current,
            "Dense compaction must preserve energy binding for the moved ant.");
    Require(antStore.Find(secondId) != nullptr,
            "A colony domain ID equal to an ant entity ID must not destroy that ant.");
    antStore.Clear();
    Require(colonySystem.FindColony(100) != nullptr && unrelated.IsValid(),
            "Clearing ant components must preserve colonies and unrelated objects.");
    colonySystem.Clear();
    Require(sharedScene.Components().Components<ColonyStateComponent>().empty() && unrelated.IsValid(),
            "Clearing colonies must preserve unrelated scene objects.");
    Require(unrelated.GetComponent<Unrelated>()->value == 42,
            "Domain cleanup must not reset the shared world.");

    colonySystem.CreateColony(200, {8, 8}, {255, 0, 0});
    auto group = sharedScene.CreateObject();
    colonySystem.GetColonyObject(200).SetParent(group);
    group.SetActive(false);
    antStore.GetScene().FixedUpdate(0.1f);
    colonySystem.Update(0.1f);
    Require(antStore.GetCount() == 0, "Inactive scene parent must suspend colony spawning.");
    group.SetActive(true);
    antStore.GetScene().FixedUpdate(0.1f);
    colonySystem.Update(0.1f);
    Require(antStore.GetCount() > 0, "Reactivated scene parent must resume colony spawning.");
    group.Destroy();
    Require(colonySystem.FindColony(200) == nullptr && antStore.GetCount() == 0,
            "Parent destruction must remove colony and dependent ants through lifecycle cleanup.");
    Require(unrelated.IsValid(), "Cascade cleanup must preserve unrelated objects.");

    std::cout << "All ant colony tests passed.\n";

    return 0;
}

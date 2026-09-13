#include "World/AntWorld.h"
#include "Components/AntIdentityComponent.h"
#include "World/Runtime/Environment/AntWorldCell.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

ant_simulation::AntConfiguration CreateConfiguration(
    const std::uint32_t workerCount
) {
    ant_simulation::AntConfiguration configuration;
    configuration.worldSize = {64, 64};
    configuration.colonyPosition = {12.0f, 12.0f};
    configuration.colonyRadius = 2.0f;
    configuration.colonyInitialAntCount = 12;
    configuration.antMaxEnergy = 100'000.0f;
    configuration.antUpdateWorkerCount = workerCount;
    return configuration;
}

void CreateFourColonies(ant_simulation::AntWorld &world) {
    using namespace ant_simulation;
    constexpr std::array<pipeframe::Vector2f, 4> Positions{{
        {12.0f, 12.0f},
        {52.0f, 12.0f},
        {12.0f, 52.0f},
        {52.0f, 52.0f},
    }};
    constexpr std::array<pipeframe::Color, 4> Colors{{
        {239, 71, 111},
        {17, 138, 178},
        {6, 214, 160},
        {255, 209, 102},
    }};

    for (std::size_t index = 0; index < Positions.size(); ++index) {
        world.CreateColony(index + 1, Positions[index], Colors[index]);
    }
}

void VerifyForeignMarkerTransfer() {
    using namespace ant_simulation;
    AntWorldCell cell;

    cell.AddMarker(MarkerKind::ToFood, 4.0f, 1);
    cell.AddMarker(MarkerKind::ToFood, 1.0f, 2);
    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId == 1 &&
            cell.GetMarker(MarkerKind::ToFood).intensity == 3.0f,
        "A foreign deposit should erode the resident marker first.");

    cell.AddMarker(MarkerKind::ToFood, 4.0f, 2);
    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId == 1 &&
            cell.GetMarker(MarkerKind::ToFood).intensity == -1.0f,
        "Ownership should remain resident on the deposit that exhausts it.");

    cell.AddMarker(MarkerKind::ToFood, 2.0f, 2);
    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId == 2 &&
            cell.GetMarker(MarkerKind::ToFood).intensity == 2.0f,
        "The next foreign deposit should take ownership after exhaustion.");
}

void VerifyDeterministicAndParallelParity() {
    using namespace ant_simulation;
    AntWorld serial(CreateConfiguration(1), 17'001);
    AntWorld parallel(CreateConfiguration(4), 17'001);
    std::string errorMessage;

    Require(serial.Initialize(errorMessage), "Serial parity world should initialize.");
    Require(parallel.Initialize(errorMessage), "Parallel parity world should initialize.");
    CreateFourColonies(serial);
    CreateFourColonies(parallel);

    for (std::size_t tick = 1; tick <= 120; ++tick) {
        serial.FixedUpdate(1.0f / 60.0f);
        parallel.FixedUpdate(1.0f / 60.0f);

        if (tick == 1 || tick == 12 || tick == 60 || tick == 120) {
            const AntBehaviorCheckpoint serialCheckpoint =
                serial.CaptureBehaviorCheckpoint();
            const AntBehaviorCheckpoint parallelCheckpoint =
                parallel.CaptureBehaviorCheckpoint();
            Require(
                serialCheckpoint.stateSignature ==
                    parallelCheckpoint.stateSignature,
                "Serial and parallel modes should reach identical checkpoints.");
        }
    }

    const AntBehaviorCheckpoint checkpoint =
        serial.CaptureBehaviorCheckpoint();
    Require(checkpoint.colonies.size() == 4,
            "Four-colony scenario should preserve every competitor.");
    Require(checkpoint.totalBirths == 48,
            "Each funded colony should create its configured population.");
    Require(checkpoint.antCount == 48 && checkpoint.physicsBodyCount == 48,
            "All born ants should have synchronized physics bodies.");

    for (const AntColonyCheckpoint &colony : checkpoint.colonies) {
        Require(colony.antCount == 12,
                "Symmetric colonies should receive equal birth accounting.");
        Require(colony.reserve == 0.0f,
                "Each colony should spend the same configured reserve.");
    }

    AntWorld replay(CreateConfiguration(1), 17'001);
    Require(replay.Initialize(errorMessage), "Replay world should initialize.");
    CreateFourColonies(replay);
    for (std::size_t tick = 0; tick < 120; ++tick) {
        replay.FixedUpdate(1.0f / 60.0f);
    }
    Require(
        replay.CaptureBehaviorCheckpoint().stateSignature ==
            checkpoint.stateSignature,
        "A seeded deterministic replay should reproduce the full state checkpoint.");
}

void VerifyColonyRemovalAndDeathAccounting() {
    using namespace ant_simulation;
    AntConfiguration configuration = CreateConfiguration(1);
    configuration.colonyInitialAntCount = 2;
    AntWorld world(configuration, 17'002);
    std::string errorMessage;
    Require(world.Initialize(errorMessage), "Removal world should initialize.");
    world.CreateColony(1, {12.0f, 12.0f}, {239, 71, 111});
    world.CreateColony(2, {52.0f, 52.0f}, {17, 138, 178});
    world.FixedUpdate(1.0f / 60.0f);
    world.FixedUpdate(1.0f / 60.0f);

    AntWorldCell *foreignTrail = world.GetEnvironment().TryGetCell(30, 30);
    Require(foreignTrail != nullptr, "Removal fixture cell should exist.");
    foreignTrail->AddMarker(MarkerKind::ToFood, 10.0f, 1);

    Require(world.RemoveColony(1), "Existing colony removal should succeed.");
    Require(!world.RemoveColony(1), "Removed colony should not be removed twice.");
    Require(world.GetColonyLifecycleSystem().FindColony(1) == nullptr,
            "Removed colony should leave the colony system.");
    Require(world.GetColonyLifecycleSystem().FindColony(2) != nullptr,
            "Unrelated colony should remain active.");
    Require(world.GetStatistics().colonyCount == 1 &&
                world.GetStatistics().antCount == 2 &&
                world.GetStatistics().physicsBodyCount == 2,
            "Colony removal should clean dependent ants and physics bodies.");
    Require(!foreignTrail->GetMarker(MarkerKind::ToFood).HasOwner(),
            "Colony removal should clear its persistent and transient markers.");
    Require(world.GetStatistics().totalDeaths == 2,
            "Removal should be included in cumulative death accounting.");

    AntView remainingAnt = world.GetAntQuery().GetAnts().front();
    remainingAnt.Kill();
    world.FixedUpdate(1.0f / 60.0f);
    Require(world.GetStatistics().antCount == 1 &&
                world.GetStatistics().physicsBodyCount == 1,
            "A dead ant should leave population and physics in one tick.");
    Require(world.GetStatistics().totalDeaths == 3,
            "Natural cleanup should update cumulative death accounting.");
}

} // namespace

int main() {
    VerifyForeignMarkerTransfer();
    VerifyDeterministicAndParallelParity();
    VerifyColonyRemovalAndDeathAccounting();
    std::cout << "All AntPezza behavioral parity tests passed.\n";
    return 0;
}

#include "World/AntWorld.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

ant_simulation::AntConfiguration CreateStressConfiguration() {
    ant_simulation::AntConfiguration configuration;
    configuration.worldSize = {512, 512};
    configuration.colonyPosition = {256.0f, 256.0f};
    configuration.colonyInitialAntCount = 1;
    configuration.antMaxEnergy = 100'000.0f;
    configuration.markerDecayRate = 0.1f;
    return configuration;
}

void Populate(ant_simulation::AntWorld &world, const std::size_t count, const std::size_t columns,
              const float spacing) {
    using namespace ant_simulation;

    AntQuery &store = world.GetAntQuery();
    const AntConfiguration &configuration = world.GetConfiguration();

    for (std::size_t index = 0; index < count; ++index) {
        const pipeframe::Vector2f position{
            8.0f + static_cast<float>(index % columns) * spacing,
            8.0f + static_cast<float>(index / columns) * spacing,
        };

        AntView ant = store.Create(1, AntRole::Follower, position,
                                   static_cast<float>(index % 360) * AntConfiguration::DegreesToRadians(1.0f),
                                   static_cast<float>(index % 31) / 31.0f, configuration);
        ant.Identity().color = configuration.toFoodAntColor;
    }

    world.GetPhysicsBodies().Synchronize();
}

void VerifyFiniteForagingState(const ant_simulation::AntWorld &world) {
    const pipeframe::Vector2f worldSize = world.GetConfiguration().GetWorldSizeFloat();

    for (const ant_simulation::AntView &ant : world.GetAntQuery().GetAnts()) {
        const pipeframe::Vector2f position = ant.GetPosition();
        const pipeframe::Vector2f direction = ant.GetDirection();

        Require(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(direction.x) &&
                    std::isfinite(direction.y),
                "Ant state must remain finite under load.");
        Require(position.x >= 0.0f && position.y >= 0.0f && position.x <= worldSize.x && position.y <= worldSize.y,
                "Ants must remain inside the guarded world border under load.");
    }
}

void RunCapacityGate() {
    using namespace ant_simulation;

    AntWorld world(CreateStressConfiguration(), 14'001);
    std::string errorMessage;
    Require(world.Initialize(errorMessage), "100K capacity world should initialize.");
    world.CreateColony(1, {256.0f, 256.0f}, pipeframe::Color(239, 71, 111)).SetReserve(0.0f);

    const auto started = Clock::now();
    Populate(world, 100'000, 400, 1.24f);
    const auto elapsed = Clock::now() - started;

    Require(world.GetAntQuery().GetCount() == 100'000, "Ant store should hold 100K ants.");
    Require(world.GetPhysicsBodies().GetBodyCount() == 100'000, "Physics should synchronize 100K bodies.");

    std::cout << "100K capacity: " << std::chrono::duration<double, std::milli>(elapsed).count() << " ms\n";
}

void RunThroughputGate() {
    using namespace ant_simulation;

    constexpr std::size_t AntCount = 10'000;
    constexpr std::size_t TickCount = 30;

    AntConfiguration configuration = CreateStressConfiguration();
    configuration.antUpdateWorkerCount = 4;
    AntWorld world(configuration, 14'002);
    std::string errorMessage;
    Require(world.Initialize(errorMessage), "10K throughput world should initialize.");
    world.CreateColony(1, {256.0f, 256.0f}, pipeframe::Color(239, 71, 111)).SetReserve(0.0f);
    Populate(world, AntCount, 128, 3.0f);

    AntSimulationStepResult accumulatedTimings;
    const auto started = Clock::now();
    for (std::size_t tick = 0; tick < TickCount; ++tick) {
        const AntSimulationStepResult result = world.FixedUpdate(1.0f / 60.0f);
        accumulatedTimings.preparationTimeMs += result.preparationTimeMs;
        accumulatedTimings.avoidanceTimeMs += result.avoidanceTimeMs;
        accumulatedTimings.physicsTimeMs += result.physicsTimeMs;
        accumulatedTimings.behaviorTimeMs += result.behaviorTimeMs;
        accumulatedTimings.cleanupTimeMs += result.cleanupTimeMs;
    }
    const double elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();

    Require(world.GetStatistics().tick == TickCount, "10K throughput run should complete every tick.");
    Require(world.GetAntQuery().GetCount() == AntCount, "10K ants should survive the throughput run.");
    VerifyFiniteForagingState(world);

    const double updatesPerSecond = static_cast<double>(AntCount * TickCount) / elapsedSeconds;
    std::cout << "10K throughput: " << updatesPerSecond << " ant updates/s (" << elapsedSeconds << " s)\n"
              << "Average phases: preparation " << accumulatedTimings.preparationTimeMs / TickCount << " ms, avoidance "
              << accumulatedTimings.avoidanceTimeMs / TickCount << " ms, physics/contact "
              << accumulatedTimings.physicsTimeMs / TickCount << " ms, behavior "
              << accumulatedTimings.behaviorTimeMs / TickCount << " ms, cleanup "
              << accumulatedTimings.cleanupTimeMs / TickCount << " ms\n";
}

void RunSoakGate() {
    using namespace ant_simulation;

    constexpr std::size_t AntCount = 2'000;
    constexpr std::size_t TickCount = 600;

    AntWorld world(CreateStressConfiguration(), 14'003);
    std::string errorMessage;
    Require(world.Initialize(errorMessage), "Soak world should initialize.");
    world.CreateColony(1, {256.0f, 256.0f}, pipeframe::Color(239, 71, 111)).SetReserve(0.0f);
    Populate(world, AntCount, 64, 6.0f);

    const auto started = Clock::now();
    for (std::size_t tick = 0; tick < TickCount; ++tick) {
        world.FixedUpdate(1.0f / 60.0f);
    }
    const double elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();

    Require(world.GetStatistics().tick == TickCount, "Soak run should complete every tick.");
    Require(world.GetAntQuery().GetCount() == AntCount, "Ant count should remain stable during soak.");
    Require(world.GetPhysicsBodies().GetBodyCount() == AntCount, "Physics count should remain stable during soak.");
    VerifyFiniteForagingState(world);

    std::cout << "Soak: " << AntCount << " ants x " << TickCount << " ticks in " << elapsedSeconds << " s\n";
}

} // namespace

int main() {
    RunCapacityGate();
    RunThroughputGate();
    RunSoakGate();
    std::cout << "All ant stress and performance tests passed.\n";
    return 0;
}

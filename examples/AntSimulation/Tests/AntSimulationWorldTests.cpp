#include "World/AntWorld.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntWorldCell.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr
            << "FAILED: "
            << message
            << '\n';

        std::exit(1);
    }
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
        8.0f,
        8.0f,
    };

    configuration.colonyRadius =
        2.0f;

    configuration.colonyInitialAntCount =
        2;

    configuration.explorerProbability =
        0.0f;

    AntWorld simulation(
        configuration,
        12345);

    std::string errorMessage;

    Require(
        simulation.Initialize(
            errorMessage),
        "Simulation world should initialize.");

    Require(
        simulation.IsInitialized(),
        "Initialized simulation should report ready.");

    Require(
        simulation.GetEnvironment()
                .GetWidth() ==
            32,
        "Simulation should create configured world width.");

    Require(
        simulation.GetStatistics()
                .colonyCount ==
            0,
        "Fresh simulation should not create an implicit colony.");

    ColonyView &colony =
        simulation.CreateColony(
            7,
            configuration.colonyPosition,
            configuration.toFoodAntColor);

    Require(
        colony.GetId() == 7,
        "Created colony should retain stable ID.");

    Require(
        simulation.GetStatistics()
                .colonyCount ==
            1,
        "Statistics should report created colony.");

    const WorldEntityId foodId =
        simulation.GetEnvironment()
            .AddFood(
                {20.5f, 20.5f},
                25);

    Require(
        foodId !=
            InvalidWorldEntityId,
        "Simulation environment should accept food.");

    const AntSimulationWorldStatistics
        beforeUpdate =
            simulation.GetStatistics();

    Require(
        beforeUpdate.antCount == 0,
        "Colony should not spawn before a simulation tick.");

    const AntSimulationStepResult firstResult =
        simulation.FixedUpdate(
            1.0f / 60.0f);

    Require(
        firstResult.livingAnts == 1,
        "First tick should create and update one ant.");

    Require(
        simulation.GetStatistics()
                .tick ==
            1,
        "Simulation tick should advance.");

    Require(
        simulation.GetStatistics()
                .antCount ==
            1,
        "First tick should contain one ant.");

    Require(
        simulation.GetStatistics()
                .physicsBodyCount ==
            1,
        "Spawned ant should receive a physics body.");

    simulation.FixedUpdate(
        1.0f / 60.0f);

    Require(
        simulation.GetStatistics()
                .tick ==
            2,
        "Second update should advance tick again.");

    Require(
        simulation.GetStatistics()
                .antCount ==
            2,
        "Funded colony should create its second ant.");

    Require(
        simulation.GetStatistics()
                .physicsBodyCount ==
            2,
        "Both ants should receive physics bodies.");

    Require(
        simulation.GetStatistics()
                .foodEntityCount ==
            1,
        "Statistics should report food entity.");

    Require(
        simulation.GetStatistics()
                .totalFoodQuantity ==
            25,
        "Statistics should report food quantity.");

    Require(
        simulation.GetStatistics()
                .wallCount >
            0,
        "Initialized world should include border walls.");

    Require(
        simulation.GetColonyLifecycleSystem()
                .FindColony(7) !=
            nullptr,
        "Colony system should find created colony.");

    Require(
        simulation.GetAntQuery()
                .GetCount() ==
            2,
        "Ant store should expose spawned ants.");

    Require(
        simulation.GetPhysicsBodies()
                .GetBodyCount() ==
            2,
        "Physics world should expose synchronized bodies.");

    Require(
        simulation.Reset(
            errorMessage),
        "Simulation should reset.");

    Require(
        simulation.IsInitialized(),
        "Reset simulation should remain initialized.");

    Require(
        simulation.GetStatistics()
                .tick ==
            0,
        "Reset should clear tick.");

    Require(
        simulation.GetStatistics()
                .colonyCount ==
            0,
        "Reset should clear colonies.");

    Require(
        simulation.GetStatistics()
                .antCount ==
            0,
        "Reset should clear ants.");

    Require(
        simulation.GetStatistics()
                .foodEntityCount ==
            0,
        "Reset should clear authored food.");

    int behaviourUpdates = 0;
    struct TickProbe : pipeframe::Behaviour {
        explicit TickProbe(int &updates) : updates(updates) {}
        void FixedUpdate(float) override { ++updates; }
        int &updates;
    };
    auto probe = simulation.GetScene().CreateObject();
    probe.Attach<TickProbe>(behaviourUpdates);
    simulation.BeginFixedStep(0.01f);
    bool duplicateRejected = false;
    try { simulation.BeginFixedStep(0.01f); } catch (const std::logic_error &) { duplicateRejected = true; }
    Require(duplicateRejected && behaviourUpdates == 1, "Repeated pre-physics must not dispatch scripts twice.");
    simulation.UpdateMovement(0.01f);
    bool skippedRejected = false;
    try { simulation.EndFixedStep(0.01f); } catch (const std::logic_error &) { skippedRejected = true; }
    Require(skippedRejected, "Cleanup cannot skip the behaviour phase.");
    simulation.UpdateBehavior(0.01f);
    simulation.EndFixedStep(0.01f);
    (void)simulation.FixedUpdate(0.01f);
    Require(behaviourUpdates == 2, "Direct and externally phased ticks must dispatch scripts exactly once.");
    simulation.GetColonyLifecycleSystem().Update(0.01f);
    Require(behaviourUpdates == 2, "Colony maintenance must not dispatch unrelated scene scripts.");
    probe.Destroy();

    Require(&simulation.GetScene() == &simulation.GetAntQuery().GetScene(),
            "Ant storage must use the scene owned by simulation state.");
    const auto oldObject = simulation.GetScene().CreateObject();
    simulation.Clear();
    Require(!oldObject.IsValid(), "Clearing the simulation invalidates scene object handles.");

    Require(
        !simulation.IsInitialized(),
        "Cleared simulation should not remain initialized.");

    std::cout
        << "All ant simulation world tests passed.\n";

    return 0;
}

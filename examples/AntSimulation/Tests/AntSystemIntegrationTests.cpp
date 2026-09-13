#include "World/Runtime/AntView.h"
#include "World/Runtime/AntQuery.h"
#include "World/Physics/AntMovementSystem.h"
#include "World/Runtime/Systems/AntForagingSystem.h"
#include "World/Runtime/Systems/AntCleanupSystem.h"
#include "World/Runtime/AntStepResult.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/ContactSolver.h"

#include <type_traits>


#include "World/Runtime/Environment/AntEnvironment.h"

#include <cmath>
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

bool NearlyEqual(
    const float first,
    const float second,
    const float tolerance = 0.0001f
) {
    return std::abs(first - second) <=
           tolerance;
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

    configuration.colonyInitialAntCount = 1;
    configuration.antSpeed = 2.0f;

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Environment should initialize.");

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    ColonyLifecycleSystem colonySystem(
        environment,
        antStore,
        configuration,
        12345);

    ColonyView &colony =
        colonySystem.CreateColony(
            1,
            configuration.colonyPosition,
            pipeframe::Color::Blue);

    AntView ant =
        antStore.Create(
            colony.GetId(),
            AntRole::Follower,
            {12.0f, 8.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId antId =
        ant.GetId();

    ant.SetTarget(
        {20.0f, 8.0f});

    AntBodySystem physicsWorld(
        antStore,
        configuration);

    ContactSolver contactSolver(
        antStore,
        environment);

    AntMovementSystem movement(antStore, physicsWorld, contactSolver, configuration);
    AntForagingSystem behavior(antStore, colonySystem, environment, configuration, 12345);
    AntCleanupSystem cleanup(antStore, colonySystem, physicsWorld, configuration.worldSize);
    auto update = [&](float delta) {
        const auto moved = movement.Update(delta);
        (void)moved;
        behavior.Update(delta);
        const auto cleaned = cleanup.Update(delta);
        AntSimulationStepResult result;
        result.livingAnts = antStore.GetCount();
        result.removedAnts = cleaned.removedAnts;
        return result;
    };

    const float initialEnergy =
        antStore.Find(antId)
            ->GetEnergy();

    const AntSimulationStepResult firstResult =
        update(0.5f);

    Require(
        firstResult.livingAnts == 1,
        "The living ant should remain in the store.");

    Require(
        firstResult.removedAnts == 0,
        "A living ant should not be removed.");

    const AntView *updatedAnt =
        antStore.Find(antId);

    Require(
        updatedAnt != nullptr,
        "Updated ant should still exist.");

    Require(
        updatedAnt->GetPosition().x >
            12.0f,
        "Registered Ant systems should move the ant toward its target.");

    Require(
        NearlyEqual(
            updatedAnt->GetPosition().y,
            8.0f),
        "Horizontal target movement should preserve Y.");

    Require(
        NearlyEqual(
            updatedAnt->GetEnergy(),
            initialEnergy - 0.5f),
        "Registered Ant systems should consume energy using delta time.");

    Require(
        updatedAnt->Motion().speed > 0.0f,
        "Registered Ant systems should record current movement speed.");

    Require(
        updatedAnt->Motion().travelDistance >
            0.0f,
        "Registered Ant systems should accumulate travel distance.");

    Require(
        updatedAnt->GetDistanceToTarget() <
            8.0f,
        "Movement should reduce the remaining target distance.");

    updatedAnt =
        antStore.Find(antId);

    AntView *mutableAnt =
        antStore.Find(antId);

    mutableAnt->GetEnergyComponent().current = 0.1f;

    const AntSimulationStepResult deathResult =
        update(0.2f);

    Require(
        deathResult.removedAnts == 1,
        "An exhausted ant should be removed.");

    Require(
        deathResult.livingAnts == 0,
        "No ants should remain after exhaustion.");

    Require(
        antStore.Find(antId) == nullptr,
        "Removed AntId should no longer resolve.");

    Require(
        physicsWorld.GetBodyCount() == 0,
        "Removing a dead ant should remove its physics body.");

    Require(
        colony.GetAntCount() == 0,
        "Colony count should be refreshed after removal.");

    std::cout
        << "All ant updater tests passed.\n";

    return 0;
}

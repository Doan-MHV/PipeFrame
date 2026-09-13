#include "ColonyFixture.h"
#include "AntFixture.h"
#include "World/Runtime/AntView.h"
#include "World/Runtime/Systems/WorkerBehavior.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Marker.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
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
        5.5f,
        5.5f,
    };

    configuration.colonyRadius = 3.0f;
    configuration.antMarkerDistance = 3.0f;

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Environment should initialize.");

    ColonyFixture colony(
    1,
    configuration.colonyPosition,
    pipeframe::Color::Blue,
    configuration);

    const float initialColonyReserve =
        colony.GetReserve();

    WorkerBehavior behavior(
        environment,
        configuration);

    std::mt19937 randomGenerator(12345);

    // ---------------------------------------------------------
    // Food collection
    // ---------------------------------------------------------

    AntFixture foodCollector(
        1,
        colony.GetId(),
        AntRole::Follower,
        {12.5f, 12.5f},
        0.0f,
        0.0f,
        configuration);

    foodCollector.SetTarget(
        {20.5f, 12.5f});

    foodCollector.ConsumeEnergy(100.0f);

    const WorldEntityId foodId =
        environment.AddFood(
            foodCollector.GetPosition(),
            1);

    Require(
        foodId != InvalidWorldEntityId,
        "Food should be created.");

    behavior.Update(
        foodCollector, foodCollector.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    Require(
        foodCollector.GetState() ==
            ForagingState::ToHomeWithFood,
        "An ant searching for food should collect it.");

    Require(
        foodCollector.IsCarryingFood(),
        "The food collector should carry food.");

    Require(
        NearlyEqual(
            foodCollector.GetMass(),
            AntView::FoodMass),
        "Carrying food should use the food mass.");

    Require(
        NearlyEqual(
            foodCollector.GetEnergy(),
            configuration.antMaxEnergy),
        "Collecting food should refill energy.");

    Require(
        environment.GetTotalFoodQuantity() == 0,
        "Collected food should be removed from the world.");

    // ---------------------------------------------------------
    // Food delivery
    // ---------------------------------------------------------

    foodCollector.SetPosition(
        colony.GetPosition());

    foodCollector.SetTarget(
        {20.5f, 12.5f});

    behavior.Update(
        foodCollector, foodCollector.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    Require(
        foodCollector.GetState() ==
            ForagingState::ToFood,
        "An ant should search again after delivering food.");

    Require(
        !foodCollector.IsCarryingFood(),
        "Delivered food should no longer be carried.");

    Require(
        foodCollector.GetForagingComponent().collectedFood == 1,
        "The ant should count delivered food.");

    Require(
        NearlyEqual(
            colony.GetFoodQuantity(),
            1.0f),
        "Delivered food should be added to the colony.");

    Require(
    NearlyEqual(
        colony.GetReserve(),
        initialColonyReserve + 1.0f),
    "Delivered food should replenish colony reserve.");

    Require(
        NearlyEqual(
            foodCollector.GetMass(),
            AntView::BaseMass),
        "After delivery the ant should use its base mass.");

    Require(
        NearlyEqual(
            colony.State().soldierRequested,
            0.2f),
        "Entering the colony should update soldier demand.");

    Require(
        NearlyEqual(
            foodCollector.GetEncounterComponent().enemyTimer,
            0.0f),
        "Entering the colony should refresh the enemy marker timer.");

    // ---------------------------------------------------------
    // Normal marker deposit
    // ---------------------------------------------------------

    AntFixture markerAnt(
        2,
        colony.GetId(),
        AntRole::Follower,
        {15.5f, 15.5f},
        0.0f,
        0.0f,
        configuration);

    markerAnt.SetTarget(
        {20.5f, 15.5f});

    markerAnt.GetForagingComponent().lastMarkerPosition = {
        10.5f,
        15.5f,
    };

    AntWorldCell *markerCell =
        environment.TryGetCellAtWorldPosition(
            markerAnt.GetPosition());

    Require(
        markerCell != nullptr,
        "Marker cell should exist.");

    behavior.Update(
        markerAnt, markerAnt.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    const Marker &homeMarker =
        markerCell->GetMarker(
            MarkerKind::ToHome);

    Require(
        homeMarker.colonyId ==
            colony.GetId(),
        "A searching ant should deposit a home marker.");

    Require(
        homeMarker.intensity > 0.0f,
        "Deposited home marker should have intensity.");

    // ---------------------------------------------------------
    // Blocked-path marker degradation
    // ---------------------------------------------------------

    AntFixture blockedAnt(
        3,
        colony.GetId(),
        AntRole::Follower,
        {18.5f, 18.5f},
        0.0f,
        0.0f,
        configuration);

    blockedAnt.SetTarget(
        {24.5f, 18.5f});

    blockedAnt.GetForagingComponent().lastMarkerPosition = {
        10.5f,
        18.5f,
    };

    blockedAnt.GetForagingComponent().timeSinceLastMarker =
        AntView::MarkerTimeoutCoefficient *
            configuration.GetAntMarkerInterval() +
        1.0f;

    AntWorldCell *blockedCell =
        environment.TryGetCellAtWorldPosition(
            blockedAnt.GetPosition());

    Require(
        blockedCell != nullptr,
        "Blocked-ant cell should exist.");

    blockedCell->AddMarker(
        MarkerKind::ToFood,
        100.0f,
        colony.GetId());

    behavior.Update(
        blockedAnt, blockedAnt.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    Require(
        blockedAnt.GetForagingComponent().blocked,
        "A timed-out ant should be marked as blocked.");

    Require(
        NearlyEqual(
            blockedCell
                ->GetMarker(
                    MarkerKind::ToFood)
                .intensity,
            0.0f),
        "A blocked ant should clear its misleading objective marker.");

    // ---------------------------------------------------------
    // ToHomeNoFood must reset its marker interval without
    // depositing a navigation marker.
    // ---------------------------------------------------------

    AntFixture emptyReturnAnt(
        4,
        colony.GetId(),
        AntRole::Follower,
        {21.5f, 21.5f},
        0.0f,
        0.0f,
        configuration);

    emptyReturnAnt.SetState(
        ForagingState::ToHomeNoFood);

    emptyReturnAnt.SetTarget(
        {25.5f, 21.5f});

    emptyReturnAnt.GetForagingComponent().lastMarkerPosition = {
        15.5f,
        21.5f,
    };

    emptyReturnAnt.GetForagingComponent().timeSinceLastMarker = 2.0f;

    behavior.Update(
        emptyReturnAnt, emptyReturnAnt.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    Require(
        NearlyEqual(
            emptyReturnAnt.GetForagingComponent().timeSinceLastMarker,
            0.0f),
        "A no-food return should still reset its marker timer.");

    // ---------------------------------------------------------
    // Wall collision
    // ---------------------------------------------------------

    AntFixture wallAnt(
        5,
        colony.GetId(),
        AntRole::Follower,
        {24.5f, 24.5f},
        0.0f,
        0.0f,
        configuration);

    wallAnt.SetTarget(
        {26.5f, 24.5f});

    Require(
        environment.AddWall(
            wallAnt.GetPosition()),
        "Wall should be created.");

    behavior.Update(
        wallAnt, wallAnt.GetForagingComponent(),
        colony,
        1.0f / 60.0f,
        randomGenerator);

    Require(
        wallAnt.IsDead(),
        "An ant occupying a wall cell should die.");

    std::cout
        << "All worker behavior tests passed.\n";

    return 0;
}

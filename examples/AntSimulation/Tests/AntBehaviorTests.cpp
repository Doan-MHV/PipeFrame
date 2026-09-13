#include "AntFixture.h"
#include "World/Runtime/AntView.h"
#include "Components/AntIdentityComponent.h"
#include "Components/ForagingComponent.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/Marker.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool NearlyEqual(
    const float first,
    const float second,
    const float tolerance = 0.0001f
) {
    return std::abs(first - second) <= tolerance;
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    AntFixture ant(
        10,
        20,
        AntRole::Follower,
        {10.0f, 10.0f},
        0.0f,
        1.0f,
        configuration);

    Require(
        ant.GetId() == 10,
        "Ant should retain its stable ID.");

    Require(
        ant.GetColonyId() == 20,
        "Ant should retain its colony ID.");

    Require(
        ant.GetRole() == AntRole::Follower,
        "Ant should retain its role.");

    Require(
        ant.GetState() == ForagingState::ToFood,
        "New ant should initially search for food.");

    Require(
        NearlyEqual(
            ant.GetEnergy(),
            configuration.antMaxEnergy),
        "New ant should begin with maximum energy.");

    Require(
        ant.GetForagingComponent().lastMarkerPosition ==
            pipeframe::Vector2f{9.0f, 10.0f},
        "Initial marker offset should follow the ant direction.");

    Require(
        NearlyEqual(
            ant.Pose().direction.GetSpeed(),
            4.0f),
        "Body direction speed should match AntPezza.");

    Require(
        NearlyEqual(
            ant.Pose().headDirection.GetSpeed(),
            5.0f),
        "Head direction speed should match AntPezza.");

    Require(
        NearlyEqual(
            ant.Pose().tailDirection.GetSpeed(),
            3.0f),
        "Tail direction speed should match AntPezza.");

    Require(
        ant.GetLegs().size() == 6,
        "Ant should contain six animated legs.");

    Require(
        ant.GetDropMarkerKind() ==
            MarkerKind::ToHome,
        "Food-searching ants should drop home markers.");

    Require(
        ant.GetMarkerFocus() ==
            MarkerKind::ToFood,
        "Food-searching ants should follow food markers.");

    ant.SetState(
        ForagingState::ToHomeWithFood);

    Require(
        ant.GetDropMarkerKind() ==
            MarkerKind::ToFood,
        "Food-carrying ants should drop food markers.");

    Require(
        ant.GetMarkerFocus() ==
            MarkerKind::ToHome,
        "Food-carrying ants should follow home markers.");

    Require(
        ant.IsCarryingFood(),
        "Returning-with-food state should carry food.");

    Require(
        NearlyEqual(
            ant.GetMass(),
            AntView::FoodMass),
        "Food-carrying ant should use food mass.");

    ant.SetState(
        ForagingState::ToHomeNoFood);

    Require(
        ant.GetDropMarkerKind() ==
            MarkerKind::None,
        "Empty returning ants should not drop objective markers.");

    Require(
        !ant.IsCarryingFood(),
        "Returning-without-food state should not carry food.");

    Require(
        NearlyEqual(
            ant.GetMass(),
            AntView::BaseMass),
        "Non-carrying ant should use base mass.");

    ant.SetPosition(
        {15.0f, 10.0f});

    Require(
        ant.IsMarkerReady(
            configuration.antMarkerDistance),
        "Ant should become marker-ready after travelling far enough.");

    ant.SetState(
        ForagingState::ToFood);

    Require(
        ant.DropMarker() ==
            MarkerKind::ToHome,
        "DropMarker should return the active marker kind.");

    Require(
        ant.GetForagingComponent().lastMarkerPosition ==
            ant.GetPosition(),
        "Dropping a marker should update the last marker position.");

    Require(
        NearlyEqual(
            ant.GetForagingComponent().timeSinceLastMarker,
            0.0f),
        "Dropping a marker should reset its timer.");

    ant.SetTarget(
        {15.0f, 20.0f});

    Require(
        NearlyEqual(
            ant.GetDistanceToTarget(),
            10.0f),
        "SetTarget should calculate target distance.");

    Require(
        ant.Pose().direction.GetTarget().y > 0.99f,
        "Target direction should point toward the target.");

    ant.Update(1.0f);

    Require(
        NearlyEqual(
            ant.GetForagingComponent().walkTime,
            1.0f),
        "Ant update should advance walk time.");

    Require(
        NearlyEqual(
            ant.GetForagingComponent().timeSinceLastMarker,
            1.0f),
        "Ant update should advance marker time.");

    Require(
        ant.GetAngle() > 0.0f,
        "Tracking direction should turn toward its target.");

    const float markerIntensity =
        ant.GetMarkerIntensity(
            configuration);

    Require(
        markerIntensity <
            configuration.markerMaxIntensity,
        "Marker strength should decrease with walk time.");

    ant.GetForagingComponent().timeSinceLastMarker =
        AntView::MarkerTimeoutCoefficient *
            configuration
                .GetAntMarkerInterval() +
        0.1f;

    Require(
        ant.IsBlocked(configuration),
        "Long marker timeout should classify an ant as blocked.");

    Require(
        ant.GetBlockedRatio(configuration) >
            0.99f,
        "Blocked ratio should reach one after the timeout.");

    ant.ConsumeEnergy(100.0f);

    Require(
        NearlyEqual(
            ant.GetEnergy(),
            configuration.antMaxEnergy -
                100.0f),
        "EnergyComponent consumption should reduce ant energy.");

    ant.RefillEnergy(configuration);

    Require(
        NearlyEqual(
            ant.GetEnergy(),
            configuration.antMaxEnergy),
        "Refill should restore maximum energy.");

    ant.Kill();

    Require(
        ant.IsDead(),
        "Killed ant should be dead.");

    ant.RefillEnergy(configuration);

    Require(
        !ant.IsDead(),
        "Refilled ant should become alive again.");

    ant.BeginEncounter(99);

    Require(
        ant.IsInEncounter(),
        "BeginEncounter should assign an opponent.");

    Require(
        ant.GetEncounterComponent().opponentId ==
            std::optional<AntId>{99},
        "Encounter should store the opponent ID.");

    ant.Update(0.5f);

    Require(
        NearlyEqual(
            ant.GetEncounterComponent().enemyTimer,
            0.5f),
        "Encounter update should advance enemy time.");

    Require(
        ant.GetEnemyMarkerIntensity(
            configuration) <
            configuration.markerMaxIntensity,
        "Enemy marker should decay with encounter time.");

    ant.EndEncounter();

    Require(
        !ant.IsInEncounter(),
        "EndEncounter should clear the opponent.");

    Require(
        NearlyEqual(
            ant.GetEncounterComponent().enemyTimer,
            -1.0f),
        "EndEncounter should reset enemy time.");

    AntFixture soldier(
        11,
        20,
        AntRole::Soldier,
        {10.0f, 10.0f},
        0.0f,
        0.0f,
        configuration);

    Require(
        soldier.GetMarkerFocus() ==
            MarkerKind::ToEnemy,
        "Soldiers should follow enemy markers.");

    ant.SetPosition(
        {30.0f, 30.0f});

    ant.UpdateLegs(1.0f / 60.0f);

    bool hasMovingLeg{false};

    for (const AntLegPose &leg :
         ant.GetLegs()) {
        if (!leg.IsDone()) {
            hasMovingLeg = true;
            break;
        }
    }

    Require(
        hasMovingLeg,
        "Moving an ant should begin a leg animation.");

    Require(
        ToString(AntRole::Explorer) ==
            "Explorer",
        "Ant roles should have inspector labels.");

    Require(
        ToString(
            ForagingState::ToHomeWithFood) ==
            "Returning with food",
        "Ant states should have inspector labels.");

    std::cout << "All ant behavior tests passed.\n";

    return 0;
}

#include "World/Runtime/AntView.h"
#include "World/Physics/AntContactSystem.h"
#include "Components/AntIdentityComponent.h"
#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

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

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    AntView firstFriendly =
        antStore.Create(
            1,
            AntRole::Follower,
            {5.0f, 5.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId firstFriendlyId =
        firstFriendly.GetId();

    AntView secondFriendly =
        antStore.Create(
            1,
            AntRole::Explorer,
            {5.5f, 5.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId secondFriendlyId =
        secondFriendly.GetId();

    AntView enemy =
        antStore.Create(
            2,
            AntRole::Follower,
            {10.0f, 10.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId enemyId =
        enemy.GetId();

    // Vector growth can move Ant objects, so always reacquire
    // them through stable AntId values.
    AntContactSystem contactSystem(
        antStore,
        {configuration.worldSize.x,configuration.worldSize.y});

    Require(
        contactSystem.ProcessContacts() == 0,
        "Ants from the same colony should not trigger enemy contact.");

    Require(
        antStore.Find(firstFriendlyId)
                ->GetEncounterComponent().enemyTimer <
            0.0f,
        "Friendly contact should not reset the enemy timer.");

    Require(
        antStore.Find(secondFriendlyId)
                ->GetEncounterComponent().enemyTimer <
            0.0f,
        "Friendly contact should not alert the second ant.");

    antStore.Find(enemyId)
        ->SetPosition({
            5.8f,
            5.0f,
        });

    const std::size_t alertedCount =
        contactSystem.ProcessContacts();

    Require(
        alertedCount == 3,
        "All nearby workers should detect an opposing colony.");

    Require(
        NearlyEqual(
            antStore.Find(firstFriendlyId)
                ->GetEncounterComponent().enemyTimer,
            0.0f),
        "The first worker should reset its enemy timer.");

    Require(
        NearlyEqual(
            antStore.Find(secondFriendlyId)
                ->GetEncounterComponent().enemyTimer,
            0.0f),
        "The second worker should reset its enemy timer.");

    Require(
        NearlyEqual(
            antStore.Find(enemyId)
                ->GetEncounterComponent().enemyTimer,
            0.0f),
        "The enemy worker should also detect opposing ants.");

    AntView soldier =
        antStore.Create(
            1,
            AntRole::Soldier,
            {20.0f, 20.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId soldierId =
        soldier.GetId();

    AntView soldierEnemy =
        antStore.Create(
            2,
            AntRole::Follower,
            {20.5f, 20.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId soldierEnemyId =
        soldierEnemy.GetId();

    contactSystem.ProcessContacts();

    Require(
        antStore.Find(soldierId)
                ->GetEncounterComponent().enemyTimer <
            0.0f,
        "Soldier contacts should remain unchanged because "
        "AntPezza's soldier contact method is empty.");

    Require(
        NearlyEqual(
            antStore.Find(soldierEnemyId)
                ->GetEncounterComponent().enemyTimer,
            0.0f),
        "A worker should detect a nearby enemy soldier.");

    antStore.Find(soldierEnemyId)
        ->Kill();

    antStore.Find(soldierId)
        ->GetEncounterComponent().enemyTimer = -1.0f;

    contactSystem.ProcessContacts();

    Require(
        antStore.Find(soldierId)
                ->GetEncounterComponent().enemyTimer <
            0.0f,
        "Dead ants should not participate in contacts.");

    std::cout
        << "All ant contact system tests passed.\n";

    return 0;
}

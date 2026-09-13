#include "World/Runtime/AntView.h"
#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"

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

    configuration.antSpeed = 2.0f;

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    AntView firstAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {5.0f, 5.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId firstAntId =
        firstAnt.GetId();

    AntBodySystem physicsWorld(
        antStore,
        configuration);

    physicsWorld.Synchronize();

    Require(
        physicsWorld.GetBodyCount() == 1,
        "Synchronize should create an ant physics body.");

    AntView *storedFirstAnt =
        antStore.Find(firstAntId);

    Require(
        storedFirstAnt != nullptr,
        "First ant should still exist.");

    Require(
        storedFirstAnt->Identity().physicsObjectId !=
            InvalidPhysicsBodyId,
        "The ant should reference its physics body.");

    AntPhysicsBody *firstBody =
        physicsWorld.FindAntBody(
            firstAntId);

    Require(
        firstBody != nullptr,
        "The physics body should be retrievable by AntId.");

    const PhysicsBodyId firstBodyId =
        firstBody->id;

    Require(
        physicsWorld.SetVelocity(
            firstAntId,
            {10.0f, 0.0f}),
        "Velocity should be assigned.");

    physicsWorld.Step(0.5f);
    physicsWorld.SynchronizeAntPositions();

    storedFirstAnt =
        antStore.Find(firstAntId);

    Require(
        NearlyEqual(
            storedFirstAnt
                ->GetPosition()
                .x,
            6.0f),
        "Movement should be limited by configured ant speed.");

    Require(
        NearlyEqual(
            storedFirstAnt
                ->GetPosition()
                .y,
            5.0f),
        "Horizontal movement should preserve Y.");

    firstBody =
        physicsWorld.FindAntBody(
            firstAntId);

    Require(
        NearlyEqual(
            firstBody->velocity.x,
            configuration.antSpeed),
        "Body velocity should match the limited movement.");

    storedFirstAnt->SetState(
        ForagingState::ToHomeWithFood);

    physicsWorld.Synchronize();

    Require(
        NearlyEqual(
            physicsWorld
                .FindAntBody(firstAntId)
                ->mass,
            AntView::FoodMass),
        "Physics mass should update while carrying food.");

    AntView secondAnt =
        antStore.Create(
            1,
            AntRole::Explorer,
            {10.0f, 10.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId secondAntId =
        secondAnt.GetId();

    physicsWorld.Synchronize();

    Require(
        physicsWorld.GetBodyCount() == 2,
        "New ants should receive physics bodies.");

    Require(
        physicsWorld.FindAntBody(
            secondAntId) != nullptr,
        "The second ant body should exist.");

    storedFirstAnt =
        antStore.Find(firstAntId);

    storedFirstAnt->Kill();

    physicsWorld.Synchronize();

    Require(
        physicsWorld.GetBodyCount() == 1,
        "Dead-ant bodies should be removed.");

    Require(
        physicsWorld.FindBody(
            firstBodyId) == nullptr,
        "Removed body IDs should become invalid.");

    Require(
        storedFirstAnt->Identity().physicsObjectId ==
            InvalidPhysicsBodyId,
        "Dead ants should release their physics body ID.");

    Require(
        physicsWorld.Teleport(
            secondAntId,
            {1.0f, 1.0f}),
        "Existing bodies should support teleportation.");

    physicsWorld.Step(0.1f);
    physicsWorld.SynchronizeAntPositions();

    const AntView *storedSecondAnt =
        antStore.Find(secondAntId);

    Require(
        storedSecondAnt
                ->GetPosition()
                .x >=
            2.5f &&
        storedSecondAnt
                ->GetPosition()
                .y >=
            2.5f,
        "Physics bodies should remain inside the guarded border.");

    physicsWorld.Clear();

    Require(
        physicsWorld.GetBodyCount() == 0,
        "Clear should remove every physics body.");

    Require(
        antStore.Find(secondAntId)
                ->Identity().physicsObjectId ==
            InvalidPhysicsBodyId,
        "Clear should release ant physics references.");

    std::cout
        << "All ant physics world tests passed.\n";

    return 0;
}

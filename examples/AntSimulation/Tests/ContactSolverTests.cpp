#include "World/Runtime/AntView.h"
#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/ContactSolver.h"
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

float Distance(
    const pipeframe::Vector2f first,
    const pipeframe::Vector2f second
) {
    const pipeframe::Vector2f difference =
        first - second;

    return std::sqrt(
        difference.x * difference.x +
        difference.y * difference.y);
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

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Environment should initialize.");

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    AntView firstAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {10.0f, 10.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId firstAntId =
        firstAnt.GetId();

    AntView secondAnt =
        antStore.Create(
            2,
            AntRole::Follower,
            {10.5f, 10.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId secondAntId =
        secondAnt.GetId();

    // Give the second ant additional mass.
    antStore.Find(secondAntId)
        ->SetState(
            ForagingState::ToHomeWithFood);

    AntBodySystem physicsWorld(
        antStore,
        configuration);

    physicsWorld.Synchronize();

    ContactSolver contactSolver(
        antStore,
        environment);

    const pipeframe::Vector2f firstBefore =
        antStore.Find(firstAntId)
            ->GetPosition();

    const pipeframe::Vector2f secondBefore =
        antStore.Find(secondAntId)
            ->GetPosition();

    const float distanceBefore =
        Distance(
            firstBefore,
            secondBefore);

    const ContactSolverResult contactResult =
        contactSolver.Solve(
            physicsWorld);

    Require(
        contactResult.antContacts == 1,
        "One overlapping ant pair should be solved.");

    const pipeframe::Vector2f firstAfter =
        antStore.Find(firstAntId)
            ->GetPosition();

    const pipeframe::Vector2f secondAfter =
        antStore.Find(secondAntId)
            ->GetPosition();

    Require(
        Distance(
            firstAfter,
            secondAfter) >
            distanceBefore,
        "Contact solving should separate overlapping ants.");

    const float firstMovement =
        Distance(
            firstBefore,
            firstAfter);

    const float secondMovement =
        Distance(
            secondBefore,
            secondAfter);

    Require(
        firstMovement >
            secondMovement,
        "The lighter ant should receive more displacement.");

    AntView wallAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {14.5f, 15.5f},
            0.0f,
            0.0f,
            configuration);

    const AntId wallAntId =
        wallAnt.GetId();

    Require(
        environment.AddWall(
            {15.5f, 15.5f}),
        "Test wall should be created.");

    physicsWorld.Synchronize();

    Require(
        physicsWorld.SetVelocity(
            wallAntId,
            {configuration.antSpeed, 0.0f}),
        "Wall ant velocity should be assigned.");

    physicsWorld.Step(0.5f);

    const ContactSolverResult wallResult =
        contactSolver.Solve(
            physicsWorld);

    Require(
        wallResult.wallConstraints == 1,
        "Entering a wall should create one wall constraint.");

    const pipeframe::Vector2f wallAntPosition =
        antStore.Find(wallAntId)
            ->GetPosition();

    Require(
        NearlyEqual(
            wallAntPosition.x,
            14.5f),
        "Wall constraint should restore the previous X position.");

    Require(
        NearlyEqual(
            wallAntPosition.y,
            15.5f),
        "Wall constraint should restore the previous Y position.");

    const AntPhysicsBody *wallBody =
        physicsWorld.FindAntBody(
            wallAntId);

    Require(
        wallBody != nullptr,
        "Wall ant physics body should remain valid.");

    Require(
        NearlyEqual(
            wallBody->velocity.x,
            0.0f) &&
        NearlyEqual(
            wallBody->velocity.y,
            0.0f),
        "Wall collision should stop the physics body.");

    for (const auto &body : physicsWorld.GetBodies()) {
        const auto *ant = antStore.Find(body.antId);
        Require(ant && ant->GetPosition() == body.position,
                "Contact pass must publish every final solved body position to the ECS before returning.");
    }

    std::cout
        << "All contact solver tests passed.\n";

    return 0;
}

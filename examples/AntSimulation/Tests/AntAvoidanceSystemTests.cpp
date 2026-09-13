#include "World/Runtime/AntView.h"
#include "World/Physics/AntAvoidanceSystem.h"
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

    configuration.colonyPosition = {
        16.0f,
        16.0f,
    };

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

    AntView secondAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {8.0f, 5.5f},
            AntConfiguration::Pi,
            0.0f,
            configuration);

    const AntId secondAntId =
        secondAnt.GetId();

    AntBodySystem physicsWorld(
        antStore,
        configuration);

    physicsWorld.Synchronize();

    AntPhysicsBody *firstBody =
        physicsWorld.FindAntBody(
            firstAntId);

    AntPhysicsBody *secondBody =
        physicsWorld.FindAntBody(
            secondAntId);

    firstBody->velocity = {
        2.0f,
        0.0f,
    };

    firstBody->direction = {
        1.0f,
        0.0f,
    };

    secondBody->velocity = {
        -2.0f,
        0.0f,
    };

    secondBody->direction = {
        -1.0f,
        0.0f,
    };

    const auto collisionTime =
        AntAvoidanceSystem::
            CalculateTimeToCollision(
                *firstBody,
                *secondBody);

    Require(
        collisionTime.has_value(),
        "Approaching ants should have a collision time.");

    Require(
        *collisionTime > 0.0f &&
        *collisionTime <
            AntAvoidanceSystem::
                MaximumTimeToCollision,
        "Collision should occur inside the avoidance horizon.");

    AntPhysicsBody divergingFirst =
        *firstBody;

    AntPhysicsBody divergingSecond =
        *secondBody;

    divergingFirst.velocity = {
        -2.0f,
        0.0f,
    };

    divergingSecond.velocity = {
        2.0f,
        0.0f,
    };

    Require(
        !AntAvoidanceSystem::
            CalculateTimeToCollision(
                divergingFirst,
                divergingSecond)
             .has_value(),
        "Diverging ants should not produce a collision time.");

    AntAvoidanceSystem avoidanceSystem(
        antStore,
        physicsWorld,
        configuration);

    const pipeframe::Vector2f originalPosition =
        firstBody->position;

    const std::size_t collisionCount =
        avoidanceSystem.Update(0.1f);

    Require(
        collisionCount > 0,
        "Avoidance should detect the approaching ant pair.");

    firstBody =
        physicsWorld.FindAntBody(
            firstAntId);

    Require(
        !NearlyEqual(
            firstBody->position.x,
            originalPosition.x) ||
        !NearlyEqual(
            firstBody->position.y,
            originalPosition.y),
        "Avoidance should apply a steering correction.");

    // Different colonies must not avoid each other.
    antStore.Find(secondAntId)
        ->Identity().colonyId = 2;

    physicsWorld.Synchronize();

    firstBody =
        physicsWorld.FindAntBody(
            firstAntId);

    secondBody =
        physicsWorld.FindAntBody(
            secondAntId);

    firstBody->position = {
        5.0f,
        5.0f,
    };

    firstBody->velocity = {
        2.0f,
        0.0f,
    };

    firstBody->direction = {
        1.0f,
        0.0f,
    };

    secondBody->position = {
        8.0f,
        5.5f,
    };

    secondBody->velocity = {
        -2.0f,
        0.0f,
    };

    secondBody->direction = {
        -1.0f,
        0.0f,
    };

    // Two calls cover both temporal slices.
    const std::size_t differentColonyFirst =
        avoidanceSystem.Update(0.1f);

    const std::size_t differentColonySecond =
        avoidanceSystem.Update(0.1f);

    Require(
        differentColonyFirst == 0 &&
        differentColonySecond == 0,
        "Ants from different colonies should not use friendly avoidance.");

    std::cout
        << "All ant avoidance system tests passed.\n";

    return 0;
}

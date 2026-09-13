#include "AntFixture.h"
#include "World/Runtime/AntView.h"
#include "World/Runtime/Systems/MarkerSampler.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/GridRaycast.h"
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

    configuration.worldSize = {
        24,
        24,
    };

    configuration.colonyPosition = {
        12.0f,
        12.0f,
    };

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Sampler environment should initialize.");

    Require(
        environment.AddWall(
            {16.5f, 12.5f}),
        "Raycast obstacle should be added.");

    const GridRaycastResult wallHit =
        GridRaycast::Cast(
            environment,
            {12.5f, 12.5f},
            {1.0f, 0.0f},
            10.0f);

    Require(
        wallHit.hit,
        "Raycast should detect an obstacle.");

    Require(
        wallHit.cellPosition ==
            pipeframe::Vector2i{16,12},
        "Raycast should identify the obstacle cell.");

    Require(
        NearlyEqual(
            wallHit.distance,
            3.5f),
        "Raycast should report distance to the wall boundary.");

    Require(
        wallHit.normal ==
            pipeframe::Vector2f{-1.0f,0.0f},
        "Raycast should report the wall-facing normal.");

    const GridRaycastResult shortRay =
        GridRaycast::Cast(
            environment,
            {12.5f, 12.5f},
            {1.0f, 0.0f},
            2.0f);

    Require(
        !shortRay.hit,
        "Raycast should ignore walls beyond its maximum distance.");

    Require(
        environment.RemoveWall(
            {16.5f, 12.5f}),
        "Raycast obstacle should be removable.");

    constexpr ColonyId TestColony{1};
    constexpr ColonyId OtherColony{2};

    for (AntWorldCell &cell :
         environment.GetCells()) {
        if (cell.wall) {
            continue;
        }

        cell.AddMarker(
            MarkerKind::ToFood,
            100.0f,
            TestColony);
    }

    AntFixture follower(
        1,
        TestColony,
        AntRole::Follower,
        {12.5f, 12.5f},
        0.0f,
        0.0f,
        configuration);

    MarkerSampler sampler(
        environment,
        configuration);

    Require(
        sampler.GetSampleCount(
            AntRole::Follower) ==
            configuration
                .antFollowerSampleCount,
        "Followers should use the full sample count.");

    Require(
        sampler.GetSampleCount(
            AntRole::Explorer) ==
            configuration
                .antExplorerSampleCount,
        "Explorers should use the reduced sample count.");

    Require(
        sampler.GetSampleCount(
            AntRole::Soldier) ==
            configuration
                .antFollowerSampleCount,
        "Soldiers should use follower sample count.");

    std::mt19937 objectiveRandom{42};

    const NavigationDecision objectiveDecision =
        sampler.SampleWorldIntensity(
            follower,
            objectiveRandom);

    Require(
        objectiveDecision ==
            NavigationDecision::Objective,
        "Owned objective markers should produce an objective decision.");

    Require(
        follower.GetDistanceToTarget() > 0.0f,
        "Objective sampling should assign a target.");

    Require(
        follower.GetDistanceToTarget() <=
            configuration
                .antSamplingDistanceMaximum *
                0.5f,
        "Objective target distance should use AntPezza overshoot protection.");

    AntEnvironment fallbackEnvironment;

    Require(
        fallbackEnvironment.Initialize(
            configuration,
            errorMessage),
        "Fallback environment should initialize.");

    MarkerSampler fallbackSampler(
        fallbackEnvironment,
        configuration);

    AntFixture explorer(
        2,
        TestColony,
        AntRole::Explorer,
        {12.5f, 12.5f},
        0.0f,
        0.0f,
        configuration);

    std::mt19937 fallbackRandom{7};

    const NavigationDecision fallbackDecision =
        fallbackSampler.SampleWorldIntensity(
            explorer,
            fallbackRandom);

    Require(
        fallbackDecision ==
            NavigationDecision::Fallback,
        "An open world without markers should use a fallback sample.");

    Require(
        explorer.GetDistanceToTarget() > 0.0f,
        "Fallback sampling should assign a target.");

    AntFixture exhausted(
        3,
        TestColony,
        AntRole::Follower,
        {12.5f, 12.5f},
        0.0f,
        0.0f,
        configuration);

    exhausted.GetEnergyComponent().current =
        configuration.antMaxEnergy *
            configuration
                .antRefillEnergyRatio -
        1.0f;

    std::mt19937 exhaustedRandom{9};

    const NavigationDecision exhaustedDecision =
        fallbackSampler.SampleWorldIntensity(
            exhausted,
            exhaustedRandom);

    Require(
        exhaustedDecision ==
            NavigationDecision::ReturnHome,
        "An exhausted food-searching ant should return home.");

    Require(
        exhausted.GetState() ==
            ForagingState::ToHomeNoFood,
        "Exhausted ant should enter returning-without-food state.");

    AntFixture otherColonyAnt(
        4,
        OtherColony,
        AntRole::Follower,
        {12.5f, 12.5f},
        0.0f,
        0.0f,
        configuration);

    std::mt19937 otherRandom{42};

    const NavigationDecision otherDecision =
        sampler.SampleWorldIntensity(
            otherColonyAnt,
            otherRandom);

    Require(
        otherDecision ==
            NavigationDecision::Fallback,
        "Ants must not follow another colony's markers.");

    AntWorldCell *foodCell =
        environment.TryGetCell(15, 12);

    Require(
        foodCell != nullptr,
        "Food target cell should exist.");

    foodCell->foodQuantity = 1;

    std::mt19937 foodRandom{100};

    const SampleResult foodSample =
        sampler.GetBestObjectiveSample(
            follower,
            AntConfiguration::Pi * 2.0f,
            1'000,
            foodRandom);

    Require(
        foodSample.IsValid(),
        "Food objective sampling should find a valid target.");

    Require(
        foodSample.earlyStop,
        "Food cells should stop objective searching immediately.");

    std::cout << "All ant marker-sampler tests passed.\n";

    return 0;
}

#include "Configuration/AntConfiguration.h"

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
    using ant_simulation::AntConfiguration;

    AntConfiguration configuration;
    std::string errorMessage;

    Require(
        configuration.Validate(errorMessage),
        "Default AntPezza configuration should be valid.");

    Require(
        errorMessage.empty(),
        "Successful validation should clear the error message.");

    Require(
        configuration.worldSize == pipeframe::Vector2i{384, 216},
        "Default world size should match AntPezza.");

    Require(
        configuration.colonyPosition ==
            pipeframe::Vector2f{40.0f, 40.0f},
        "Default colony position should match AntPezza.");

    Require(
        configuration.colonyInitialAntCount == 1'000,
        "Default colony population should match AntPezza.");

    Require(
        configuration.antFollowerSampleCount == 64,
        "Follower sample count should match AntPezza.");

    Require(
        configuration.antExplorerSampleCount == 8,
        "Explorer sample count should match AntPezza.");

    Require(
        NearlyEqual(
            configuration.antFieldOfView,
            AntConfiguration::DegreesToRadians(135.0f)),
        "Follower field of view should be stored in radians.");

    Require(
        NearlyEqual(
            configuration.antExploreFieldOfView,
            AntConfiguration::DegreesToRadians(90.0f)),
        "Explorer field of view should be stored in radians.");

    Require(
        NearlyEqual(
            configuration.GetAntMarkerInterval(),
            1.5f),
        "Marker interval should be marker distance divided by speed.");

    Require(
        NearlyEqual(
            configuration.GetMarkerMaximumIntensityInverse(),
            0.001f),
        "Marker maximum inverse should match the default maximum.");

    Require(
        configuration.GetWorldSizeFloat() ==
            pipeframe::Vector2f{384.0f, 216.0f},
        "Floating-point world size should match the integer world size.");

    AntConfiguration invalidWorld = configuration;
    invalidWorld.worldSize.x = 0;

    Require(
        !invalidWorld.Validate(errorMessage),
        "A zero-width world should be rejected.");

    Require(
        !errorMessage.empty(),
        "Invalid configuration should provide an error message.");

    AntConfiguration invalidProbability = configuration;
    invalidProbability.explorerProbability = 1.5f;

    Require(
        !invalidProbability.Validate(errorMessage),
        "Explorer probability above one should be rejected.");

    AntConfiguration invalidSampling = configuration;
    invalidSampling.antSamplingDistanceMinimum = 10.0f;
    invalidSampling.antSamplingDistanceMaximum = 5.0f;

    Require(
        !invalidSampling.Validate(errorMessage),
        "Maximum sampling distance below the minimum should be rejected.");

    AntConfiguration invalidWorkers = configuration;
    invalidWorkers.antUpdateWorkerCount = 0;

    Require(
        !invalidWorkers.Validate(errorMessage),
        "A zero-worker ant update mode should be rejected.");

    AntConfiguration invalidColony = configuration;
    invalidColony.colonyPosition = {
        500.0f,
        40.0f,
    };

    Require(
        !invalidColony.Validate(errorMessage),
        "A default colony outside the world should be rejected.");

    std::cout << "All ant configuration tests passed.\n";

    return 0;
}

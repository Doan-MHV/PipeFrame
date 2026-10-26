#include "SailBoatConfiguration.h"

#include <cmath>

namespace sailboat_simulation {
namespace {

bool IsFinitePositive(const float value) {
    return std::isfinite(value) && value > 0.0f;
}

bool IsProbability(const float value) {
    return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
}

} // namespace

bool SailBoatConfiguration::Validate(std::string &errorMessage) const {
    errorMessage.clear();

    if (populationSize == 0) {
        errorMessage = "Population size must be greater than zero.";
        return false;
    }

    if (!IsFinitePositive(maximumIterationTime)) {
        errorMessage = "Maximum iteration time must be finite and greater than zero.";
        return false;
    }

    if (!IsProbability(eliteRatio) || !IsProbability(newNodeProbability) ||
        !IsProbability(newConnectionProbability) || !IsProbability(newValueProbability)) {
        errorMessage = "Training ratios and mutation probabilities must be between zero and one.";
        return false;
    }

    if (!IsFinitePositive(weightRange) || !IsFinitePositive(smallWeightRange)) {
        errorMessage = "Mutation weight ranges must be finite and greater than zero.";
        return false;
    }

    if (mutationCount == 0 || maximumHiddenNodes == 0 || bestSavePeriod == 0) {
        errorMessage = "Mutation, hidden-node, and save-period counts must be greater than zero.";
        return false;
    }

    if (maximumWaypointCount == 0 || !IsFinitePositive(waypointRadius)) {
        errorMessage = "Waypoint capacity and radius must be greater than zero.";
        return false;
    }

    if (!IsFinitePositive(worldSize.x) || !IsFinitePositive(worldSize.y)) {
        errorMessage = "World dimensions must be finite and greater than zero.";
        return false;
    }

    if (!IsFinitePositive(simulationSpeedUp) || !IsFinitePositive(angularSpeedDegrees)) {
        errorMessage = "Simulation and angular speeds must be finite and greater than zero.";
        return false;
    }

    if (!std::isfinite(wind.x) || !std::isfinite(wind.y)) {
        errorMessage = "Wind must contain finite components.";
        return false;
    }

    if (!std::isfinite(audioVolume) || audioVolume < 0.0f || audioVolume > 100.0f) {
        errorMessage = "Audio volume must be between zero and 100.";
        return false;
    }

    return true;
}

} // namespace sailboat_simulation

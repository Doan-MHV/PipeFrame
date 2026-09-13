#include "Configuration/AntConfiguration.h"

#include <cmath>
#include <string>

namespace ant_simulation {

namespace {

bool IsFinitePositive(const float value) {
    return std::isfinite(value) && value > 0.0f;
}

bool IsFiniteNonNegative(const float value) {
    return std::isfinite(value) && value >= 0.0f;
}

bool IsProbability(const float value) {
    return std::isfinite(value) &&
           value >= 0.0f &&
           value <= 1.0f;
}

} // namespace

pipeframe::Vector2f AntConfiguration::GetWorldSizeFloat() const {
    return {
        static_cast<float>(worldSize.x),
        static_cast<float>(worldSize.y),
    };
}

float AntConfiguration::GetMarkerMaximumIntensityInverse() const {
    if (markerMaxIntensity <= 0.0f) {
        return 0.0f;
    }

    return 1.0f / markerMaxIntensity;
}

float AntConfiguration::GetAntMarkerInterval() const {
    if (antSpeed <= 0.0f) {
        return 0.0f;
    }

    return antMarkerDistance / antSpeed;
}

bool AntConfiguration::Validate(
    std::string &errorMessage
) const {
    errorMessage.clear();

    if (worldSize.x <= 0 || worldSize.y <= 0) {
        errorMessage =
            "Ant world dimensions must both be greater than zero.";

        return false;
    }



    if (!IsFiniteNonNegative(markerDecayRate)) {
        errorMessage =
            "Marker decay rate must be finite and non-negative.";

        return false;
    }

    if (!IsFinitePositive(markerMaxIntensity)) {
        errorMessage =
            "Marker maximum intensity must be finite and greater than zero.";

        return false;
    }

    if (!IsFiniteNonNegative(markerIntensityThreshold) ||
        markerIntensityThreshold > markerMaxIntensity) {
        errorMessage =
            "Marker intensity threshold must be non-negative and no greater "
            "than the marker maximum intensity.";

        return false;
    }

    if (!IsProbability(markerSamplingDegradation)) {
        errorMessage =
            "Marker sampling degradation must be between zero and one.";

        return false;
    }

    if (!IsFinitePositive(antMaxEnergy)) {
        errorMessage =
            "Ant maximum energy must be finite and greater than zero.";

        return false;
    }

    if (!IsProbability(antRefillEnergyRatio)) {
        errorMessage =
            "Ant refill energy ratio must be between zero and one.";

        return false;
    }

    if (!IsFinitePositive(antSpeed)) {
        errorMessage =
            "Ant speed must be finite and greater than zero.";

        return false;
    }

    if (!IsFinitePositive(antCost)) {
        errorMessage =
            "Ant cost must be finite and greater than zero.";

        return false;
    }

    if (!IsFinitePositive(antSoldierCost)) {
        errorMessage =
            "Ant soldier cost must be finite and greater than zero.";

        return false;
    }

    if (!IsFinitePositive(antFieldOfView) ||
        antFieldOfView > 2.0f * Pi) {
        errorMessage =
            "Ant field of view must be greater than zero and no greater "
            "than 360 degrees.";

        return false;
    }

    if (!IsFinitePositive(antExploreFieldOfView) ||
        antExploreFieldOfView > 2.0f * Pi) {
        errorMessage =
            "Explorer field of view must be greater than zero and no greater "
            "than 360 degrees.";

        return false;
    }

    if (!IsFinitePositive(antMarkerDistance)) {
        errorMessage =
            "Ant marker distance must be finite and greater than zero.";

        return false;
    }

    if (antFollowerSampleCount == 0) {
        errorMessage =
            "Follower marker sample count must be greater than zero.";

        return false;
    }

    if (antExplorerSampleCount == 0) {
        errorMessage =
            "Explorer marker sample count must be greater than zero.";

        return false;
    }

    if (!IsFiniteNonNegative(antSamplingDistanceMinimum)) {
        errorMessage =
            "Minimum sampling distance must be finite and non-negative.";

        return false;
    }

    if (!IsFinitePositive(antSamplingDistanceMaximum) ||
        antSamplingDistanceMaximum <
            antSamplingDistanceMinimum) {
        errorMessage =
            "Maximum sampling distance must be greater than or equal to the "
            "minimum sampling distance.";

        return false;
    }

    if (antUpdateWorkerCount == 0 ||
        antUpdateWorkerCount > 64) {
        errorMessage =
            "Ant update worker count must be between one and 64.";

        return false;
    }

    if (!std::isfinite(colonyPosition.x) ||
        !std::isfinite(colonyPosition.y)) {
        errorMessage =
            "Colony position must contain finite coordinates.";

        return false;
    }

    if (colonyPosition.x < 0.0f ||
        colonyPosition.y < 0.0f ||
        colonyPosition.x >= static_cast<float>(worldSize.x) ||
        colonyPosition.y >= static_cast<float>(worldSize.y)) {
        errorMessage =
            "Default colony position must be inside the ant world.";

        return false;
    }

    if (!IsFinitePositive(colonyRadius)) {
        errorMessage =
            "Colony radius must be finite and greater than zero.";

        return false;
    }

    if (colonyInitialAntCount == 0) {
        errorMessage =
            "Initial colony ant count must be greater than zero.";

        return false;
    }

    if (!IsProbability(explorerProbability)) {
        errorMessage =
            "Explorer probability must be between zero and one.";

        return false;
    }

    if (!IsFinitePositive(detailZoomThreshold)) {
        errorMessage =
            "Detailed rendering zoom threshold must be finite and greater "
            "than zero.";

        return false;
    }

    if (!IsFiniteNonNegative(farAntScaleBoost)) {
        errorMessage =
            "Far ant scale boost must be finite and non-negative.";

        return false;
    }

    return true;
}

} // namespace ant_simulation

#include "World/Runtime/Systems/MarkerSampler.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/GridRaycast.h"
#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

MarkerSampler::MarkerSampler(
    const AntEnvironment &sourceEnvironment,
    const AntConfiguration &sourceConfiguration
)
    : environment(sourceEnvironment),
      configuration(sourceConfiguration) {
}

std::size_t MarkerSampler::GetSampleCount(
    const AntRole role
) const {
    switch (role) {
        case AntRole::Follower:
        case AntRole::Soldier:
            return configuration
                .antFollowerSampleCount;

        case AntRole::Explorer:
            return configuration
                .antExplorerSampleCount;
    }

    return 0;
}

NavigationDecision
MarkerSampler::SampleWorldIntensity(
    AntView &ant,
    std::mt19937 &randomGenerator
) const {
    constexpr float TargetDistanceCoefficient{
        0.5f
    };

    const float fieldOfViewCoefficient =
        ant.GetForagingComponent().blocked
            ? 2.0f
            : 1.0f;

    const std::size_t sampleCount =
        GetSampleCount(
            ant.GetRole());

    const SampleResult objective =
        GetBestObjectiveSample(
            ant,
            configuration.antFieldOfView *
                fieldOfViewCoefficient,
            sampleCount,
            randomGenerator);

    if (objective.IsValid()) {
        ant.SetTarget(
            objective.position,
            objective.distance *
                TargetDistanceCoefficient);

        return NavigationDecision::Objective;
    }

    if (ant.GetState() ==
            ForagingState::ToFood &&
        ant.GetEnergy() <
            configuration.antMaxEnergy *
                configuration
                    .antRefillEnergyRatio) {
        ant.SetState(
            ForagingState::ToHomeNoFood);

        return NavigationDecision::ReturnHome;
    }

    const SampleResult fallback =
        GetBestFallbackSample(
            ant,
            configuration
                    .antExploreFieldOfView *
                fieldOfViewCoefficient,
            sampleCount,
            randomGenerator);

    if (fallback.IsValid()) {
        ant.SetTarget(
            fallback.position,
            fallback.distance *
                TargetDistanceCoefficient);

        return NavigationDecision::Fallback;
    }

    const float angleWidth =
        AntConfiguration::Pi *
        fieldOfViewCoefficient;

    const float targetAngle =
        ant.GetAngle() +
        RandomRange(
            randomGenerator,
            -angleWidth * 0.5f,
            angleWidth * 0.5f);

    const float targetDistance =
        RandomRange(
            randomGenerator,
            0.0f,
            configuration
                .antSamplingDistanceMaximum);

    const pipeframe::Vector2f targetDirection{
        std::cos(targetAngle),
        std::sin(targetAngle),
    };

    ant.SetTarget(
        ant.GetPosition() +
            targetDirection *
                targetDistance,
        targetDistance);

    return NavigationDecision::Random;
}

SampleResult MarkerSampler::GetSample(
    const AntView &ant,
    const float fieldOfView,
    std::mt19937 &randomGenerator
) const {
    SampleResult result;

    const float halfFieldOfView =
        fieldOfView * 0.5f;

    result.angle =
        ant.GetAngle() +
        RandomRange(
            randomGenerator,
            -halfFieldOfView,
            halfFieldOfView);

    result.direction = {
        std::cos(result.angle),
        std::sin(result.angle),
    };

    const float minimumDistanceSquared =
        configuration
            .antSamplingDistanceMinimum *
        configuration
            .antSamplingDistanceMinimum;

    const float maximumDistanceSquared =
        configuration
            .antSamplingDistanceMaximum *
        configuration
            .antSamplingDistanceMaximum;

    const float distanceSquared =
        RandomRange(
            randomGenerator,
            minimumDistanceSquared,
            maximumDistanceSquared);

    result.distance =
        std::sqrt(distanceSquared);

    const GridRaycastResult raycast =
        GridRaycast::Cast(
            environment,
            {ant.GetPosition().x,ant.GetPosition().y},
            {result.direction.x,result.direction.y},
            result.distance);

    if (raycast.hit) {
        return {};
    }

    result.position =
        ant.GetPosition() +
        result.direction *
            result.distance;

    if (!environment
             .IsSimulationPositionValid(
                 result.position)) {
        return {};
    }

    result.cell =
        environment
            .TryGetCellAtWorldPosition(
                result.position);

    return result;
}

SampleResult MarkerSampler::GetValidSample(
    const AntView &ant,
    const float fieldOfView,
    const std::size_t sampleCount,
    std::mt19937 &randomGenerator
) const {
    for (std::size_t index = 0;
         index < sampleCount;
         ++index) {
        SampleResult sample =
            GetSample(
                ant,
                fieldOfView,
                randomGenerator);

        if (!sample.IsValid() ||
            sample.cell->wall) {
            continue;
        }

        return sample;
    }

    return {};
}

SampleResult
MarkerSampler::GetBestObjectiveSample(
    const AntView &ant,
    const float fieldOfView,
    const std::size_t sampleCount,
    std::mt19937 &randomGenerator
) const {
    const MarkerKind markerFocus =
        ant.GetMarkerFocus();

    SampleResult best;

    for (std::size_t index = 0;
         index < sampleCount;
         ++index) {
        SampleResult sample =
            GetSample(
                ant,
                fieldOfView,
                randomGenerator);

        if (!sample.IsValid() ||
            sample.cell->wall) {
            continue;
        }

        const Marker &objectiveMarker =
            sample.cell->GetMarker(
                markerFocus);

        if (objectiveMarker.colonyId !=
            ant.GetColonyId()) {
            continue;
        }

        if (markerFocus ==
                MarkerKind::ToFood &&
            sample.cell->foodQuantity > 0) {
            sample.earlyStop = true;
            return sample;
        }

        const float markerIntensity =
            objectiveMarker.intensity *
            sample.cell
                ->markerSamplingCoefficient;

        if (markerIntensity >
            best.intensity) {
            best = sample;
            best.intensity =
                markerIntensity;
        }
    }

    return best;
}

SampleResult
MarkerSampler::GetBestFallbackSample(
    const AntView &ant,
    const float fieldOfView,
    const std::size_t sampleCount,
    std::mt19937 &randomGenerator
) const {
    SampleResult best;

    for (std::size_t index = 0;
         index < sampleCount;
         ++index) {
        const SampleResult sample =
            GetSample(
                ant,
                fieldOfView,
                randomGenerator);

        if (!sample.IsValid() ||
            sample.cell->wall) {
            continue;
        }

        if (sample.distance >
            best.distance) {
            best = sample;
        }
    }

    return best;
}

float MarkerSampler::RandomRange(
    std::mt19937 &randomGenerator,
    const float minimum,
    const float maximum
) {
    if (maximum <= minimum) {
        return minimum;
    }

    std::uniform_real_distribution<float>
        distribution(
            minimum,
            maximum);

    return distribution(
        randomGenerator);
}

} // namespace ant_simulation

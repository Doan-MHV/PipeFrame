#ifndef ANT_MARKER_SAMPLER_H
#define ANT_MARKER_SAMPLER_H

#include <cstddef>
#include <random>

#include "World/Runtime/AntView.h"
#include "Components/AntIdentityComponent.h"
#include "World/Runtime/Systems/SampleResult.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {

enum class NavigationDecision {
    Objective,
    Fallback,
    ReturnHome,
    Random,
};

class MarkerSampler {
public:
    MarkerSampler(
        const AntEnvironment &environment,
        const AntConfiguration &configuration
    );

    [[nodiscard]]
    std::size_t GetSampleCount(
        AntRole role
    ) const;

    NavigationDecision SampleWorldIntensity(
        AntView &ant,
        std::mt19937 &randomGenerator
    ) const;

    [[nodiscard]]
    SampleResult GetSample(
        const AntView &ant,
        float fieldOfView,
        std::mt19937 &randomGenerator
    ) const;

    [[nodiscard]]
    SampleResult GetValidSample(
        const AntView &ant,
        float fieldOfView,
        std::size_t sampleCount,
        std::mt19937 &randomGenerator
    ) const;

    [[nodiscard]]
    SampleResult GetBestObjectiveSample(
        const AntView &ant,
        float fieldOfView,
        std::size_t sampleCount,
        std::mt19937 &randomGenerator
    ) const;

    [[nodiscard]]
    SampleResult GetBestFallbackSample(
        const AntView &ant,
        float fieldOfView,
        std::size_t sampleCount,
        std::mt19937 &randomGenerator
    ) const;

private:
    [[nodiscard]]
    static float RandomRange(
        std::mt19937 &randomGenerator,
        float minimum,
        float maximum
    );

    const AntEnvironment &environment;
    const AntConfiguration &configuration;
};

} // namespace ant_simulation

#endif
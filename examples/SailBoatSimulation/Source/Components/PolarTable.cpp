#include "PolarTable.h"

#include <algorithm>
#include <cmath>

namespace sailboat_simulation {
namespace {

constexpr std::array<float, PolarTable::SampleCount> Samples{
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.8f, 1.6f, 2.3f, 3.1f, 3.9f, 4.0f, 4.1f, 4.3f, 4.4f,
    4.5f, 4.6f, 4.7f, 4.7f, 4.8f, 4.9f, 5.0f, 5.0f, 5.1f, 5.1f,
    5.2f, 5.2f, 5.3f, 5.3f, 5.4f, 5.4f, 5.4f, 5.5f, 5.5f, 5.6f,
    5.6f, 5.6f, 5.6f, 5.7f, 5.7f, 5.7f, 5.7f, 5.7f, 5.8f, 5.8f,
    5.8f, 5.8f, 5.8f, 5.8f, 5.8f, 5.8f, 5.9f, 5.9f, 5.9f, 5.9f,
    5.9f, 5.9f, 5.9f, 5.9f, 5.9f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f,
    6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.1f, 6.1f, 6.1f, 6.1f,
    6.1f, 6.1f, 6.1f, 6.1f, 6.1f, 6.1f, 6.2f, 6.2f, 6.2f, 6.2f,
    6.2f, 6.2f, 6.2f, 6.2f, 6.2f, 6.2f, 6.2f, 6.3f, 6.3f, 6.3f,
    6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f,
    6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.2f, 6.2f, 6.2f, 6.2f, 6.2f,
    6.2f, 6.2f, 6.2f, 6.1f, 6.1f, 6.1f, 6.1f, 6.1f, 6.0f, 6.0f,
    6.0f, 6.0f, 5.9f, 5.9f, 5.9f, 5.8f, 5.8f, 5.8f, 5.8f, 5.7f,
    5.7f, 5.7f, 5.6f, 5.5f, 5.5f, 5.5f, 5.4f, 5.4f, 5.3f, 5.2f,
    5.2f, 5.2f, 5.1f, 5.1f, 5.0f, 5.0f, 5.0f, 4.9f, 4.9f, 4.8f,
    4.8f,
};

} // namespace

float PolarTable::GetSpeedKnots(const float absoluteWindAngleDegrees) {
    const float index = std::clamp(std::abs(absoluteWindAngleDegrees), 0.0f, 180.0f);
    const std::size_t lower = static_cast<std::size_t>(std::floor(index));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(index));
    const float fraction = index - static_cast<float>(lower);
    return Samples[lower] * (1.0f - fraction) + Samples[upper] * fraction;
}

const std::array<float, PolarTable::SampleCount> &PolarTable::GetSamples() { return Samples; }

} // namespace sailboat_simulation

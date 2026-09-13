#ifndef SAILBOAT_POLAR_TABLE_H
#define SAILBOAT_POLAR_TABLE_H

#include <array>

namespace sailboat_simulation {

class PolarTable final {
public:
    static constexpr float ReferenceWindSpeedKnots = 10.0f;
    static constexpr std::size_t SampleCount = 181;

    [[nodiscard]] static float GetSpeedKnots(float absoluteWindAngleDegrees);
    [[nodiscard]] static const std::array<float, SampleCount> &GetSamples();
};

} // namespace sailboat_simulation

#endif

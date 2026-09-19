#include "World/Runtime/ColonyHistory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ant_simulation {

ColonyHistory::ColonyHistory(const std::size_t newMaximumSamples, const float newSamplePeriod)
    : SampleHistory(newMaximumSamples), samplePeriod(std::max(0.001f, newSamplePeriod)) {}

void ColonyHistory::Reset() {
    ClearSamples();

    sampleAccumulator = 0.0f;
    elapsedTime = 0.0f;
}

void ColonyHistory::Update(const ColonyView &colony, const float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) {

        return;
    }

    sampleAccumulator += deltaTime;
    elapsedTime += deltaTime;

    while (sampleAccumulator >= samplePeriod) {
        sampleAccumulator -= samplePeriod;

        AddSample(colony);
    }
}

std::span<const ColonyHistorySample> ColonyHistory::GetSamples() const { return Samples(); }

std::size_t ColonyHistory::GetMaximumSamples() const { return Capacity(); }

float ColonyHistory::GetSamplePeriod() const { return samplePeriod; }

void ColonyHistory::AddSample(const ColonyView &colony) {
    PushSample({
        .elapsedTime = elapsedTime,

        .antCount = colony.GetAntCount(),

        .foodQuantity = colony.GetFoodQuantity(),

        .collectionRate = colony.GetCollectionRate(),

        .reserve = colony.GetReserve(),
    });
}

} // namespace ant_simulation

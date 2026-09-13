#ifndef ANT_COLONY_HISTORY_H
#define ANT_COLONY_HISTORY_H

#include <cstddef>
#include <span>
#include <PipeFrame/Data/SampleHistory.h>

#include "World/Runtime/ColonyView.h"

namespace ant_simulation {

struct ColonyHistorySample {
    float elapsedTime{0.0f};

    std::size_t antCount{0};

    float foodQuantity{0.0f};
    float collectionRate{0.0f};
    float reserve{0.0f};
};

class ColonyHistory final : public pipeframe::SampleHistory<ColonyHistorySample> {
public:
    explicit ColonyHistory(
        std::size_t maximumSamples = 240,
        float samplePeriod = 0.1f
    );

    void Reset();

    void Update(
        const ColonyView &colony,
        float deltaTime
    );

    [[nodiscard]]
    std::span<const ColonyHistorySample>
    GetSamples() const;

    [[nodiscard]]
    std::size_t GetMaximumSamples() const;

    [[nodiscard]]
    float GetSamplePeriod() const;

private:
    void AddSample(const ColonyView &colony);

    float samplePeriod{0.1f};
    float sampleAccumulator{0.0f};
    float elapsedTime{0.0f};
};

} // namespace ant_simulation

#endif

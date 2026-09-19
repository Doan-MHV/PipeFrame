#ifndef PIPEFRAME_DATA_SAMPLE_HISTORY_H
#define PIPEFRAME_DATA_SAMPLE_HISTORY_H

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace pipeframe {

template <typename Sample>
class SampleHistory {
public:
    explicit SampleHistory(const std::size_t capacity = 240) : capacity(std::max<std::size_t>(2, capacity)) {}
    virtual ~SampleHistory() = default;
    void ClearSamples() { samples.clear(); }
    [[nodiscard]] std::span<const Sample> Samples() const { return samples; }
    [[nodiscard]] std::size_t Capacity() const { return capacity; }

protected:
    void PushSample(Sample value) {
        samples.push_back(std::move(value));
        if (samples.size() > capacity)
            samples.erase(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(samples.size() - capacity));
    }

private:
    std::size_t capacity;
    std::vector<Sample> samples;
};

}  // namespace pipeframe

#endif

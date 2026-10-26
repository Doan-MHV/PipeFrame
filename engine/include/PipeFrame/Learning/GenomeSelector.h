#ifndef PIPEFRAME_LEARNING_GENOME_SELECTOR_H
#define PIPEFRAME_LEARNING_GENOME_SELECTOR_H

#include <PipeFrame/Core/DeterministicRandom.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace pipeframe::learning {

class GenomeSelector final {
public:
    explicit GenomeSelector(std::uint32_t seed);

    void SetScores(std::span<const float> scores);
    [[nodiscard]] std::size_t Pick();
    [[nodiscard]] std::vector<std::size_t> RankDescending() const;
    [[nodiscard]] std::span<const float> GetCumulativeWeights() const;

private:
    std::vector<float> scores;
    std::vector<float> cumulativeWeights;
    DeterministicRandom random;
};

}  // namespace pipeframe::learning

#endif

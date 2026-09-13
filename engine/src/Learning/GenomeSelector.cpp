#include <PipeFrame/Learning/GenomeSelector.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace pipeframe::learning {

GenomeSelector::GenomeSelector(const std::uint32_t seed) : random(seed) {}

void GenomeSelector::SetScores(const std::span<const float> newScores) {
    scores.assign(newScores.begin(), newScores.end());
    cumulativeWeights.clear();
    if (scores.empty()) {
        return;
    }

    float minimum = 0.0f;
    for (const float score : scores) {
        if (std::isfinite(score)) {
            minimum = std::min(minimum, score);
        }
    }

    std::vector<float> weights(scores.size(), 0.0f);
    float total = 0.0f;
    for (std::size_t index = 0; index < scores.size(); ++index) {
        weights[index] = std::isfinite(scores[index]) ? std::max(0.0f, scores[index] - minimum) : 0.0f;
        total += weights[index];
    }
    if (total <= 0.0f) {
        std::fill(weights.begin(), weights.end(), 1.0f);
        total = static_cast<float>(weights.size());
    }

    cumulativeWeights.reserve(weights.size());
    float cumulative = 0.0f;
    for (const float weight : weights) {
        cumulative += weight / total;
        cumulativeWeights.push_back(cumulative);
    }
    cumulativeWeights.back() = 1.0f;
}

std::size_t GenomeSelector::Pick() {
    if (cumulativeWeights.empty()) {
        return 0;
    }
    const float threshold = random.NextFloat();
    const auto iterator = std::lower_bound(cumulativeWeights.begin(), cumulativeWeights.end(), threshold);
    return iterator == cumulativeWeights.end()
               ? cumulativeWeights.size() - 1
               : static_cast<std::size_t>(std::distance(cumulativeWeights.begin(), iterator));
}

std::vector<std::size_t> GenomeSelector::RankDescending() const {
    std::vector<std::size_t> indices(scores.size());
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    std::stable_sort(indices.begin(), indices.end(), [this](const std::size_t left, const std::size_t right) {
        const float leftScore = std::isfinite(scores[left]) ? scores[left] : -INFINITY;
        const float rightScore = std::isfinite(scores[right]) ? scores[right] : -INFINITY;
        return leftScore > rightScore;
    });
    return indices;
}

std::span<const float> GenomeSelector::GetCumulativeWeights() const { return cumulativeWeights; }

} // namespace pipeframe::learning

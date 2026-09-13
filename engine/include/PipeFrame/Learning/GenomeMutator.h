#ifndef PIPEFRAME_LEARNING_GENOME_MUTATOR_H
#define PIPEFRAME_LEARNING_GENOME_MUTATOR_H

#include <cstddef>
#include <cstdint>

#include <PipeFrame/Core/DeterministicRandom.h>
#include <PipeFrame/Learning/Genome.h>

namespace pipeframe::learning {

struct MutationSettings {
    float newNodeProbability{0.1f};
    float newConnectionProbability{0.8f};
    float newValueProbability{0.1f};
    float weightRange{2.0f};
    float smallWeightRange{0.01f};
    std::uint32_t mutationCount{4};
    std::uint32_t maximumHiddenNodes{100};
};

class GenomeMutator final {
  public:
    GenomeMutator(MutationSettings settings, std::uint64_t seed);

    void Mutate(Genome &genome);
    bool AddNode(Genome &genome);
    bool AddConnection(Genome &genome);
    bool MutateBias(Genome &genome);
    bool MutateWeight(Genome &genome);

  private:
    [[nodiscard]] bool Chance(float probability);
    [[nodiscard]] float FullRange(float magnitude);
    [[nodiscard]] std::size_t Index(std::size_t count);

    MutationSettings settings;
    DeterministicRandom random;
};

} // namespace pipeframe::learning

#endif

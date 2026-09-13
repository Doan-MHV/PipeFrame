#include <PipeFrame/Learning/GenomeMutator.h>

#include <algorithm>

namespace pipeframe::learning {

GenomeMutator::GenomeMutator(const MutationSettings newSettings, const std::uint64_t seed)
    : settings(newSettings), random(seed) {}

void GenomeMutator::Mutate(Genome &genome) {
    for (std::uint32_t mutation = 0; mutation < settings.mutationCount; ++mutation) {
        if (Chance(0.25f)) {
            Chance(0.5f) ? static_cast<void>(MutateBias(genome))
                         : static_cast<void>(MutateWeight(genome));
        }
    }
    if (genome.GetHiddenCount() < settings.maximumHiddenNodes && Chance(settings.newNodeProbability)) {
        AddNode(genome);
    }
    if (Chance(settings.newConnectionProbability)) {
        AddConnection(genome);
    }
}

bool GenomeMutator::AddNode(Genome &genome) {
    return !genome.GetConnections().empty() && genome.SplitConnection(Index(genome.GetConnections().size()));
}

bool GenomeMutator::AddConnection(Genome &genome) {
    const std::size_t inputCount = genome.GetInputCount();
    const std::size_t outputCount = genome.GetOutputCount();
    const std::size_t hiddenCount = genome.GetHiddenCount();
    const std::size_t sourceCount = inputCount + hiddenCount;
    const std::size_t targetCount = outputCount + hiddenCount;
    if (sourceCount == 0 || targetCount == 0) {
        return false;
    }

    std::size_t from = Index(sourceCount);
    if (from >= inputCount) {
        from += outputCount;
    }
    const std::size_t to = inputCount + Index(targetCount);
    return genome.AddConnection(from, to, FullRange(settings.weightRange));
}

bool GenomeMutator::MutateBias(Genome &genome) {
    if (genome.GetNodes().empty()) {
        return false;
    }
    GenomeNode &node = genome.GetNodes()[Index(genome.GetNodes().size())];
    if (Chance(settings.newValueProbability)) {
        node.bias = FullRange(settings.weightRange);
    } else if (Chance(0.25f)) {
        node.bias += FullRange(settings.weightRange);
    } else {
        node.bias += settings.smallWeightRange * FullRange(settings.weightRange);
    }
    return true;
}

bool GenomeMutator::MutateWeight(Genome &genome) {
    if (genome.GetConnections().empty()) {
        return false;
    }
    GenomeConnection &connection = genome.GetConnections()[Index(genome.GetConnections().size())];
    if (Chance(settings.newValueProbability)) {
        connection.weight = FullRange(settings.weightRange);
    } else if (Chance(0.75f)) {
        connection.weight += settings.smallWeightRange * FullRange(settings.weightRange);
    } else {
        connection.weight += FullRange(settings.weightRange);
    }
    return true;
}

bool GenomeMutator::Chance(const float probability) {
    return random.NextFloat() < std::clamp(probability, 0.0f, 1.0f);
}

float GenomeMutator::FullRange(const float magnitude) {
    return (random.NextFloat() * 2.0f - 1.0f) * magnitude;
}

std::size_t GenomeMutator::Index(const std::size_t count) {
    return count == 0 ? 0 : static_cast<std::size_t>(random.NextU64() % count);
}

} // namespace pipeframe::learning

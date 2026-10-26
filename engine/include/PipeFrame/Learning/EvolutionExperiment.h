#ifndef PIPEFRAME_LEARNING_EVOLUTION_EXPERIMENT_H
#define PIPEFRAME_LEARNING_EVOLUTION_EXPERIMENT_H

#include <PipeFrame/Learning/ExperimentRuntime.h>
#include <PipeFrame/Learning/GenomeMutator.h>
#include <PipeFrame/Learning/Network.h>
#include <PipeFrame/Learning/TrainingInterfaces.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pipeframe::learning {

struct EvolutionExperimentConfig {
    std::size_t inputCount{1};
    std::size_t outputCount{1};
    std::size_t population{32};
    float eliteRatio{0.2f};
    std::uint64_t seed{1};
    MutationSettings mutation;
};

using GenomeEvaluator = std::function<float(Network& network, std::size_t populationIndex)>;

class EvolutionExperiment {
public:
    bool Initialize(EvolutionExperimentConfig configuration, std::string& errorMessage);
    bool RunGeneration(const GenomeEvaluator& evaluator, std::string& errorMessage);
    void StartNewRun();
    void Clear();

    bool SaveCheckpoint(const std::filesystem::path& directory, std::string& errorMessage) const;
    bool LoadCheckpoint(const std::filesystem::path& directory, std::string& errorMessage);

    [[nodiscard]] std::uint32_t Run() const;
    [[nodiscard]] std::uint32_t Generation() const;
    [[nodiscard]] std::span<const Genome> Population() const;
    [[nodiscard]] const Genome* BestGenome() const;
    [[nodiscard]] float BestScore() const;
    [[nodiscard]] const ExperimentHistory& History() const;
    [[nodiscard]] std::optional<InferenceSnapshot> BestInferenceSnapshot(std::string* errorMessage = nullptr) const;
    [[nodiscard]] ExperimentStatistics Statistics() const;

private:
    bool CreateInitialPopulation(std::string& errorMessage);
    bool Evolve(std::span<const float> scores, std::string& errorMessage);

    EvolutionExperimentConfig configuration;
    std::vector<Genome> population;
    ExperimentHistory history;
    std::optional<Genome> bestGenome;
    std::uint32_t run{};
    std::uint32_t generation{};
    float bestScore{};
    float averageScore{};
    bool initialized{};
};

}  // namespace pipeframe::learning

#endif

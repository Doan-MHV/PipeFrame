#include <PipeFrame/Learning/EvolutionExperiment.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>

#include <PipeFrame/Learning/GenomeSelector.h>
#include <PipeFrame/Learning/NetworkGenerator.h>

namespace pipeframe::learning {

bool EvolutionExperiment::Initialize(EvolutionExperimentConfig newConfiguration, std::string &errorMessage) {
    Clear();
    if (newConfiguration.inputCount == 0 || newConfiguration.outputCount == 0 || newConfiguration.population == 0 ||
        newConfiguration.population > 1'000'000 || !std::isfinite(newConfiguration.eliteRatio) ||
        newConfiguration.eliteRatio <= 0.0f || newConfiguration.eliteRatio > 1.0f) {
        errorMessage = "Invalid evolution experiment configuration.";
        return false;
    }
    configuration = newConfiguration;
    if (!CreateInitialPopulation(errorMessage))
        return false;
    initialized = true;
    errorMessage.clear();
    return true;
}

bool EvolutionExperiment::RunGeneration(const GenomeEvaluator &evaluator, std::string &errorMessage) {
    if (!initialized || !evaluator) {
        errorMessage = "Evolution experiment is not initialized.";
        return false;
    }
    std::vector<float> scores;
    scores.reserve(population.size());
    std::size_t bestIndex{};
    float generationBest = -std::numeric_limits<float>::infinity();
    float total{};
    for (std::size_t index = 0; index < population.size(); ++index) {
        auto network = NetworkGenerator::Generate(population[index], &errorMessage);
        if (!network)
            return false;
        float score = evaluator(*network, index);
        if (!std::isfinite(score))
            score = -std::numeric_limits<float>::max();
        scores.push_back(score);
        total += score;
        if (score > generationBest) {
            generationBest = score;
            bestIndex = index;
        }
    }
    averageScore = total / static_cast<float>(scores.size());
    if (!bestGenome || generationBest >= bestScore) {
        bestGenome = population[bestIndex];
        bestScore = generationBest;
    }
    history.Add({run,
                 generation,
                 generationBest,
                 averageScore,
                 1.0f,
                 population[bestIndex].GetNodeCount(),
                 population[bestIndex].GetConnections().size(),
                 {}});
    if (!Evolve(scores, errorMessage))
        return false;
    ++generation;
    errorMessage.clear();
    return true;
}

void EvolutionExperiment::StartNewRun() {
    if (!initialized)
        return;
    ++run;
    generation = 0;
    bestScore = 0.0f;
    averageScore = 0.0f;
    bestGenome.reset();
    std::string ignored;
    initialized = CreateInitialPopulation(ignored);
}

void EvolutionExperiment::Clear() {
    population.clear();
    history.Clear();
    bestGenome.reset();
    run = 0;
    generation = 0;
    bestScore = 0.0f;
    averageScore = 0.0f;
    initialized = false;
}

bool EvolutionExperiment::SaveCheckpoint(const std::filesystem::path &directory, std::string &errorMessage) const {
    if (!initialized) {
        errorMessage = "Evolution experiment is not initialized.";
        return false;
    }
    std::error_code filesystemError;
    std::filesystem::create_directories(directory / "Population", filesystemError);
    if (filesystemError) {
        errorMessage = "Unable to create checkpoint: " + filesystemError.message();
        return false;
    }
    CheckpointMetadata metadata{
        "pipeframe.neat",
        1,
        run,
        generation,
        configuration.seed,
        population.size(),
        {{"inputs", std::to_string(configuration.inputCount)},
         {"outputs", std::to_string(configuration.outputCount)},
         {"elite_ratio", std::to_string(configuration.eliteRatio)},
         {"best_score", std::to_string(bestScore)},
         {"new_node_probability", std::to_string(configuration.mutation.newNodeProbability)},
         {"new_connection_probability", std::to_string(configuration.mutation.newConnectionProbability)},
         {"new_value_probability", std::to_string(configuration.mutation.newValueProbability)},
         {"weight_range", std::to_string(configuration.mutation.weightRange)},
         {"small_weight_range", std::to_string(configuration.mutation.smallWeightRange)},
         {"mutation_count", std::to_string(configuration.mutation.mutationCount)},
         {"maximum_hidden_nodes", std::to_string(configuration.mutation.maximumHiddenNodes)}}};
    if (!metadata.Save(directory / "checkpoint.pftrain", errorMessage))
        return false;
    for (std::size_t index = 0; index < population.size(); ++index) {
        if (!population[index].Save(directory / "Population" / ("agent_" + std::to_string(index) + ".pfneat"),
                                    errorMessage))
            return false;
    }
    if (!history.SaveCsv(directory / "history.csv", errorMessage))
        return false;
    return !bestGenome || bestGenome->Save(directory / "best.pfneat", errorMessage);
}

bool EvolutionExperiment::LoadCheckpoint(const std::filesystem::path &directory, std::string &errorMessage) {
    CheckpointMetadata metadata;
    if (!metadata.Load(directory / "checkpoint.pftrain", errorMessage) || metadata.trainerId != "pipeframe.neat" ||
        metadata.population == 0 || metadata.population > 1'000'000) {
        if (errorMessage.empty())
            errorMessage = "Checkpoint is not a PipeFrame NEAT experiment.";
        return false;
    }
    EvolutionExperimentConfig loadedConfig = configuration;
    try {
        loadedConfig.inputCount = std::stoull(metadata.values.at("inputs"));
        loadedConfig.outputCount = std::stoull(metadata.values.at("outputs"));
        loadedConfig.eliteRatio = std::stof(metadata.values.at("elite_ratio"));
        loadedConfig.mutation.newNodeProbability = std::stof(metadata.values.at("new_node_probability"));
        loadedConfig.mutation.newConnectionProbability = std::stof(metadata.values.at("new_connection_probability"));
        loadedConfig.mutation.newValueProbability = std::stof(metadata.values.at("new_value_probability"));
        loadedConfig.mutation.weightRange = std::stof(metadata.values.at("weight_range"));
        loadedConfig.mutation.smallWeightRange = std::stof(metadata.values.at("small_weight_range"));
        loadedConfig.mutation.mutationCount =
            static_cast<std::uint32_t>(std::stoul(metadata.values.at("mutation_count")));
        loadedConfig.mutation.maximumHiddenNodes =
            static_cast<std::uint32_t>(std::stoul(metadata.values.at("maximum_hidden_nodes")));
        loadedConfig.population = metadata.population;
        loadedConfig.seed = metadata.seed;
    } catch (...) {
        errorMessage = "Checkpoint evolution configuration is invalid.";
        return false;
    }
    std::vector<Genome> loadedPopulation(metadata.population);
    for (std::size_t index = 0; index < loadedPopulation.size(); ++index) {
        if (!loadedPopulation[index].Load(directory / "Population" / ("agent_" + std::to_string(index) + ".pfneat"),
                                          errorMessage))
            return false;
    }
    ExperimentHistory loadedHistory;
    if (!loadedHistory.LoadCsv(directory / "history.csv", errorMessage))
        return false;
    std::optional<Genome> loadedBest;
    if (std::filesystem::exists(directory / "best.pfneat")) {
        Genome value;
        if (!value.Load(directory / "best.pfneat", errorMessage))
            return false;
        loadedBest = std::move(value);
    }
    configuration = loadedConfig;
    population = std::move(loadedPopulation);
    history = std::move(loadedHistory);
    bestGenome = std::move(loadedBest);
    run = metadata.run;
    generation = metadata.iteration;
    try {
        bestScore = std::stof(metadata.values.at("best_score"));
    } catch (...) {
        bestScore = 0.0f;
    }
    initialized = true;
    errorMessage.clear();
    return true;
}

std::uint32_t EvolutionExperiment::Run() const { return run; }
std::uint32_t EvolutionExperiment::Generation() const { return generation; }
std::span<const Genome> EvolutionExperiment::Population() const { return population; }
const Genome *EvolutionExperiment::BestGenome() const { return bestGenome ? &*bestGenome : nullptr; }
float EvolutionExperiment::BestScore() const { return bestScore; }
const ExperimentHistory &EvolutionExperiment::History() const { return history; }

std::optional<InferenceSnapshot> EvolutionExperiment::BestInferenceSnapshot(std::string *errorMessage) const {
    if (!bestGenome) {
        if (errorMessage)
            *errorMessage = "No evaluated genome is available.";
        return std::nullopt;
    }
    auto network = NetworkGenerator::Generate(*bestGenome, errorMessage);
    if (!network)
        return std::nullopt;
    std::vector<float> zeroInputs(bestGenome->GetInputCount(), 0.0f);
    network->Execute(zeroInputs);
    return network->GetInferenceSnapshot();
}

ExperimentStatistics EvolutionExperiment::Statistics() const {
    return {run, generation, 0.0f, 0.0f, 0.0f, bestScore, averageScore, population.size(), false, false};
}

bool EvolutionExperiment::CreateInitialPopulation(std::string &errorMessage) {
    population.clear();
    population.reserve(configuration.population);
    GenomeMutator mutator(configuration.mutation, configuration.seed + static_cast<std::uint64_t>(run) * 100'003u);
    for (std::size_t index = 0; index < configuration.population; ++index) {
        Genome genome(configuration.inputCount, configuration.outputCount);
        if (index > 0 && !mutator.AddConnection(genome)) {
            errorMessage = "Unable to seed evolution population.";
            return false;
        }
        if (index > 0)
            mutator.Mutate(genome);
        population.push_back(std::move(genome));
    }
    errorMessage.clear();
    return true;
}

bool EvolutionExperiment::Evolve(const std::span<const float> scores, std::string &errorMessage) {
    GenomeSelector selector(configuration.seed + static_cast<std::uint64_t>(run) * 100'003u + generation);
    selector.SetScores(scores);
    GenomeMutator mutator(configuration.mutation,
                          configuration.seed + static_cast<std::uint64_t>(run) * 100'003u + generation + 1u);
    const auto ranking = selector.RankDescending();
    const std::size_t eliteCount = std::clamp<std::size_t>(
        static_cast<std::size_t>(configuration.eliteRatio * population.size()), 1, population.size());
    std::vector<Genome> next;
    next.reserve(population.size());
    std::set<std::string> signatures;
    for (std::size_t index = 0; index < eliteCount; ++index) {
        Genome genome = population[ranking[index]];
        if (!signatures.insert(genome.GetSignature()).second)
            mutator.Mutate(genome);
        next.push_back(std::move(genome));
    }
    while (next.size() < population.size()) {
        Genome genome = population[selector.Pick()];
        mutator.Mutate(genome);
        next.push_back(std::move(genome));
    }
    population = std::move(next);
    errorMessage.clear();
    return true;
}

} // namespace pipeframe::learning

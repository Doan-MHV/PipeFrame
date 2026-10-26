#include "SailBoatPopulationTrainer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>

#include <PipeFrame/Learning/GenomeMutator.h>
#include <PipeFrame/Learning/GenomeSelector.h>

namespace sailboat_simulation {
namespace {

constexpr float AsyncFixedDeltaTime = 1.0f / 60.0f;

pipeframe::learning::MutationSettings MutationSettingsFrom(const SailBoatConfiguration &configuration) {
    return {
        configuration.newNodeProbability,
        configuration.newConnectionProbability,
        configuration.newValueProbability,
        configuration.weightRange,
        configuration.smallWeightRange,
        configuration.mutationCount,
        configuration.maximumHiddenNodes,
    };
}

} // namespace

SailBoatPopulationTrainer::~SailBoatPopulationTrainer() { WaitForAsync(); }

bool SailBoatPopulationTrainer::Initialize(const SailBoatConfiguration &newConfiguration,
                                           const RaceCourse &newCourse,
                                           const BoatEnvironment &newEnvironment,
                                           std::string &errorMessage) {
    Clear();
    errorMessage.clear();
    if (!newConfiguration.Validate(errorMessage)) {
        return false;
    }
    if (!newCourse.Validate(errorMessage)) {
        return false;
    }

    configuration = newConfiguration;
    course = newCourse;
    environment = newEnvironment;
    generation = 0;
    exploration = 0;
    generationClock.SetLimit(configuration.maximumIterationTime);
    generationClock.Reset();
    bestScore = 0.0f;
    bestAgentIndex = 0;
    bestGenome.reset();
    history.Clear();
    if (!CreateInitialPopulation(errorMessage)) {
        return false;
    }
    initialized = true;
    asyncEvaluation = configuration.asyncTraining;
    return true;
}

void SailBoatPopulationTrainer::Clear() {
    WaitForAsync();
    configuration = {};
    course.Clear();
    environment = {};
    agents.clear();
    history.Clear();
    bestGenome.reset();
    runDirectory.clear();
    generation = 0;
    exploration = 0;
    generationClock = pipeframe::learning::ExperimentClock{};
    bestScore = 0.0f;
    bestAgentIndex = 0;
    initialized = false;
    asyncEvaluation = false;
}

void SailBoatPopulationTrainer::Update(const float deltaTime) {
    if (!initialized || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
        return;
    }
    if (asyncEvaluation) {
        UpdateAsynchronous(deltaTime);
    } else {
        UpdateSynchronous(deltaTime);
    }
}

void SailBoatPopulationTrainer::ResetGeneration() {
    WaitForAsync();
    generationClock.Reset();
    for (SailBoatAgent &agent : agents) {
        agent.Reset(course);
    }
    RefreshBestAgent();
}

void SailBoatPopulationTrainer::StartNewExploration() {
    WaitForAsync();
    ++exploration;
    generation = 0;
    generationClock.Reset();
    bestScore = 0.0f;
    bestAgentIndex = 0;
    bestGenome.reset();
    std::string errorMessage;
    if (!CreateInitialPopulation(errorMessage)) {
        initialized = false;
    }
}

void SailBoatPopulationTrainer::SetAsyncEvaluation(const bool enabled) {
    if (asyncEvaluation == enabled) {
        return;
    }
    WaitForAsync();
    asyncEvaluation = enabled;
    configuration.asyncTraining = enabled;
}

void SailBoatPopulationTrainer::SetRunDirectory(std::filesystem::path directory) {
    runDirectory = std::move(directory);
}

bool SailBoatPopulationTrainer::SaveRunArtifacts(std::string &errorMessage) const {
    errorMessage.clear();
    if (runDirectory.empty()) {
        errorMessage = "No training run directory is configured.";
        return false;
    }
    std::error_code filesystemError;
    std::filesystem::create_directories(runDirectory, filesystemError);
    if (filesystemError) {
        errorMessage = "Unable to create training run directory: " + filesystemError.message();
        return false;
    }
    if (!history.SaveCsv(runDirectory / "history.csv", errorMessage)) {
        return false;
    }
    if (bestGenome.has_value() && !bestGenome->Save(runDirectory / "best.pfneat", errorMessage)) {
        return false;
    }

    std::ofstream metadata(runDirectory / "run.pftrain", std::ios::trunc);
    if (!metadata) {
        errorMessage = "Unable to create training run metadata.";
        return false;
    }
    metadata << "PIPEFRAME_SAILBOAT_TRAINING 1\n"
             << "exploration " << exploration << '\n'
             << "generation " << generation << '\n'
             << "seed " << configuration.seedOffset << '\n'
             << "population " << configuration.populationSize << '\n'
             << "best_score " << bestScore << '\n';
    return metadata.good();
}

bool SailBoatPopulationTrainer::SaveCheckpoint(const std::filesystem::path &directory,
                                               std::string &errorMessage) const {
    errorMessage.clear();
    if (!initialized || IsAsyncBusy()) {
        errorMessage = "Training must be initialized and between asynchronous evaluations before checkpointing.";
        return false;
    }

    std::error_code filesystemError;
    const std::filesystem::path populationDirectory = directory / "Population";
    std::filesystem::create_directories(populationDirectory, filesystemError);
    if (filesystemError) {
        errorMessage = "Unable to create checkpoint directory: " + filesystemError.message();
        return false;
    }

    pipeframe::learning::CheckpointMetadata metadata{
        .trainerId = "pipeframe.sailboat.neat",
        .version = 1,
        .run = exploration,
        .iteration = generation,
        .seed = configuration.seedOffset,
        .population = agents.size(),
        .values = {
            {"sailboat_schema", "2"},
            {"best_score", std::to_string(bestScore)},
            {"maximum_iteration_time", std::to_string(configuration.maximumIterationTime)},
            {"elite_ratio", std::to_string(configuration.eliteRatio)},
            {"new_node_probability", std::to_string(configuration.newNodeProbability)},
            {"new_connection_probability", std::to_string(configuration.newConnectionProbability)},
            {"new_value_probability", std::to_string(configuration.newValueProbability)},
            {"weight_range", std::to_string(configuration.weightRange)},
            {"small_weight_range", std::to_string(configuration.smallWeightRange)},
            {"mutation_count", std::to_string(configuration.mutationCount)},
            {"maximum_hidden_nodes", std::to_string(configuration.maximumHiddenNodes)},
            {"best_save_period", std::to_string(configuration.bestSavePeriod)},
            {"waypoint_radius", std::to_string(configuration.waypointRadius)},
            {"simulation_speed_up", std::to_string(configuration.simulationSpeedUp)},
            {"angular_speed_degrees", std::to_string(configuration.angularSpeedDegrees)},
            {"async_training", configuration.asyncTraining ? "1" : "0"},
        },
    };
    if (!metadata.Save(directory / "checkpoint.pftrain", errorMessage)) {
        return false;
    }

    for (std::size_t index = 0; index < agents.size(); ++index) {
        if (!agents[index].GetGenome().Save(
                populationDirectory / ("agent_" + std::to_string(index) + ".pfneat"), errorMessage)) {
            return false;
        }
    }
    if (!history.SaveCsv(directory / "history.csv", errorMessage)) {
        return false;
    }
    if (bestGenome.has_value() && !bestGenome->Save(directory / "best.pfneat", errorMessage)) {
        return false;
    }
    return true;
}

bool SailBoatPopulationTrainer::LoadCheckpoint(const std::filesystem::path &directory,
                                               std::string &errorMessage) {
    WaitForAsync();
    errorMessage.clear();
    pipeframe::learning::CheckpointMetadata metadata;
    if (!metadata.Load(directory / "checkpoint.pftrain", errorMessage) ||
        metadata.trainerId != "pipeframe.sailboat.neat" || metadata.version > 2 || metadata.population == 0 ||
        metadata.seed > std::numeric_limits<std::uint32_t>::max() ||
        metadata.population > 1'000'000) {
        errorMessage = "Invalid or unsupported SailBoat training checkpoint.";
        return false;
    }
    const std::size_t population = metadata.population;

    SailBoatConfiguration loadedConfiguration = configuration;
    float loadedBestScore = 0.0f;
    try {
        const auto readFloat = [&metadata](const char *key, float &value) {
            if (const auto found = metadata.values.find(key); found != metadata.values.end()) {
                std::size_t consumed = 0;
                const float parsed = std::stof(found->second, &consumed);
                if (consumed != found->second.size() || !std::isfinite(parsed)) {
                    throw std::invalid_argument{"invalid checkpoint float"};
                }
                value = parsed;
            }
        };
        const auto readUnsigned = [&metadata](const char *key, std::uint32_t &value) {
            if (const auto found = metadata.values.find(key); found != metadata.values.end()) {
                std::size_t consumed = 0;
                const auto parsed = std::stoull(found->second, &consumed);
                if (consumed != found->second.size() || parsed > std::numeric_limits<std::uint32_t>::max()) {
                    throw std::invalid_argument{"invalid checkpoint integer"};
                }
                value = static_cast<std::uint32_t>(parsed);
            }
        };
        readFloat("best_score", loadedBestScore);
        readFloat("maximum_iteration_time", loadedConfiguration.maximumIterationTime);
        readFloat("elite_ratio", loadedConfiguration.eliteRatio);
        readFloat("new_node_probability", loadedConfiguration.newNodeProbability);
        readFloat("new_connection_probability", loadedConfiguration.newConnectionProbability);
        readFloat("new_value_probability", loadedConfiguration.newValueProbability);
        readFloat("weight_range", loadedConfiguration.weightRange);
        readFloat("small_weight_range", loadedConfiguration.smallWeightRange);
        readUnsigned("mutation_count", loadedConfiguration.mutationCount);
        readUnsigned("maximum_hidden_nodes", loadedConfiguration.maximumHiddenNodes);
        readUnsigned("best_save_period", loadedConfiguration.bestSavePeriod);
        readFloat("waypoint_radius", loadedConfiguration.waypointRadius);
        readFloat("simulation_speed_up", loadedConfiguration.simulationSpeedUp);
        readFloat("angular_speed_degrees", loadedConfiguration.angularSpeedDegrees);
        if (const auto found = metadata.values.find("async_training"); found != metadata.values.end()) {
            if (found->second != "0" && found->second != "1") {
                throw std::invalid_argument{"invalid checkpoint boolean"};
            }
            loadedConfiguration.asyncTraining = found->second == "1";
        }
        loadedConfiguration.populationSize = static_cast<std::uint32_t>(population);
        loadedConfiguration.seedOffset = static_cast<std::uint32_t>(metadata.seed);
    } catch (const std::exception &) {
        errorMessage = "Invalid SailBoat training configuration in checkpoint.";
        return false;
    }
    if (!loadedConfiguration.Validate(errorMessage)) {
        errorMessage = "Invalid SailBoat training configuration in checkpoint: " + errorMessage;
        return false;
    }

    std::vector<SailBoatAgent> loadedAgents;
    loadedAgents.reserve(population);
    for (std::size_t index = 0; index < population; ++index) {
        SailBoatAgent agent(index);
        if (!agent.GetGenome().Load(
                directory / "Population" / ("agent_" + std::to_string(index) + ".pfneat"), errorMessage) ||
            !agent.Compile(&errorMessage)) {
            return false;
        }
        agent.Reset(course);
        loadedAgents.push_back(std::move(agent));
    }

    pipeframe::learning::ExperimentHistory loadedHistory;
    if (!loadedHistory.LoadCsv(directory / "history.csv", errorMessage)) {
        return false;
    }
    std::optional<pipeframe::learning::Genome> loadedBest;
    if (std::filesystem::exists(directory / "best.pfneat")) {
        pipeframe::learning::Genome genome;
        if (!genome.Load(directory / "best.pfneat", errorMessage)) {
            return false;
        }
        loadedBest = std::move(genome);
    }

    agents = std::move(loadedAgents);
    history = std::move(loadedHistory);
    bestGenome = std::move(loadedBest);
    configuration = loadedConfiguration;
    exploration = metadata.run;
    generation = metadata.iteration;
    bestScore = loadedBestScore;
    asyncEvaluation = configuration.asyncTraining;
    generationClock.SetLimit(configuration.maximumIterationTime);
    generationClock.Reset();
    initialized = true;
    RefreshBestAgent();
    return true;
}

bool SailBoatPopulationTrainer::IsInitialized() const { return initialized; }
bool SailBoatPopulationTrainer::IsAsyncEvaluation() const { return asyncEvaluation; }
bool SailBoatPopulationTrainer::IsAsyncBusy() const { return asyncResult.Busy(); }
std::uint32_t SailBoatPopulationTrainer::GetGeneration() const { return generation; }
std::uint32_t SailBoatPopulationTrainer::GetExploration() const { return exploration; }
float SailBoatPopulationTrainer::GetGenerationTime() const { return generationClock.Elapsed(); }
std::size_t SailBoatPopulationTrainer::GetBestAgentIndex() const { return bestAgentIndex; }
float SailBoatPopulationTrainer::GetBestScore() const { return bestScore; }
std::span<const SailBoatAgent> SailBoatPopulationTrainer::GetAgents() const { return agents; }
const SailBoatAgent *SailBoatPopulationTrainer::GetBestAgent() const {
    return agents.empty() ? nullptr : &agents[std::min(bestAgentIndex, agents.size() - 1)];
}
const pipeframe::learning::Genome *SailBoatPopulationTrainer::GetBestGenome() const {
    return bestGenome.has_value() ? &*bestGenome : nullptr;
}
const pipeframe::learning::ExperimentHistory &SailBoatPopulationTrainer::GetHistory() const { return history; }

std::optional<SailBoatGeneralizationResult>
SailBoatPopulationTrainer::EvaluateGeneralization(
    const pipeframe::learning::Genome &genome,
    const std::span<const SailBoatGeneralizationTrial> trials,
    const float angularSpeedDegrees,
    const float maximumTime,
    const float fixedDeltaTime,
    std::string &errorMessage
) {
    errorMessage.clear();
    if (trials.empty() || !std::isfinite(angularSpeedDegrees) ||
        angularSpeedDegrees < 0.0f || !std::isfinite(maximumTime) ||
        maximumTime <= 0.0f || !std::isfinite(fixedDeltaTime) ||
        fixedDeltaTime <= 0.0f) {
        errorMessage = "Generalization requires trials and finite positive timing values.";
        return std::nullopt;
    }

    SailBoatGeneralizationResult result;
    result.scores.reserve(trials.size());
    float totalScore = 0.0f;
    std::size_t completed = 0;

    for (std::size_t index = 0; index < trials.size(); ++index) {
        if (!trials[index].course.Validate(errorMessage)) {
            errorMessage = "Invalid generalization course: " + errorMessage;
            return std::nullopt;
        }

        SailBoatAgent agent(index);
        agent.GetGenome() = genome;
        if (!agent.Compile(&errorMessage)) {
            return std::nullopt;
        }
        agent.Reset(trials[index].course);
        std::vector<SailBoatAgent> evaluatedAgents;
        evaluatedAgents.push_back(std::move(agent));
        EvaluateAgents(
            evaluatedAgents,
            trials[index].environment,
            trials[index].course,
            angularSpeedDegrees,
            maximumTime,
            fixedDeltaTime);

        const SailBoatAgent &evaluated = evaluatedAgents.front();
        const float score = evaluated.GetScore();
        result.scores.push_back(score);
        totalScore += score;
        result.bestScore = index == 0 ? score : std::max(result.bestScore, score);
        completed += evaluated.GetTask().HasFinished() ? 1 : 0;
    }

    result.averageScore = totalScore / static_cast<float>(trials.size());
    result.completionRate =
        static_cast<float>(completed) / static_cast<float>(trials.size());
    return result;
}

void SailBoatPopulationTrainer::EvaluateAgents(std::vector<SailBoatAgent> &evaluatedAgents,
                                               const BoatEnvironment &evaluatedEnvironment,
                                               const RaceCourse &evaluatedCourse,
                                               const float angularSpeedDegrees,
                                               const float maximumTime,
                                               const float deltaTime) {
    float elapsed = 0.0f;
    while (elapsed < maximumTime) {
        bool anyActive = false;
        for (SailBoatAgent &agent : evaluatedAgents) {
            anyActive |= agent.IsActive();
            agent.Update(evaluatedEnvironment, evaluatedCourse, angularSpeedDegrees,
                         deltaTime, false);
        }
        if (!anyActive) {
            break;
        }
        elapsed += deltaTime;
    }
}

void SailBoatPopulationTrainer::UpdateSynchronous(const float deltaTime) {
    const float trainingDeltaTime = deltaTime * configuration.simulationSpeedUp;
    for (std::size_t index = 0; index < agents.size(); ++index) {
        agents[index].Update(environment, course, configuration.angularSpeedDegrees,
                             trainingDeltaTime, index == bestAgentIndex);
    }
    generationClock.Advance(trainingDeltaTime);
    RefreshBestAgent();

    const bool anyActive = std::any_of(agents.begin(), agents.end(),
                                       [](const SailBoatAgent &agent) { return agent.IsActive(); });
    if (!anyActive || generationClock.Expired()) {
        FinishGeneration();
    }
}

void SailBoatPopulationTrainer::UpdateAsynchronous(
    const float deltaTime
) {
    generationClock.Advance(std::min(
        deltaTime * configuration.simulationSpeedUp,
        std::max(0.0f,
                 configuration.maximumIterationTime - generationClock.Elapsed())));

    if (!asyncResult.Busy()) {
        StartAsyncGeneration();
        return;
    }
    if (!asyncResult.Ready()) {
        return;
    }
    agents = asyncResult.Take();
    generationClock.Reset();
    generationClock.Advance(configuration.maximumIterationTime);
    RefreshBestAgent();
    FinishGeneration();
}

void SailBoatPopulationTrainer::StartAsyncGeneration() {
    std::vector<SailBoatAgent> evaluationAgents = agents;
    const BoatEnvironment evaluationEnvironment = environment;
    const RaceCourse evaluationCourse = course;
    const float angularSpeed = configuration.angularSpeedDegrees;
    const float maximumTime = configuration.maximumIterationTime;
    const float deltaTime = AsyncFixedDeltaTime * configuration.simulationSpeedUp;
    asyncResult.Start([agents = std::move(evaluationAgents), evaluationEnvironment,
                       evaluationCourse, angularSpeed, maximumTime, deltaTime]() mutable {
        EvaluateAgents(agents, evaluationEnvironment, evaluationCourse,
                       angularSpeed, maximumTime, deltaTime);
        return agents;
    });
}

void SailBoatPopulationTrainer::FinishGeneration() {
    if (agents.empty()) {
        return;
    }
    RefreshBestAgent();

    float totalScore = 0.0f;
    std::size_t completed = 0;
    for (const SailBoatAgent &agent : agents) {
        totalScore += agent.GetScore();
        completed += agent.GetTask().HasFinished() ? 1 : 0;
    }
    const SailBoatAgent &best = agents[bestAgentIndex];
    history.Add({
        .run = exploration,
        .iteration = generation,
        .bestScore = best.GetScore(),
        .averageScore = totalScore / static_cast<float>(agents.size()),
        .progress = static_cast<float>(completed) / static_cast<float>(agents.size()),
        .modelNodeCount = best.GetGenome().GetNodeCount(),
        .modelConnectionCount = best.GetGenome().GetConnections().size(),
        .metrics = {{"race_time", best.GetTask().GetRaceTime()},
                    {"distance", best.GetTask().GetRaceDistance()}},
    });

    if (!bestGenome.has_value() || best.GetScore() >= bestScore) {
        bestScore = best.GetScore();
        bestGenome = best.GetGenome();
    }

    std::string ignoredError;
    if (!runDirectory.empty() &&
        generation % configuration.bestSavePeriod == 0) {
        SaveRunArtifacts(ignoredError);
    }
    CreateNextGeneration(ignoredError);
    ++generation;
    generationClock.Reset();
}

bool SailBoatPopulationTrainer::CreateNextGeneration(std::string &errorMessage) {
    errorMessage.clear();
    if (agents.empty()) {
        errorMessage = "Cannot evolve an empty SailBoat population.";
        return false;
    }

    std::vector<float> scores;
    scores.reserve(agents.size());
    for (const SailBoatAgent &agent : agents) {
        scores.push_back(agent.GetScore());
    }

    const std::uint32_t evolutionSeed = configuration.seedOffset + exploration * 100'003u + generation;
    pipeframe::learning::GenomeSelector selector(evolutionSeed);
    selector.SetScores(scores);
    const std::vector<std::size_t> ranking = selector.RankDescending();
    pipeframe::learning::GenomeMutator mutator(MutationSettingsFrom(configuration), evolutionSeed + 1u);

    const std::size_t eliteCount = std::clamp<std::size_t>(
        static_cast<std::size_t>(configuration.eliteRatio * static_cast<float>(agents.size())),
        1, agents.size());
    std::vector<pipeframe::learning::Genome> nextGenomes;
    nextGenomes.reserve(agents.size());
    std::set<std::string> eliteSignatures;
    for (std::size_t index = 0; index < eliteCount; ++index) {
        pipeframe::learning::Genome genome = agents[ranking[index]].GetGenome();
        if (!eliteSignatures.insert(genome.GetSignature()).second) {
            mutator.Mutate(genome);
        }
        nextGenomes.push_back(std::move(genome));
    }
    while (nextGenomes.size() < agents.size()) {
        pipeframe::learning::Genome genome = agents[selector.Pick()].GetGenome();
        mutator.Mutate(genome);
        nextGenomes.push_back(std::move(genome));
    }

    std::vector<SailBoatAgent> nextAgents;
    nextAgents.reserve(nextGenomes.size());
    for (std::size_t index = 0; index < nextGenomes.size(); ++index) {
        SailBoatAgent agent(index);
        agent.GetGenome() = std::move(nextGenomes[index]);
        if (!agent.Compile(&errorMessage)) {
            return false;
        }
        agent.Reset(course);
        nextAgents.push_back(std::move(agent));
    }
    agents = std::move(nextAgents);
    bestAgentIndex = 0;
    return true;
}

bool SailBoatPopulationTrainer::CreateInitialPopulation(std::string &errorMessage) {
    errorMessage.clear();
    agents.clear();
    agents.reserve(configuration.populationSize);

    for (std::size_t index = 0; index < configuration.populationSize; ++index) {
        SailBoatAgent agent(index);

        if (!agent.Compile(&errorMessage)) {
            agents.clear();
            return false;
        }
        agent.Reset(course);
        agents.push_back(std::move(agent));
    }
    return true;
}

void SailBoatPopulationTrainer::RefreshBestAgent() {
    if (agents.empty()) {
        bestAgentIndex = 0;
        return;
    }
    bestAgentIndex = static_cast<std::size_t>(std::distance(
        agents.begin(), std::max_element(agents.begin(), agents.end(),
                                         [](const SailBoatAgent &left, const SailBoatAgent &right) {
                                             return left.GetScore() < right.GetScore();
                                         })));
}

void SailBoatPopulationTrainer::WaitForAsync() {
    asyncResult.Wait();
}

} // namespace sailboat_simulation

#ifndef SAILBOAT_POPULATION_TRAINER_H
#define SAILBOAT_POPULATION_TRAINER_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Configuration/SailBoatConfiguration.h"
#include <PipeFrame/Learning/ExperimentRuntime.h>
#include <PipeFrame/Learning/Genome.h>
#include "SailBoatAgent.h"

namespace sailboat_simulation {

struct SailBoatGeneralizationTrial {
    RaceCourse course;
    BoatEnvironment environment;
};

struct SailBoatGeneralizationResult {
    std::vector<float> scores;
    float averageScore{0.0f};
    float bestScore{0.0f};
    float completionRate{0.0f};
};

class SailBoatPopulationTrainer final {
public:
    SailBoatPopulationTrainer() = default;
    ~SailBoatPopulationTrainer();

    SailBoatPopulationTrainer(const SailBoatPopulationTrainer &) = delete;
    SailBoatPopulationTrainer &operator=(const SailBoatPopulationTrainer &) = delete;

    bool Initialize(const SailBoatConfiguration &configuration, const RaceCourse &course,
                    const BoatEnvironment &environment, std::string &errorMessage);
    void Clear();
    void Update(float deltaTime);
    void ResetGeneration();
    void StartNewExploration();
    void SetAsyncEvaluation(bool enabled);
    void SetRunDirectory(std::filesystem::path directory);

    bool SaveRunArtifacts(std::string &errorMessage) const;
    bool SaveCheckpoint(const std::filesystem::path &directory, std::string &errorMessage) const;
    bool LoadCheckpoint(const std::filesystem::path &directory, std::string &errorMessage);

    [[nodiscard]] static std::optional<SailBoatGeneralizationResult>
    EvaluateGeneralization(
        const pipeframe::learning::Genome &genome,
        std::span<const SailBoatGeneralizationTrial> trials,
        float angularSpeedDegrees,
        float maximumTime,
        float fixedDeltaTime,
        std::string &errorMessage
    );

    [[nodiscard]] bool IsInitialized() const;
    [[nodiscard]] bool IsAsyncEvaluation() const;
    [[nodiscard]] bool IsAsyncBusy() const;
    [[nodiscard]] std::uint32_t GetGeneration() const;
    [[nodiscard]] std::uint32_t GetExploration() const;
    [[nodiscard]] float GetGenerationTime() const;
    [[nodiscard]] std::size_t GetBestAgentIndex() const;
    [[nodiscard]] float GetBestScore() const;
    [[nodiscard]] std::span<const SailBoatAgent> GetAgents() const;
    [[nodiscard]] const SailBoatAgent *GetBestAgent() const;
    [[nodiscard]] const pipeframe::learning::Genome *GetBestGenome() const;
    [[nodiscard]] const pipeframe::learning::ExperimentHistory &GetHistory() const;

private:
    static void EvaluateAgents(std::vector<SailBoatAgent> &agents, const BoatEnvironment &environment,
                               const RaceCourse &course, float angularSpeedDegrees,
                               float maximumTime, float deltaTime);

    void UpdateSynchronous(float deltaTime);
    void UpdateAsynchronous(float deltaTime);
    void StartAsyncGeneration();
    void FinishGeneration();
    bool CreateInitialPopulation(std::string &errorMessage);
    bool CreateNextGeneration(std::string &errorMessage);
    void RefreshBestAgent();
    void WaitForAsync();

    SailBoatConfiguration configuration;
    RaceCourse course;
    BoatEnvironment environment;
    std::vector<SailBoatAgent> agents;
    pipeframe::learning::ExperimentHistory history;
    std::optional<pipeframe::learning::Genome> bestGenome;
    std::filesystem::path runDirectory;
    pipeframe::learning::BackgroundEvaluation<std::vector<SailBoatAgent>> asyncResult;

    std::uint32_t generation{0};
    std::uint32_t exploration{0};
    pipeframe::learning::ExperimentClock generationClock;
    float bestScore{0.0f};
    std::size_t bestAgentIndex{0};
    bool initialized{false};
    bool asyncEvaluation{false};
};

} // namespace sailboat_simulation

#endif

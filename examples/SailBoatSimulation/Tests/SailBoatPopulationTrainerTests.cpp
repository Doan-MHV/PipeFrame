#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include <SFML/Graphics/RenderTexture.hpp>

#include "Configuration/SailBoatConfiguration.h"
#include "Rendering/SailBoatRenderer.h"
#include "Components/BoatEnvironment.h"
#include "Training/SailBoatPopulationTrainer.h"
#include "World/RaceCourse.h"

namespace {

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

sailboat_simulation::RaceCourse CreateShortCourse() {
    using namespace sailboat_simulation;
    RaceCourse course;
    course.SetWorldSize({100.0f, 100.0f});
    course.SetStart({{50.0f, 50.0f}, {60.0f, 50.0f}});
    course.SetFinish({{55.0f, 45.0f}, {55.0f, 55.0f}});
    return course;
}

sailboat_simulation::SailBoatConfiguration CreateConfiguration(const std::uint32_t population) {
    sailboat_simulation::SailBoatConfiguration configuration;
    configuration.populationSize = population;
    configuration.maximumIterationTime = 0.05f;
    configuration.eliteRatio = 0.25f;
    configuration.newNodeProbability = 1.0f;
    configuration.newConnectionProbability = 1.0f;
    configuration.mutationCount = 4;
    configuration.seedOffset = 17;
    configuration.asyncTraining = false;
    configuration.worldSize = {100.0f, 100.0f};
    configuration.wind = {0.0f, 1.0f};
    return configuration;
}

bool TestPopulationGenerationAndHistory() {
    using namespace sailboat_simulation;
    const RaceCourse course = CreateShortCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    SailBoatPopulationTrainer trainer;
    std::string errorMessage;

    bool passed = true;
    passed &= Check(trainer.Initialize(CreateConfiguration(12), course, environment, errorMessage),
                    "A valid population should initialize. " + errorMessage);
    passed &= Check(trainer.GetAgents().size() == 12,
                    "The trainer should create the configured number of boat agents.");
    trainer.Update(0.01f);
    passed &= Check(trainer.GetGeneration() == 1,
                    "Completing the short course should evolve the next generation.");
    passed &= Check(trainer.GetHistory().Samples().size() == 1,
                    "Every completed generation should append persistent history data.");
    passed &= Check(trainer.GetHistory().Samples()[0].progress == 1.0f,
                    "The short test course should report a complete population.");
    passed &= Check(trainer.GetBestGenome() != nullptr,
                    "The trainer should retain the best genome across generations.");

    bool hasMutatedTopology = false;
    for (const SailBoatAgent &agent : trainer.GetAgents()) {
        hasMutatedTopology |= !agent.GetGenome().GetConnections().empty();
    }
    passed &= Check(hasMutatedTopology,
                    "Selection and forced structural mutation should diversify the new generation.");
    return passed;
}

bool TestInitialPopulationIsSeededAndDeterministic() {
    using namespace sailboat_simulation;
    const SailBoatConfiguration configuration = CreateConfiguration(12);
    const RaceCourse course = CreateShortCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    SailBoatPopulationTrainer first;
    SailBoatPopulationTrainer second;
    std::string errorMessage;

    bool passed = true;
    passed &= Check(first.Initialize(configuration, course, environment, errorMessage),
                    "The first seeded population should initialize. " + errorMessage);
    passed &= Check(second.Initialize(configuration, course, environment, errorMessage),
                    "The comparison seeded population should initialize. " + errorMessage);
    if (first.GetAgents().size() != configuration.populationSize ||
        second.GetAgents().size() != configuration.populationSize) {
        return Check(false, "Seeded populations should retain the configured size.");
    }

    passed &= Check(first.GetAgents()[0].GetGenome().GetConnections().empty(),
                    "Pezza generation zero should use a neutral unconnected genome.");
    for (std::size_t index = 1; index < first.GetAgents().size(); ++index) {
        passed &= Check(first.GetAgents()[index].GetGenome().GetConnections().empty(),
                        "Every Pezza generation-zero genome should begin neutral.");
        passed &= Check(first.GetAgents()[index].GetGenome().GetSignature() ==
                            second.GetAgents()[index].GetGenome().GetSignature(),
                        "Gen 1 seeding should be reproducible for a fixed seed offset.");
        passed &= Check(first.GetAgents()[index].GetGenome().GetSignature() ==
                            first.GetAgents()[0].GetGenome().GetSignature(),
                        "Pezza generation zero should begin with identical genomes.");
    }
    return passed;
}

bool TestRunArtifactsAndCheckpointResume() {
    using namespace sailboat_simulation;
    const RaceCourse course = CreateShortCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    const SailBoatConfiguration configuration = CreateConfiguration(8);
    const std::filesystem::path testRoot =
        std::filesystem::temp_directory_path() / "pipeframe_sailboat_population_tests";
    std::error_code cleanupError;
    std::filesystem::remove_all(testRoot, cleanupError);

    SailBoatPopulationTrainer trainer;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(trainer.Initialize(configuration, course, environment, errorMessage),
                    "Checkpoint source population should initialize. " + errorMessage);
    trainer.SetRunDirectory(testRoot / "Run");
    trainer.Update(0.01f);
    passed &= Check(trainer.SaveRunArtifacts(errorMessage),
                    "Training run artifacts should save. " + errorMessage);
    passed &= Check(std::filesystem::exists(testRoot / "Run/history.csv") &&
                        std::filesystem::exists(testRoot / "Run/best.pfneat") &&
                        std::filesystem::exists(testRoot / "Run/run.pftrain"),
                    "A run should contain history, its best model, and metadata.");

    passed &= Check(trainer.SaveCheckpoint(testRoot / "Checkpoint", errorMessage),
                    "A complete population checkpoint should save. " + errorMessage);

    SailBoatConfiguration differentConfiguration = configuration;
    differentConfiguration.eliteRatio = 0.5f;
    differentConfiguration.newNodeProbability = 0.0f;
    differentConfiguration.newConnectionProbability = 0.0f;
    differentConfiguration.newValueProbability = 0.0f;
    differentConfiguration.mutationCount = 1;
    differentConfiguration.maximumHiddenNodes = 2;
    differentConfiguration.seedOffset = 999;
    SailBoatPopulationTrainer resumed;
    passed &= Check(resumed.Initialize(differentConfiguration, course, environment, errorMessage),
                    "Checkpoint destination should initialize. " + errorMessage);
    passed &= Check(resumed.LoadCheckpoint(testRoot / "Checkpoint", errorMessage),
                    "A population checkpoint should resume. " + errorMessage);
    passed &= Check(resumed.GetGeneration() == trainer.GetGeneration() &&
                        resumed.GetAgents().size() == trainer.GetAgents().size() &&
                        resumed.GetHistory().Samples().size() == trainer.GetHistory().Samples().size() &&
                        resumed.GetBestScore() == trainer.GetBestScore(),
                    "Resume should restore generation, population, history, and the best score.");
    passed &= Check(resumed.GetAgents()[0].GetGenome().GetSignature() ==
                        trainer.GetAgents()[0].GetGenome().GetSignature(),
                    "Resume should restore population genomes exactly.");

    trainer.Update(0.01f);
    resumed.Update(0.01f);
    passed &= Check(resumed.GetGeneration() == trainer.GetGeneration(),
                    "A resumed fixed-seed run should advance to the same generation.");
    for (std::size_t index = 0; index < trainer.GetAgents().size(); ++index) {
        passed &= Check(resumed.GetAgents()[index].GetGenome().GetSignature() ==
                            trainer.GetAgents()[index].GetGenome().GetSignature(),
                        "Checkpoint continuation should reproduce every evolved genome.");
    }

    trainer.SetAsyncEvaluation(true);
    passed &= Check(trainer.SaveCheckpoint(testRoot / "AsyncCheckpoint", errorMessage),
                    "A switched async mode should save in checkpoint metadata. " + errorMessage);
    SailBoatPopulationTrainer asyncResumed;
    passed &= Check(asyncResumed.Initialize(configuration, course, environment, errorMessage) &&
                        asyncResumed.LoadCheckpoint(testRoot / "AsyncCheckpoint", errorMessage),
                    "The async-mode checkpoint should load. " + errorMessage);
    passed &= Check(asyncResumed.IsAsyncEvaluation(),
                    "Checkpoint resume should restore a runtime async-mode change.");

    std::filesystem::remove_all(testRoot, cleanupError);
    return passed;
}

bool TestGeneralizationAcrossCoursesAndWind() {
    using namespace sailboat_simulation;
    RaceCourse eastCourse;
    eastCourse.SetWorldSize({100.0f, 100.0f});
    eastCourse.SetStart({{10.0f, 20.0f}, {20.0f, 20.0f}});
    eastCourse.SetFinish({{20.0f, 15.0f}, {20.0f, 25.0f}});
    RaceCourse southCourse;
    southCourse.SetWorldSize({120.0f, 80.0f});
    southCourse.SetStart({{60.0f, 10.0f}, {60.0f, 20.0f}});
    southCourse.SetFinish({{55.0f, 20.0f}, {65.0f, 20.0f}});

    const std::array<SailBoatGeneralizationTrial, 2> trials{{
        {eastCourse, {{100.0f, 100.0f}, {0.0f, 1.0f}}},
        {southCourse, {{120.0f, 80.0f}, {-1.0f, 0.0f}}},
    }};
    const pipeframe::learning::Genome neutralGenome(4, 1);
    std::string errorMessage;
    const auto result = SailBoatPopulationTrainer::EvaluateGeneralization(
        neutralGenome, trials, 10.0f, 5.0f, 1.0f / 60.0f, errorMessage);

    bool passed = true;
    passed &= Check(result.has_value(),
                    "A policy should evaluate across varied courses and wind. " + errorMessage);
    if (!result.has_value()) {
        return false;
    }
    passed &= Check(result->scores.size() == trials.size(),
                    "Generalization should return one deterministic score per trial.");
    passed &= Check(result->completionRate == 1.0f && result->averageScore > 0.0f,
                    "The controlled generalization fixtures should complete every course.");
    return passed;
}

bool TestNewExplorationChangesEvolutionSeed() {
    using namespace sailboat_simulation;
    const SailBoatConfiguration configuration = CreateConfiguration(12);
    const RaceCourse course = CreateShortCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    SailBoatPopulationTrainer firstExploration;
    SailBoatPopulationTrainer nextExploration;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(firstExploration.Initialize(configuration, course, environment, errorMessage),
                    "First exploration should initialize. " + errorMessage);
    passed &= Check(nextExploration.Initialize(configuration, course, environment, errorMessage),
                    "Comparison exploration should initialize. " + errorMessage);
    nextExploration.StartNewExploration();
    firstExploration.Update(0.01f);
    nextExploration.Update(0.01f);

    passed &= Check(nextExploration.GetExploration() == 1 &&
                        nextExploration.GetGeneration() == 1,
                    "A new exploration should reset and then advance its own generation.");
    bool differs = false;
    for (std::size_t index = 0; index < firstExploration.GetAgents().size(); ++index) {
        differs |= firstExploration.GetAgents()[index].GetGenome().GetSignature() !=
                   nextExploration.GetAgents()[index].GetGenome().GetSignature();
    }
    passed &= Check(differs,
                    "Changing exploration should change the deterministic evolution seed.");
    return passed;
}

bool TestPeriodicBestSaving() {
    using namespace sailboat_simulation;
    SailBoatConfiguration configuration = CreateConfiguration(8);
    configuration.bestSavePeriod = 2;
    const std::filesystem::path testRoot =
        std::filesystem::temp_directory_path() / "pipeframe_sailboat_periodic_save";
    std::error_code cleanupError;
    std::filesystem::remove_all(testRoot, cleanupError);

    SailBoatPopulationTrainer trainer;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(trainer.Initialize(
                        configuration,
                        CreateShortCourse(),
                        {{100.0f, 100.0f}, {0.0f, 1.0f}},
                        errorMessage),
                    "Periodic-save trainer should initialize. " + errorMessage);
    trainer.SetRunDirectory(testRoot);
    trainer.Update(0.01f);
    passed &= Check(std::filesystem::exists(testRoot / "best.pfneat"),
                    "Generation zero should save at the configured period boundary.");
    std::filesystem::remove(testRoot / "best.pfneat", cleanupError);
    trainer.Update(0.01f);
    passed &= Check(!std::filesystem::exists(testRoot / "best.pfneat"),
                    "An off-period generation should not rewrite the best genome.");
    trainer.Update(0.01f);
    passed &= Check(std::filesystem::exists(testRoot / "best.pfneat"),
                    "The next period boundary should save the best genome again.");
    std::filesystem::remove_all(testRoot, cleanupError);
    return passed;
}

bool TestAsynchronousEvaluation() {
    using namespace sailboat_simulation;
    SailBoatConfiguration configuration = CreateConfiguration(16);
    configuration.asyncTraining = true;
    configuration.simulationSpeedUp = 10.0f;
    SailBoatPopulationTrainer trainer;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(trainer.Initialize(configuration, CreateShortCourse(),
                                       {{100.0f, 100.0f}, {0.0f, 1.0f}}, errorMessage),
                    "An asynchronous population should initialize. " + errorMessage);
    trainer.Update(0.01f);
    passed &= Check(trainer.GetGenerationTime() > 0.0f,
                    "The asynchronous training timer should advance while work is running.");
    for (std::size_t attempt = 0; attempt < 1000 && trainer.GetGeneration() == 0; ++attempt) {
        std::this_thread::yield();
        trainer.Update(0.01f);
    }
    passed &= Check(trainer.GetGeneration() == 1,
                    "Background evaluation should publish a completed generation.");
    passed &= Check(!trainer.GetHistory().Samples().empty(),
                    "Background evaluation should produce normal history samples.");
    return passed;
}

bool TestThousandAgentStressAndBoundedHistory() {
    using namespace sailboat_simulation;
    SailBoatConfiguration configuration = CreateConfiguration(1000);
    configuration.maximumIterationTime = 10.0f;
    SailBoatPopulationTrainer trainer;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(trainer.Initialize(configuration, CreateShortCourse(),
                                       {{100.0f, 100.0f}, {0.0f, 1.0f}}, errorMessage),
                    "The Pezza-sized population should initialize. " + errorMessage);

    const auto start = std::chrono::steady_clock::now();
    trainer.Update(1.0f / 60.0f);
    const float elapsedMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - start).count();

    std::size_t trajectoryPointCount = 0;
    for (const SailBoatAgent &agent : trainer.GetAgents()) {
        trajectoryPointCount += agent.GetBoat().GetTrajectory().size();
    }
    passed &= Check(elapsedMilliseconds < 1000.0f,
                    "A 1,000-agent release update should complete within the stress-test budget.");
    passed &= Check(trajectoryPointCount <= 1001,
                    "Only the leading agent should record per-frame trajectory history.");
    return passed;
}

bool TestPezzaPopulationRendering() {
    using namespace sailboat_simulation;
    SailBoatPopulationTrainer trainer;
    SailBoatRenderer renderer;
    RaceCourse course;
    course.SetWorldSize({100.0f, 100.0f});
    course.SetStart({{10.0f, 50.0f}, {20.0f, 50.0f}});
    course.SetFinish({{90.0f, 40.0f}, {90.0f, 60.0f}});
    SailBoatConfiguration rendererConfiguration = CreateConfiguration(4);
    rendererConfiguration.maximumIterationTime = 10.0f;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(trainer.Initialize(rendererConfiguration, course,
                                       {{100.0f, 100.0f}, {0.0f, 1.0f}}, errorMessage),
                    "Renderer test population should initialize. " + errorMessage);
    passed &= Check(renderer.LoadAssets(
                        std::filesystem::path{PIPEFRAME_SAILBOAT_PROJECT_DIR} / "Assets",
                        errorMessage),
                    "Pezza SailBoat textures should load. " + errorMessage);

    sf::RenderTexture target({256, 256});
    target.clear();
    trainer.Update(0.1f);
    renderer.Advance(0.5f);
    renderer.DrawWater(target, {100.0f, 100.0f}, {1.0f, 0.0f}, true,
                       trainer.GetAgents(), trainer.GetBestAgentIndex(), true, true);
    renderer.DrawCourse(target, course, trainer.GetBestAgent()->GetTask());
    renderer.DrawPopulation(target, trainer.GetAgents(), trainer.GetBestAgentIndex());
    renderer.DrawTrajectory(target, trainer.GetBestAgent()->GetBoat());
    renderer.DrawNextTarget(target, trainer.GetBestAgent()->GetBoat(),
                            trainer.GetBestAgent()->GetTask(), course);
    target.display();

    const SailBoatRenderStatistics &statistics = renderer.GetStatistics();
    passed &= Check(renderer.AreAssetsLoaded() && renderer.GetWaterTime() == 0.5f,
                    "Renderer should retain loaded assets and advance water animation time.");
    passed &= Check(!pipeframe::GraphicsResourceService::ShadersAvailable()||statistics.waterShaderActive,
                    "Available shader hardware should use the animated ripple water path.");
    passed &= Check(!statistics.waterShaderActive || statistics.wakeSources == 1,
                    "Pezza-style ghost rendering should let only the best boat disturb water.");
    passed &= Check(statistics.candidateBoats == 4 && statistics.visibleBoats == 4 &&
                        statistics.culledBoats == 0 && statistics.ghostBoats == 3,
                    "Renderer should draw the complete population and classify non-best boats as ghosts.");
    passed &= Check(statistics.boatVertices == 24,
                    "Four textured boats should be emitted as four batched six-vertex quads.");
    passed &= Check(statistics.populationDrawCalls == 1,
                    "The visible population should be submitted in one draw call.");
    passed &= Check(statistics.trajectoryVertices >= 2,
                    "The best trajectory should create a thick two-sided route strip.");
    renderer.DrawWater(target, {100.0f, 100.0f}, {1.0f, 0.0f}, true,
                       trainer.GetAgents(), trainer.GetBestAgentIndex(), true, false);
    passed &= Check(!statistics.waterShaderActive || statistics.wakeSources == 0,
                    "Paused rendering should neither inject wakes nor advance water state.");
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestInitialPopulationIsSeededAndDeterministic();
    passed &= TestPopulationGenerationAndHistory();
    passed &= TestRunArtifactsAndCheckpointResume();
    passed &= TestAsynchronousEvaluation();
    passed &= TestGeneralizationAcrossCoursesAndWind();
    passed &= TestNewExplorationChangesEvolutionSeed();
    passed &= TestPeriodicBestSaving();
    passed &= TestThousandAgentStressAndBoundedHistory();
    passed &= TestPezzaPopulationRendering();

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat population trainer tests passed.\n";
    return 0;
}

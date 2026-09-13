#include <PipeFrame/Learning/EvolutionExperiment.h>
#include <PipeFrame/Learning/NetworkGenerator.h>
#include <PipeFrame/Learning/TrainingInterfaces.h>

#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// Setup and validation must execute in optimized builds as well.
#define CHECK(expression) do { if (!(expression)) throw std::runtime_error(#expression); } while (false)

float Evaluate(pipeframe::learning::Network &network, std::size_t) {
    const std::vector<float> input{1.0f};
    CHECK(network.Execute(input));
    return -std::abs(network.GetOutputs().front() - 0.75f);
}

class ScalarEnvironment final : public pipeframe::learning::Environment {
  public:
    void Reset(std::uint64_t seed) override { value = static_cast<float>(seed % 3); terminal = false; }
    std::span<const float> Observe() const override { return std::span(&value, 1); }
    void Step(std::span<const float> action, float) override {
        if (!action.empty()) value += action.front();
        terminal = true;
    }
    bool IsTerminal() const override { return terminal; }
    float Score() const override { return -std::abs(value - 2.0f); }

  private:
    float value{};
    bool terminal{};
};

void TestPublicEnvironmentContract() {
    ScalarEnvironment environment;
    environment.Reset(1);
    CHECK(environment.Observe().front() == 1.0f);
    const std::vector<float> action{1.0f};
    environment.Step(action, 1.0f);
    CHECK(environment.IsTerminal() && environment.Score() == 0.0f);

    pipeframe::learning::Genome genome(1, 1);
    CHECK(genome.AddConnection(0, 1, 1.0f));
    auto network = pipeframe::learning::NetworkGenerator::Generate(genome);
    CHECK(network);
    pipeframe::learning::NetworkPolicy policy(std::move(*network));
    std::vector<float> output;
    CHECK(policy.Evaluate(std::vector<float>{0.5f}, output));
    CHECK(output.size() == 1 && !policy.Inspect().nodes.empty());
}

void TestClockAndBackgroundEvaluation() {
    pipeframe::learning::ExperimentClock clock(2.0f);
    clock.Advance(0.5f);
    CHECK(clock.Elapsed() == 0.5f && clock.Progress() == 0.25f && !clock.Expired());
    clock.Advance(2.0f);
    CHECK(clock.Progress() == 1.0f && clock.Expired());

    pipeframe::learning::BackgroundEvaluation<int> work;
    CHECK(work.Start([] { return 42; }));
    work.Wait();
    CHECK(!work.Busy());
}

void TestNonBoatEvolutionCheckpointAndVisualization() {
    using namespace pipeframe::learning;
    EvolutionExperimentConfig configuration;
    configuration.inputCount = 1;
    configuration.outputCount = 1;
    configuration.population = 24;
    configuration.seed = 1234;

    EvolutionExperiment first;
    EvolutionExperiment second;
    std::string error;
    CHECK(first.Initialize(configuration, error));
    CHECK(second.Initialize(configuration, error));
    for (int generation = 0; generation < 5; ++generation) {
        CHECK(first.RunGeneration(Evaluate, error));
        CHECK(second.RunGeneration(Evaluate, error));
    }
    CHECK(first.Generation() == 5 && first.History().Samples().size() == 5);
    CHECK(first.BestGenome() && first.BestGenome()->GetSignature() == second.BestGenome()->GetSignature());
    CHECK(first.Population().front().GetSignature() == second.Population().front().GetSignature());

    const auto snapshot = first.BestInferenceSnapshot(&error);
    CHECK(snapshot && snapshot->nodes.size() >= 2);
    for (const auto &edge : snapshot->edges) {
        CHECK(edge.source < snapshot->nodes.size() && edge.target < snapshot->nodes.size());
    }

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "pipeframe_non_boat_learning_test";
    std::error_code cleanup;
    std::filesystem::remove_all(root, cleanup);
    CHECK(first.SaveCheckpoint(root, error));

    EvolutionExperiment resumed;
    CHECK(resumed.LoadCheckpoint(root, error));
    CHECK(resumed.Generation() == first.Generation());
    CHECK(resumed.History().Samples().size() == first.History().Samples().size());
    CHECK(resumed.Population().front().GetSignature() == first.Population().front().GetSignature());
    CHECK(resumed.RunGeneration(Evaluate, error));
    CHECK(first.RunGeneration(Evaluate, error));
    CHECK(resumed.Population().front().GetSignature() == first.Population().front().GetSignature());
    std::filesystem::remove_all(root, cleanup);
}

void TestDisabledEdgesRemainObservable() {
    using namespace pipeframe::learning;
    Genome genome(1, 1);
    CHECK(genome.AddConnection(0, 1, -0.5f));
    genome.GetConnections().front().enabled = false;
    auto network = NetworkGenerator::Generate(genome);
    CHECK(network && network->Execute(std::vector<float>{1.0f}));
    const auto snapshot = network->GetInferenceSnapshot();
    CHECK(snapshot.edges.size() == 1 && !snapshot.edges.front().enabled);
    CHECK(snapshot.edges.front().weight < 0.0f);
    CHECK(snapshot.edges.front().value == 0.0f);

    Genome activeGenome(1, 1);
    CHECK(activeGenome.AddConnection(0, 1, -0.5f));
    auto activeNetwork = NetworkGenerator::Generate(activeGenome);
    CHECK(activeNetwork && activeNetwork->Execute(std::vector<float>{1.0f}));
    const auto activeSnapshot = activeNetwork->GetInferenceSnapshot();
    CHECK(activeSnapshot.edges.size() == 1);
    CHECK(activeSnapshot.edges.front().value == -0.5f);
}

} // namespace

int main() {
    TestPublicEnvironmentContract();
    TestClockAndBackgroundEvaluation();
    TestNonBoatEvolutionCheckpointAndVisualization();
    TestDisabledEdgesRemainObservable();
}

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <PipeFrame/Learning/Activation.h>
#include <PipeFrame/Learning/DirectedAcyclicGraph.h>
#include <PipeFrame/Learning/Genome.h>
#include <PipeFrame/Learning/GenomeMutator.h>
#include <PipeFrame/Learning/GenomeSelector.h>
#include <PipeFrame/Learning/NetworkGenerator.h>

namespace {

bool NearlyEqual(const float left, const float right, const float tolerance = 0.0001f) {
    return std::abs(left - right) <= tolerance;
}

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool TestPezzaActivations() {
    using namespace pipeframe::learning;
    bool passed = true;
    passed &= Check(NearlyEqual(Activate(Activation::None, -2.0f), -2.0f),
                    "Identity activation should preserve its input.");
    passed &= Check(NearlyEqual(Activate(Activation::Sigmoid, 0.0f), 0.5f),
                    "Pezza's 4.9 sigmoid should equal 0.5 at zero.");
    passed &= Check(NearlyEqual(Activate(Activation::Relu, -2.0f), 0.0f),
                    "ReLU should reject negative inputs.");
    passed &= Check(NearlyEqual(Activate(Activation::Tanh, 0.5f), std::tanh(0.5f)),
                    "Tanh activation should match the standard implementation.");
    return passed;
}

bool TestCycleSafeDag() {
    using namespace pipeframe::learning;
    DirectedAcyclicGraph graph;
    graph.AddNode();
    graph.AddNode();
    graph.AddNode();

    bool passed = true;
    passed &= Check(graph.AddConnection(0, 1) && graph.AddConnection(1, 2),
                    "A valid feed-forward chain should be accepted.");
    passed &= Check(!graph.AddConnection(2, 0), "A connection that creates a cycle must be rejected.");
    passed &= Check(!graph.AddConnection(0, 1), "Duplicate connections must be rejected.");
    const auto order = graph.GetTopologicalOrder();
    passed &= Check(order.has_value() && order->size() == 3,
                    "A valid DAG should provide a complete topological order.");
    return passed;
}

bool TestNetworkGenerationAndSplit() {
    using namespace pipeframe::learning;
    Genome genome(4, 1);
    genome.GetNodes()[4].bias = 0.1f;
    genome.AddConnection(0, 4, 0.5f);
    genome.AddConnection(1, 4, -1.0f);

    std::string errorMessage;
    auto network = NetworkGenerator::Generate(genome, &errorMessage);
    bool passed = true;
    passed &= Check(network.has_value(), "A valid genome should compile. " + errorMessage);
    if (!network.has_value()) {
        return false;
    }
    passed &= Check(network->Execute(std::vector<float>{2.0f, 1.0f, 0.0f, 0.0f}),
                    "The network should accept exactly four SailBoat inputs.");
    passed &= Check(NearlyEqual(network->GetOutputs()[0], std::tanh(0.1f)),
                    "Compiled weighted connections and output bias should execute correctly.");
    passed &= Check(!network->Execute(std::vector<float>{1.0f}),
                    "The network should reject an incompatible input count.");

    Genome splitGenome(1, 1);
    splitGenome.AddConnection(0, 1, 0.75f);
    passed &= Check(splitGenome.SplitConnection(0), "Splitting a connection should add a hidden node.");
    passed &= Check(splitGenome.GetHiddenCount() == 1 && splitGenome.GetConnections().size() == 2,
                    "A split should replace one edge with two edges through one hidden node.");
    auto splitNetwork = NetworkGenerator::Generate(splitGenome, &errorMessage);
    passed &= Check(splitNetwork.has_value() && splitNetwork->Execute(std::vector<float>{1.0f}) &&
                        NearlyEqual(splitNetwork->GetOutputs()[0], std::tanh(0.75f)),
                    "Connection splitting should preserve positive-path behavior.");
    return passed;
}

bool TestDeterministicMutationAndSelection() {
    using namespace pipeframe::learning;

    MutationSettings configuration;
    configuration.mutationCount = 8;
    configuration.newNodeProbability = 1.0f;
    configuration.newConnectionProbability = 1.0f;

    Genome first(4, 1);
    first.AddConnection(0, 4, 0.5f);
    Genome second = first;

    GenomeMutator firstMutator(configuration, 42);
    GenomeMutator secondMutator(configuration, 42);
    firstMutator.Mutate(first);
    secondMutator.Mutate(second);

    bool passed = true;
    passed &= Check(first.GetSignature() == second.GetSignature(),
                    "Equal seeds and genomes must produce deterministic mutation.");
    passed &= Check(first.GetHiddenCount() == 1,
                    "A forced structural mutation should split a connection into a hidden node.");

    GenomeSelector selector(7);
    selector.SetScores(std::vector<float>{-5.0f, 0.0f, 10.0f});
    passed &= Check(selector.RankDescending() == std::vector<std::size_t>{2, 1, 0},
                    "Elite ranking should order finite scores from best to worst.");
    passed &= Check(selector.GetCumulativeWeights().size() == 3 &&
                        NearlyEqual(selector.GetCumulativeWeights().back(), 1.0f),
                    "Roulette selection should normalize negative and positive score sets safely.");

    GenomeSelector uniformSelector(9);
    uniformSelector.SetScores(std::vector<float>{0.0f, 0.0f, 0.0f});
    passed &= Check(NearlyEqual(uniformSelector.GetCumulativeWeights()[0], 1.0f / 3.0f),
                    "All-zero populations should use uniform parent weights.");
    return passed;
}

bool TestConnectionExhaustion() {
    using namespace pipeframe::learning;
    MutationSettings configuration;
    GenomeMutator mutator(configuration,0);
    for(unsigned seed=0;seed<20000;++seed) {
        Genome genome(4,1);
        if(!Check(mutator.AddConnection(genome),"An empty genome must always receive a legal edge")) return false;
    }
    Genome full(4,1);
    for(unsigned i=0;i<4;++i) if(!mutator.AddConnection(full)) return Check(false,"Fill every legal input/output edge");
    return Check(!mutator.AddConnection(full) && full.GetConnections().size()==4,"Saturated graphs must stop without duplicate edges");
}

bool TestVersionedPersistence() {
    using namespace pipeframe::learning;
    Genome original(4, 1);
    original.GetNodes()[4].bias = -0.25f;
    original.AddConnection(0, 4, 1.25f);
    original.AddConnection(3, 4, -0.75f);
    original.SplitConnection(0);

    const std::filesystem::path testRoot =
        std::filesystem::temp_directory_path() / "pipeframe_sailboat_neat_tests";
    std::error_code cleanupError;
    std::filesystem::remove_all(testRoot, cleanupError);
    std::filesystem::create_directories(testRoot, cleanupError);
    const std::filesystem::path genomePath = testRoot / "roundtrip.pfneat";

    std::string errorMessage;
    bool passed = true;
    passed &= Check(original.Save(genomePath, errorMessage),
                    "A genome should save using the versioned format. " + errorMessage);

    Genome loaded;
    errorMessage.clear();
    passed &= Check(loaded.Load(genomePath, errorMessage),
                    "A saved genome should load. " + errorMessage);
    passed &= Check(loaded.GetSignature() == original.GetSignature(),
                    "Genome persistence should preserve nodes, topology, biases, and weights exactly.");

    std::filesystem::remove_all(testRoot, cleanupError);
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestPezzaActivations();
    passed &= TestCycleSafeDag();
    passed &= TestNetworkGenerationAndSplit();
    passed &= TestDeterministicMutationAndSelection();
    passed &= TestVersionedPersistence();
    passed &= TestConnectionExhaustion();

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat NEAT tests passed.\n";
    return 0;
}

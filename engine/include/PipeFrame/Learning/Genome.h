#ifndef PIPEFRAME_LEARNING_GENOME_H
#define PIPEFRAME_LEARNING_GENOME_H

#include <PipeFrame/Learning/Activation.h>
#include <PipeFrame/Learning/DirectedAcyclicGraph.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace pipeframe::learning {

enum class NodeKind : std::uint8_t {
    Input,
    Output,
    Hidden,
};

struct GenomeNode final {
    float bias{0.0f};
    Activation activation{Activation::Relu};
    NodeKind kind{NodeKind::Hidden};
};

struct GenomeConnection final {
    std::size_t from{0};
    std::size_t to{0};
    float weight{0.0f};
    bool enabled{true};
};

class Genome final {
public:
    Genome() = default;
    Genome(std::size_t inputCount, std::size_t outputCount);

    std::size_t AddHiddenNode(Activation activation = Activation::Relu, float bias = 0.0f);
    bool AddConnection(std::size_t from, std::size_t to, float weight);
    bool RemoveConnection(std::size_t connectionIndex);
    bool SplitConnection(std::size_t connectionIndex);

    [[nodiscard]] std::size_t GetInputCount() const;
    [[nodiscard]] std::size_t GetOutputCount() const;
    [[nodiscard]] std::size_t GetHiddenCount() const;
    [[nodiscard]] std::size_t GetNodeCount() const;
    [[nodiscard]] bool IsInput(std::size_t node) const;
    [[nodiscard]] bool IsOutput(std::size_t node) const;
    [[nodiscard]] const std::vector<GenomeNode>& GetNodes() const;
    [[nodiscard]] std::vector<GenomeNode>& GetNodes();
    [[nodiscard]] const std::vector<GenomeConnection>& GetConnections() const;
    [[nodiscard]] std::vector<GenomeConnection>& GetConnections();
    [[nodiscard]] const DirectedAcyclicGraph& GetGraph() const;
    [[nodiscard]] std::string GetSignature() const;

    bool Save(const std::filesystem::path& path, std::string& errorMessage) const;
    bool Load(const std::filesystem::path& path, std::string& errorMessage);

private:
    void AddNode(GenomeNode node);

    std::size_t inputCount{0};
    std::size_t outputCount{0};
    std::vector<GenomeNode> nodes;
    std::vector<GenomeConnection> connections;
    DirectedAcyclicGraph graph;
};

}  // namespace pipeframe::learning

#endif

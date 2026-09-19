#ifndef PIPEFRAME_LEARNING_NETWORK_H
#define PIPEFRAME_LEARNING_NETWORK_H

#include <PipeFrame/Learning/Activation.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace pipeframe::learning {

struct NetworkConnection final {
    std::size_t target{0};
    float weight{0.0f};
    float value{0.0f};
    bool enabled{true};
};

struct NetworkNode final {
    float bias{0.0f};
    float sum{0.0f};
    float value{0.0f};
    Activation activation{Activation::None};
    std::vector<NetworkConnection> outgoing;
};

enum class NetworkNodeRole : std::uint8_t { Input, Output, Hidden };

struct InferenceNode {
    std::size_t index{};
    NetworkNodeRole role{NetworkNodeRole::Hidden};
    Activation activation{Activation::None};
    float bias{};
    float sum{};
    float value{};
};

struct InferenceEdge {
    std::size_t source{};
    std::size_t target{};
    float weight{};
    float value{};
    bool enabled{true};
};

struct InferenceSnapshot {
    std::vector<InferenceNode> nodes;
    std::vector<InferenceEdge> edges;
};

class Network final {
public:
    bool Execute(std::span<const float> inputs);

    [[nodiscard]] std::span<const float> GetOutputs() const;
    [[nodiscard]] const std::vector<NetworkNode>& GetNodes() const;
    [[nodiscard]] std::size_t GetConnectionCount() const;
    [[nodiscard]] InferenceSnapshot GetInferenceSnapshot() const;

private:
    friend class NetworkGenerator;

    std::vector<NetworkNode> nodes;
    std::vector<std::size_t> inputNodes;
    std::vector<std::size_t> outputNodes;
    std::vector<float> outputs;
    std::vector<NetworkNodeRole> roles;
    std::size_t connectionCount{0};
};

}  // namespace pipeframe::learning

#endif

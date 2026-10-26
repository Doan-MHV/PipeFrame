#include <PipeFrame/Learning/Network.h>

#include <algorithm>

namespace pipeframe::learning {

bool Network::Execute(const std::span<const float> inputs) {
    if (inputs.size() != inputNodes.size()) {
        return false;
    }

    for (NetworkNode &node : nodes) {
        node.sum = 0.0f;
        node.value = 0.0f;
    }
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        nodes[inputNodes[index]].sum = inputs[index];
    }

    for (NetworkNode &node : nodes) {
        const float value = Activate(node.activation, node.sum + node.bias);
        node.value = value;
        for (NetworkConnection &connection : node.outgoing) {
            if (connection.enabled) {
                connection.value = value * connection.weight;
                nodes[connection.target].sum += connection.value;
            } else {
                connection.value = 0.0f;
            }
        }
    }

    outputs.resize(outputNodes.size());
    for (std::size_t index = 0; index < outputNodes.size(); ++index) {
        const NetworkNode &output = nodes[outputNodes[index]];
        outputs[index] = Activate(output.activation, output.sum + output.bias);
    }
    return true;
}

std::span<const float> Network::GetOutputs() const { return outputs; }
const std::vector<NetworkNode> &Network::GetNodes() const { return nodes; }
std::size_t Network::GetConnectionCount() const { return connectionCount; }

InferenceSnapshot Network::GetInferenceSnapshot() const {
    InferenceSnapshot snapshot;
    snapshot.nodes.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const NetworkNode &node = nodes[index];
        snapshot.nodes.push_back({index, roles[index], node.activation, node.bias, node.sum, node.value});
        for (const NetworkConnection &edge : node.outgoing) {
            snapshot.edges.push_back({index, edge.target, edge.weight, edge.value, edge.enabled});
        }
    }
    return snapshot;
}

} // namespace pipeframe::learning

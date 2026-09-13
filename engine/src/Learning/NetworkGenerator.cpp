#include <PipeFrame/Learning/NetworkGenerator.h>

#include <algorithm>

namespace pipeframe::learning {

std::optional<Network> NetworkGenerator::Generate(const Genome &genome, std::string *errorMessage) {
    const auto order = genome.GetGraph().GetTopologicalOrder();
    if (!order.has_value()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Cannot generate a network from a cyclic genome.";
        }
        return std::nullopt;
    }

    Network network;
    network.nodes.resize(genome.GetNodeCount());
    network.roles.resize(genome.GetNodeCount(), NetworkNodeRole::Hidden);
    std::vector<std::size_t> genomeToNetwork(genome.GetNodeCount());

    for (std::size_t networkIndex = 0; networkIndex < order->size(); ++networkIndex) {
        const std::size_t genomeIndex = (*order)[networkIndex];
        genomeToNetwork[genomeIndex] = networkIndex;
        const GenomeNode &source = genome.GetNodes()[genomeIndex];
        network.nodes[networkIndex].bias = source.bias;
        network.nodes[networkIndex].activation = source.activation;
        network.roles[networkIndex] = source.kind == NodeKind::Input ? NetworkNodeRole::Input :
                                      source.kind == NodeKind::Output ? NetworkNodeRole::Output :
                                                                      NetworkNodeRole::Hidden;
    }

    for (std::size_t genomeIndex = 0; genomeIndex < genome.GetNodeCount(); ++genomeIndex) {
        if (genome.IsInput(genomeIndex)) {
            network.inputNodes.push_back(genomeToNetwork[genomeIndex]);
        } else if (genome.IsOutput(genomeIndex)) {
            network.outputNodes.push_back(genomeToNetwork[genomeIndex]);
        }
    }

    for (const GenomeConnection &connection : genome.GetConnections()) {
        const std::size_t from = genomeToNetwork[connection.from];
        const std::size_t to = genomeToNetwork[connection.to];
        if (from >= to) {
            if (errorMessage != nullptr) {
                *errorMessage = "Genome connection violates topological execution order.";
            }
            return std::nullopt;
        }
        network.nodes[from].outgoing.push_back({to, connection.weight, 0.0f, connection.enabled});
        ++network.connectionCount;
    }
    network.outputs.resize(network.outputNodes.size());
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return network;
}

} // namespace pipeframe::learning

#include <PipeFrame/Learning/Genome.h>

#include <bit>
#include <fstream>
#include <limits>
#include <sstream>

namespace pipeframe::learning {
namespace {

constexpr std::uint32_t FileMagic = 0x50464E54;
constexpr std::uint32_t FileVersion = 2;
constexpr std::uint64_t MaximumSerializedItems = 1'000'000;

template <typename ValueType> bool Write(std::ofstream &stream, const ValueType value) {
    stream.write(reinterpret_cast<const char *>(&value), sizeof(value));
    return stream.good();
}

template <typename ValueType> bool Read(std::ifstream &stream, ValueType &value) {
    stream.read(reinterpret_cast<char *>(&value), sizeof(value));
    return stream.good();
}

} // namespace

Genome::Genome(const std::size_t newInputCount, const std::size_t newOutputCount)
    : inputCount(newInputCount), outputCount(newOutputCount) {
    for (std::size_t index = 0; index < inputCount; ++index) {
        AddNode({0.0f, Activation::None, NodeKind::Input});
    }
    for (std::size_t index = 0; index < outputCount; ++index) {
        AddNode({0.0f, Activation::Tanh, NodeKind::Output});
    }
}

std::size_t Genome::AddHiddenNode(const Activation activation, const float bias) {
    AddNode({bias, activation, NodeKind::Hidden});
    return nodes.size() - 1;
}

bool Genome::AddConnection(const std::size_t from, const std::size_t to, const float weight) {
    if (from >= nodes.size() || to >= nodes.size() || IsOutput(from) || IsInput(to) || !graph.AddConnection(from, to)) {
        return false;
    }
    connections.push_back({from, to, weight});
    return true;
}

bool Genome::RemoveConnection(const std::size_t connectionIndex) {
    if (connectionIndex >= connections.size()) {
        return false;
    }
    const GenomeConnection connection = connections[connectionIndex];
    if (!graph.RemoveConnection(connection.from, connection.to)) {
        return false;
    }
    connections.erase(connections.begin() + static_cast<std::ptrdiff_t>(connectionIndex));
    return true;
}

bool Genome::SplitConnection(const std::size_t connectionIndex) {
    if (connectionIndex >= connections.size()) {
        return false;
    }
    const GenomeConnection original = connections[connectionIndex];
    if (!RemoveConnection(connectionIndex)) {
        return false;
    }
    const std::size_t hidden = AddHiddenNode();
    return AddConnection(original.from, hidden, original.weight) && AddConnection(hidden, original.to, 1.0f);
}

std::size_t Genome::GetInputCount() const { return inputCount; }
std::size_t Genome::GetOutputCount() const { return outputCount; }
std::size_t Genome::GetHiddenCount() const { return nodes.size() - inputCount - outputCount; }
std::size_t Genome::GetNodeCount() const { return nodes.size(); }
bool Genome::IsInput(const std::size_t node) const {
    return node < nodes.size() && nodes[node].kind == NodeKind::Input;
}
bool Genome::IsOutput(const std::size_t node) const {
    return node < nodes.size() && nodes[node].kind == NodeKind::Output;
}
const std::vector<GenomeNode> &Genome::GetNodes() const { return nodes; }
std::vector<GenomeNode> &Genome::GetNodes() { return nodes; }
const std::vector<GenomeConnection> &Genome::GetConnections() const { return connections; }
std::vector<GenomeConnection> &Genome::GetConnections() { return connections; }
const DirectedAcyclicGraph &Genome::GetGraph() const { return graph; }

std::string Genome::GetSignature() const {
    std::ostringstream signature;
    signature << inputCount << ':' << outputCount;
    for (const GenomeNode &node : nodes) {
        signature << '|' << static_cast<unsigned int>(node.kind) << ',' << static_cast<unsigned int>(node.activation)
                  << ',' << std::bit_cast<std::uint32_t>(node.bias);
    }
    for (const GenomeConnection &connection : connections) {
        signature << '>' << connection.from << ',' << connection.to << ','
                  << std::bit_cast<std::uint32_t>(connection.weight) << ',' << connection.enabled;
    }
    return signature.str();
}

bool Genome::Save(const std::filesystem::path &path, std::string &errorMessage) const {
    errorMessage.clear();
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        errorMessage = "Unable to create NEAT genome file: " + path.string();
        return false;
    }

    const std::uint64_t serializedInputs = inputCount;
    const std::uint64_t serializedOutputs = outputCount;
    const std::uint64_t nodeCount = nodes.size();
    const std::uint64_t connectionCount = connections.size();
    if (!Write(stream, FileMagic) || !Write(stream, FileVersion) || !Write(stream, serializedInputs) ||
        !Write(stream, serializedOutputs) || !Write(stream, nodeCount) || !Write(stream, connectionCount)) {
        errorMessage = "Unable to write NEAT genome header.";
        return false;
    }

    for (const GenomeNode &node : nodes) {
        if (!Write(stream, node.bias) || !Write(stream, static_cast<std::uint8_t>(node.activation)) ||
            !Write(stream, static_cast<std::uint8_t>(node.kind))) {
            errorMessage = "Unable to write NEAT genome nodes.";
            return false;
        }
    }
    for (const GenomeConnection &connection : connections) {
        if (!Write(stream, static_cast<std::uint64_t>(connection.from)) ||
            !Write(stream, static_cast<std::uint64_t>(connection.to)) || !Write(stream, connection.weight) ||
            !Write(stream, static_cast<std::uint8_t>(connection.enabled))) {
            errorMessage = "Unable to write NEAT genome connections.";
            return false;
        }
    }
    return true;
}

bool Genome::Load(const std::filesystem::path &path, std::string &errorMessage) {
    errorMessage.clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        errorMessage = "Unable to open NEAT genome file: " + path.string();
        return false;
    }

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint64_t serializedInputs = 0;
    std::uint64_t serializedOutputs = 0;
    std::uint64_t nodeCount = 0;
    std::uint64_t connectionCount = 0;
    if (!Read(stream, magic) || !Read(stream, version) || !Read(stream, serializedInputs) ||
        !Read(stream, serializedOutputs) || !Read(stream, nodeCount) || !Read(stream, connectionCount) ||
        magic != FileMagic || (version != 1 && version != FileVersion) || nodeCount > MaximumSerializedItems ||
        connectionCount > MaximumSerializedItems || serializedInputs + serializedOutputs > nodeCount) {
        errorMessage = "Invalid or unsupported NEAT genome header.";
        return false;
    }

    Genome loaded;
    loaded.inputCount = static_cast<std::size_t>(serializedInputs);
    loaded.outputCount = static_cast<std::size_t>(serializedOutputs);
    for (std::uint64_t index = 0; index < nodeCount; ++index) {
        float bias = 0.0f;
        std::uint8_t activation = 0;
        std::uint8_t kind = 0;
        if (!Read(stream, bias) || !Read(stream, activation) || !Read(stream, kind) ||
            activation > static_cast<std::uint8_t>(Activation::Tanh) ||
            kind > static_cast<std::uint8_t>(NodeKind::Hidden)) {
            errorMessage = "Invalid NEAT genome node data.";
            return false;
        }
        loaded.AddNode({bias, static_cast<Activation>(activation), static_cast<NodeKind>(kind)});
    }

    for (std::uint64_t index = 0; index < connectionCount; ++index) {
        std::uint64_t from = 0;
        std::uint64_t to = 0;
        float weight = 0.0f;
        std::uint8_t enabled = 1;
        if (!Read(stream, from) || !Read(stream, to) || !Read(stream, weight) ||
            (version >= 2 && !Read(stream, enabled)) || enabled > 1 || from > std::numeric_limits<std::size_t>::max() ||
            to > std::numeric_limits<std::size_t>::max() ||
            !loaded.AddConnection(static_cast<std::size_t>(from), static_cast<std::size_t>(to), weight)) {
            errorMessage = "Invalid NEAT genome connection data.";
            return false;
        }
        loaded.connections.back().enabled = enabled != 0;
    }

    *this = std::move(loaded);
    return true;
}

void Genome::AddNode(GenomeNode node) {
    nodes.push_back(node);
    graph.AddNode();
}

} // namespace pipeframe::learning

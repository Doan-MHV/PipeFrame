#ifndef PIPEFRAME_LEARNING_DIRECTED_ACYCLIC_GRAPH_H
#define PIPEFRAME_LEARNING_DIRECTED_ACYCLIC_GRAPH_H

#include <cstddef>
#include <optional>
#include <vector>

namespace pipeframe::learning {

class DirectedAcyclicGraph final {
public:
    std::size_t AddNode();
    bool AddConnection(std::size_t from, std::size_t to);
    bool RemoveConnection(std::size_t from, std::size_t to);

    [[nodiscard]] bool HasConnection(std::size_t from, std::size_t to) const;
    [[nodiscard]] bool IsReachable(std::size_t from, std::size_t to) const;
    [[nodiscard]] std::size_t GetNodeCount() const;
    [[nodiscard]] std::size_t GetOutgoingCount(std::size_t node) const;
    [[nodiscard]] std::optional<std::vector<std::size_t>> GetTopologicalOrder() const;

    void Clear();

private:
    std::vector<std::vector<std::size_t>> outgoing;
    std::vector<std::size_t> incomingCounts;
};

} // namespace pipeframe::learning

#endif


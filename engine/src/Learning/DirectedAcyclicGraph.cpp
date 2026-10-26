#include <PipeFrame/Learning/DirectedAcyclicGraph.h>

#include <algorithm>

namespace pipeframe::learning {

std::size_t DirectedAcyclicGraph::AddNode() {
    outgoing.emplace_back();
    incomingCounts.push_back(0);
    return outgoing.size() - 1;
}

bool DirectedAcyclicGraph::AddConnection(const std::size_t from, const std::size_t to) {
    if (from >= outgoing.size() || to >= outgoing.size() || from == to ||
        HasConnection(from, to) || IsReachable(to, from)) {
        return false;
    }
    outgoing[from].push_back(to);
    ++incomingCounts[to];
    return true;
}

bool DirectedAcyclicGraph::RemoveConnection(const std::size_t from, const std::size_t to) {
    if (from >= outgoing.size() || to >= outgoing.size()) {
        return false;
    }
    auto &edges = outgoing[from];
    const auto iterator = std::find(edges.begin(), edges.end(), to);
    if (iterator == edges.end()) {
        return false;
    }
    edges.erase(iterator);
    --incomingCounts[to];
    return true;
}

bool DirectedAcyclicGraph::HasConnection(const std::size_t from, const std::size_t to) const {
    if (from >= outgoing.size()) {
        return false;
    }
    const auto &edges = outgoing[from];
    return std::find(edges.begin(), edges.end(), to) != edges.end();
}

bool DirectedAcyclicGraph::IsReachable(const std::size_t from, const std::size_t to) const {
    if (from >= outgoing.size() || to >= outgoing.size()) {
        return false;
    }
    std::vector<bool> visited(outgoing.size(), false);
    std::vector<std::size_t> pending{from};
    while (!pending.empty()) {
        const std::size_t current = pending.back();
        pending.pop_back();
        if (current == to) {
            return true;
        }
        if (visited[current]) {
            continue;
        }
        visited[current] = true;
        pending.insert(pending.end(), outgoing[current].begin(), outgoing[current].end());
    }
    return false;
}

std::size_t DirectedAcyclicGraph::GetNodeCount() const { return outgoing.size(); }

std::size_t DirectedAcyclicGraph::GetOutgoingCount(const std::size_t node) const {
    return node < outgoing.size() ? outgoing[node].size() : 0;
}

std::optional<std::vector<std::size_t>> DirectedAcyclicGraph::GetTopologicalOrder() const {
    std::vector<std::size_t> incoming = incomingCounts;
    std::vector<std::size_t> ready;
    for (std::size_t index = incoming.size(); index-- > 0;) {
        if (incoming[index] == 0) {
            ready.push_back(index);
        }
    }

    std::vector<std::size_t> order;
    order.reserve(outgoing.size());
    while (!ready.empty()) {
        const std::size_t node = ready.back();
        ready.pop_back();
        order.push_back(node);
        for (const std::size_t target : outgoing[node]) {
            if (--incoming[target] == 0) {
                ready.push_back(target);
            }
        }
    }
    if (order.size() != outgoing.size()) {
        return std::nullopt;
    }
    return order;
}

void DirectedAcyclicGraph::Clear() {
    outgoing.clear();
    incomingCounts.clear();
}

} // namespace pipeframe::learning


#pragma once
#include <PipeFrame/Physics/Physics2D.h>
#include <span>
#include <unordered_map>
#include <vector>
#include <stdexcept>
namespace pipeframe {
// Dense solver-body storage shared by general physics and project-specific
// solvers. Structural changes invalidate body pointers and spans.
template<class Body = PhysicsBody2D> class BodyStorage {
public:
    PhysicsBodyId Create(Body body = {}) {
        if (!nextId) throw std::overflow_error("Physics body identity exhausted");
        body.id = nextId++;
        body.previousPosition = body.position;
        bodies.push_back(std::move(body));
        indices[bodies.back().id] = bodies.size() - 1;
        return bodies.back().id;
    }
    Body *Find(PhysicsBodyId id) {
        auto found = indices.find(id);
        return found == indices.end() ? nullptr : &bodies[found->second];
    }
    const Body *Find(PhysicsBodyId id) const { return const_cast<BodyStorage *>(this)->Find(id); }
    bool Remove(PhysicsBodyId id) {
        const auto found = indices.find(id);
        if (found == indices.end()) return false;
        const auto index = found->second;
        if (index + 1 != bodies.size()) {
            bodies[index] = std::move(bodies.back());
            indices[bodies[index].id] = index;
        }
        bodies.pop_back(); indices.erase(id); return true;
    }
    std::span<Body> Bodies() { return bodies; }
    std::span<const Body> Bodies() const { return bodies; }
    void Clear(bool resetIds = false) { bodies.clear(); indices.clear(); if (resetIds) nextId = 1; }
private:
    std::vector<Body> bodies;
    std::unordered_map<PhysicsBodyId, std::size_t> indices;
    PhysicsBodyId nextId{1};
};
}

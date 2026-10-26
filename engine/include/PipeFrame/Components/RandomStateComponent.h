#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <random>
namespace pipeframe {
// Per-entity random stream. Authored seeds are applied at scene initialization/reset.
struct RandomStateComponent {
    // Generator internals are intentionally not exposed or serialized by this schema.
    // The authored seed belongs to the project's settings component.
    static auto Schema() {
        return ComponentSchema<RandomStateComponent>("pipeframe.random-state", "Random State").Required();
    }
    std::mt19937 generator{1};
};
}  // namespace pipeframe

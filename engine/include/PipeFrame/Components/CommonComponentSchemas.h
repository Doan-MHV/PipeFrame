#pragma once
#include <PipeFrame/Components/EnergySchema.h>
#include <PipeFrame/Components/RandomStateComponent.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Project/ComponentRegistry.h>

#include <numbers>

namespace pipeframe {
// Compatibility entry point; the component owns the only schema definition.
inline auto Transform2DSchema() {
    return Transform2DComponent::Schema();
}

inline void RegisterCommonComponents(ComponentRegistry& registry) {
    registry.Register(Transform2DComponent::Schema());
    registry.Register(EnergyComponent::Schema());
    registry.Register(RandomStateComponent::Schema());
    registry.Register(Motion2DComponent::Schema());
}

}  // namespace pipeframe

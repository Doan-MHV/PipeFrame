#pragma once
#include <PipeFrame/Components/EnergyComponent.h>
namespace pipeframe {
// Compatibility wrapper. New code calls EnergyComponent::Schema().
inline const auto& EnergySchema() {
    return EnergyComponent::Schema();
}
}  // namespace pipeframe

#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
#include <algorithm>

namespace pipeframe {
// Domain-neutral consumable resource; projects choose units and depletion rules.
struct EnergyComponent {
    static const ComponentSchema<EnergyComponent> &Schema() {
    static const auto schema = ComponentSchema<EnergyComponent>("pipeframe.energy", "EnergyComponent")
        .ReadOnly({.key="current", .displayName="Current", .kind=PropertyKind::Number, .defaultValue=0.0, .unit="", .minimum=0.0}, &EnergyComponent::current)
        .ReadOnly({.key="capacity", .displayName="Capacity", .kind=PropertyKind::Number, .defaultValue=0.0, .unit="", .minimum=0.0}, &EnergyComponent::capacity);
        return schema;
    }
    float current{};
    float capacity{};
    bool IsDepleted() const { return current <= 0; }
    void Consume(float amount) { if (amount > 0) current = std::max(0.0f, current - amount); }
    void Refill(float maximum) { capacity = maximum; current = maximum; }
    void Deplete() { current = 0; }
};
}

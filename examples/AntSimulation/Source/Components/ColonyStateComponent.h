#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
#include "World/Runtime/Environment/Marker.h"
#include <PipeFrame/Foundation/MathTypes.h>
#include <cstddef>
namespace ant_simulation {
struct ColonyStateComponent {
    // Inspector exposure is declared here; unlisted members stay runtime-only.
    static auto Schema() {
        using namespace pipeframe;
        using K=PropertyKind;
        return ComponentSchema<ColonyStateComponent>("ant.colony-state","Colony State").Required()
        .ReadOnly({.key="radius", .displayName="Current Radius", .kind=K::Number, .defaultValue=0.0},&ColonyStateComponent::radius)
        .ReadOnly({.key="reserve", .displayName="Reserve", .kind=K::Number, .defaultValue=0.0},&ColonyStateComponent::reserve)
        .ReadOnly({.key="food", .displayName="Delivered Food", .kind=K::Number, .defaultValue=0.0},&ColonyStateComponent::foodQuantity)
        .ReadOnly({.key="members", .displayName="Ant Count", .kind=K::Integer, .defaultValue=std::int64_t{0}},
            [](const auto &c)->PropertyValue { return static_cast<std::int64_t>(c.memberCount); });
    }

    ColonyId id{};
    float radius{}, reserve{}, foodQuantity{}, collectionRate{}, soldierRequested{};
    pipeframe::Color color{pipeframe::Color::White};
    std::size_t memberCount{};
};
}

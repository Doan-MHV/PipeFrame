#pragma once
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Project/Component.h>

#include <cstddef>

#include "World/Simulation/Environment/Marker.h"
namespace ant_simulation {
struct ColonyStateComponent : pipeframe::Component<ColonyStateComponent> {
  // Inspector exposure is declared here; unlisted members stay runtime-only.
  static auto BuildSchema() {
    using namespace pipeframe;
    using K = PropertyKind;
    return Define("ant.colony-state", "Colony State")
      .Required()
      .ReadOnly("radius", "Current Radius", &ColonyStateComponent::radius)
      .ReadOnly("reserve", "Reserve", &ColonyStateComponent::reserve)
      .ReadOnly("food", "Delivered Food", &ColonyStateComponent::foodQuantity)
      .ReadOnly({.key = "members",
                  .displayName = "Ant Count",
                  .kind = K::Integer,
                  .defaultValue = std::int64_t{0}},
        [](const auto& c) -> PropertyValue { return static_cast<std::int64_t>(c.memberCount); });
  }

  ColonyId id{};
  float radius{}, reserve{}, foodQuantity{}, collectionRate{}, soldierRequested{};
  pipeframe::Color color{pipeframe::Color::White};
  std::size_t memberCount{};
};
}  // namespace ant_simulation

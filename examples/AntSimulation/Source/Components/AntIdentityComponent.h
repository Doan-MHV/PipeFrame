#pragma once
#include "World/Runtime/Environment/Marker.h"
#include <PipeFrame/ECS/Entity.h>
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Project/ComponentSchema.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace ant_simulation {
using AntId = pipeframe::ecs::Entity;
inline constexpr AntId InvalidAntId{};

enum class AntRole : std::uint8_t {
    Follower = 0,
    Explorer = 1,
    Soldier = 2,
};

[[nodiscard]]
constexpr std::string_view ToString(const AntRole role) {
    switch (role) {
    case AntRole::Follower:
        return "Follower";

    case AntRole::Explorer:
        return "Explorer";

    case AntRole::Soldier:
        return "Soldier";
    }

    return "Unknown";
}

struct AntIdentityComponent {
    // Inspector exposure is declared here; unlisted members stay runtime-only.
    static auto Schema() {
        using namespace pipeframe;
        using K = PropertyKind;
        return ComponentSchema<AntIdentityComponent>("ant.identity", "Ant Identity")
            .Required()
            .ReadOnly({.key = "colony", .displayName = "Colony", .kind = K::Integer, .defaultValue = std::int64_t{0}},
                      [](const auto &c) -> PropertyValue { return static_cast<std::int64_t>(c.colonyId); })
            .ReadOnly({.key = "role", .displayName = "Role", .kind = K::String, .defaultValue = std::string{}},
                      [](const auto &c) -> PropertyValue { return std::string(ToString(c.role)); })
            .ReadOnly({.key = "color", .displayName = "Color", .kind = K::Color, .defaultValue = Color{}},
                      &AntIdentityComponent::color);
    }

    ColonyId colonyId{InvalidColonyId};
    AntRole role{AntRole::Follower};
    pipeframe::Color color{231, 111, 81};
    std::size_t nameIndex{}, nameSuffix{};
    std::uint64_t physicsObjectId{};
};
} // namespace ant_simulation

#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <PipeFrame/Foundation/MathTypes.h>
#include <cstdint>
#include <string_view>
#include <cstddef>

namespace ant_simulation {

enum class ForagingState : std::uint8_t {
    ToFood = 0,
    ToHomeWithFood = 1,
    ToHomeNoFood = 2,
};

[[nodiscard]]
constexpr std::string_view ToString(
    const ForagingState state
) {
    switch (state) {
    case ForagingState::ToFood:
        return "Searching for food";

    case ForagingState::ToHomeWithFood:
        return "Returning with food";

    case ForagingState::ToHomeNoFood:
        return "Returning without food";
    }

    return "Unknown";
}


// Per-ant food seeking and marker trail state, owned by the scene's ECS world.
struct ForagingComponent {
    // Inspector exposure is declared here; unlisted members stay runtime-only.
    static auto Schema() {
        using namespace pipeframe;
        using K=PropertyKind;
        return ComponentSchema<ForagingComponent>("ant.foraging","Foraging").Required()
        .ReadOnly({.key="target", .displayName="Target", .kind=K::Vector2, .defaultValue=Vector2f{}},&ForagingComponent::target)
        .ReadOnly({.key="distanceToTarget", .displayName="Target Distance", .kind=K::Number, .defaultValue=0.0},&ForagingComponent::distanceToTarget);
    }

    ForagingState state{ForagingState::ToFood};
    pipeframe::Vector2f target{};
    float distanceToTarget{};
    pipeframe::Vector2f lastMarkerPosition{};
    float timeSinceLastMarker{};
    float walkTime{};
    bool blocked{};
    std::size_t collectedFood{};
};
} // namespace ant_simulation

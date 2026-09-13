#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <PipeFrame/ECS/Entity.h>
#include <optional>

namespace ant_simulation {
// Ant-specific encounter data. Entity identity and storage belong to PipeFrame ECS.
struct AntEncounterComponent {
    // Inspector exposure is declared here; unlisted members stay runtime-only.
    static auto Schema() {
        using namespace pipeframe;
        using K=PropertyKind;
        return ComponentSchema<AntEncounterComponent>("ant.encounter","Encounter").Required()
        .ReadOnly({.key="enemyTimer", .displayName="Enemy Timer", .kind=K::Number, .defaultValue=0.0},&AntEncounterComponent::enemyTimer);
    }

    float enemyTimer{-1.0f};
    std::optional<pipeframe::ecs::Entity> opponentId;
};
} // namespace ant_simulation

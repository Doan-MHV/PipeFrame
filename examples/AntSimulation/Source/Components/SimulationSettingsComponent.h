#pragma once
#include "Runtime/AntTypeIds.h"
#include <PipeFrame/Project/ComponentSchema.h>

namespace ant_simulation {
struct SimulationSettingsComponent {
    std::int64_t workers{1};
    bool showGrid{true};
    bool showMarkers{true};
    bool showShadows{true};
    bool showTargets{false};
    bool showPhysics{false};
    bool showAnts{true};
    bool dynamicColors{false};
    std::int64_t markerIntensity{10};
    pipeframe::AssetReference mapAsset{};

    static auto Schema() {
        using namespace pipeframe;
        using K = PropertyKind;
        return ComponentSchema<SimulationSettingsComponent>(SimulationSettingsTypeId, "Simulation Settings")
            .Required()
            .Editable({.key = UpdateWorkerCountKey,
                       .displayName = "Update Workers",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{1},
                       .unit = "",
                       .minimum = 1,
                       .maximum = 64,
                       .step = 1},
                      &SimulationSettingsComponent::workers)
            .Editable({.key = ShowGridKey, .displayName = "Show Grid", .kind = K::Boolean, .defaultValue = true},
                      &SimulationSettingsComponent::showGrid)
            .Editable({.key = ShowMarkersKey, .displayName = "Show Markers", .kind = K::Boolean, .defaultValue = true},
                      &SimulationSettingsComponent::showMarkers)
            .Editable({.key = ShowShadowsKey, .displayName = "Show Shadows", .kind = K::Boolean, .defaultValue = true},
                      &SimulationSettingsComponent::showShadows)
            .Editable({.key = ShowTargetsKey, .displayName = "Show Targets", .kind = K::Boolean, .defaultValue = false},
                      &SimulationSettingsComponent::showTargets)
            .Editable(
                {.key = ShowPhysicsDebugKey, .displayName = "Physics Debug", .kind = K::Boolean, .defaultValue = false},
                &SimulationSettingsComponent::showPhysics)
            .Editable({.key = ShowAntsKey, .displayName = "Draw Ants", .kind = K::Boolean, .defaultValue = true},
                      &SimulationSettingsComponent::showAnts)
            .Editable({.key = DynamicAntColorsKey,
                       .displayName = "Dynamic Colors",
                       .kind = K::Boolean,
                       .defaultValue = false},
                      &SimulationSettingsComponent::dynamicColors)
            .Editable({.key = MarkerIntensityKey,
                       .displayName = "Marker Intensity",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{10},
                       .unit = "",
                       .minimum = 0,
                       .maximum = 100,
                       .step = 1},
                      &SimulationSettingsComponent::markerIntensity)
            .Editable({.key = MapAssetKey,
                       .displayName = "Environment Map",
                       .kind = K::AssetReference,
                       .defaultValue = AssetReference{},
                       .editorHint = "asset:Tilemap"},
                      &SimulationSettingsComponent::mapAsset);
    }
};
} // namespace ant_simulation

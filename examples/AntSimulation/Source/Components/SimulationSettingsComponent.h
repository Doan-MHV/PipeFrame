#pragma once
#include <PipeFrame/Project/Component.h>

#include "Integration/AntTypeIds.h"

namespace ant_simulation {
struct SimulationSettingsComponent : pipeframe::Component<SimulationSettingsComponent> {
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

  static auto BuildSchema() {
    using namespace pipeframe;
    return Define(SimulationSettingsTypeId, "Simulation Settings")
      .Required()
      .Editable(UpdateWorkerCountKey, "Update Workers", &SimulationSettingsComponent::workers, {.minimum = 1, .maximum = 64, .step = 1})
      .Editable(ShowGridKey, "Show Grid", &SimulationSettingsComponent::showGrid)
      .Editable(ShowMarkersKey, "Show Markers", &SimulationSettingsComponent::showMarkers)
      .Editable(ShowShadowsKey, "Show Shadows", &SimulationSettingsComponent::showShadows)
      .Editable(ShowTargetsKey, "Show Targets", &SimulationSettingsComponent::showTargets)
      .Editable(ShowPhysicsDebugKey, "Physics Debug", &SimulationSettingsComponent::showPhysics)
      .Editable(ShowAntsKey, "Draw Ants", &SimulationSettingsComponent::showAnts)
      .Editable(DynamicAntColorsKey, "Dynamic Colors", &SimulationSettingsComponent::dynamicColors)
      .Editable(MarkerIntensityKey, "Marker Intensity", &SimulationSettingsComponent::markerIntensity, {.minimum = 0, .maximum = 100, .step = 1})
      .Editable(MapAssetKey, "Environment Map", &SimulationSettingsComponent::mapAsset, {.editorHint = "asset:Tilemap"});
  }
};
}  // namespace ant_simulation

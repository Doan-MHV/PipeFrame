#pragma once

#include <PipeFrame/UI/ViewPanel.h>

namespace ant_simulation {

class AntSimulationRuntime;

// Keep the drawer order explicit: selection can open its own presenter without
// depending on a magic page number in the view builder.
enum class AntPanelKind {
  Tools,
  Settings,
  Colony,
  SelectedAnt,
  Profiler,
  Control,
  Time,
};

class AntPanel : public pipeframe::ui::ViewPanel {
public:
  AntPanel(AntSimulationRuntime& runtime, AntPanelKind kind) : runtime(runtime), kind(kind) {}

  void OnUpdate(float) override;

protected:
  pipeframe::ui::View BuildView() override;

private:
  // The runtime is stable across Reset. It resolves the current AntWorld when
  // building each view, so panels never retain a replaced simulation world.
  AntSimulationRuntime& runtime;
  AntPanelKind kind;
};

template <AntPanelKind Kind>
class AntDashboardPanel final : public AntPanel {
public:
  explicit AntDashboardPanel(AntSimulationRuntime& runtime) : AntPanel(runtime, Kind) {}
};

using AntToolsPanel = AntDashboardPanel<AntPanelKind::Tools>;
using AntSettingsPanel = AntDashboardPanel<AntPanelKind::Settings>;
using AntColonyPanel = AntDashboardPanel<AntPanelKind::Colony>;
using AntSelectionPanel = AntDashboardPanel<AntPanelKind::SelectedAnt>;
using AntProfilerPanel = AntDashboardPanel<AntPanelKind::Profiler>;
using AntControlPanel = AntDashboardPanel<AntPanelKind::Control>;
using AntTimePanel = AntDashboardPanel<AntPanelKind::Time>;

}  // namespace ant_simulation

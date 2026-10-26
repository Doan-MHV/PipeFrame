#include <PipeFrame/Project/ProjectDefinition.h>
#include <PipeFrame/Render/RenderContext.h>

#include "Editor/AntPanels.h"
#include "Integration/AntSimulationRuntime.h"

namespace ant_simulation {

AntSimulationRuntime::~AntSimulationRuntime() = default;

namespace {
const pipeframe::ui::ViewTheme& AntTheme() {
  static const pipeframe::ui::ViewTheme theme = [] {
    pipeframe::ui::ViewTheme value;
    value.glassSurface = {39, 43, 47, 208};
    value.floatingSurface = {42, 45, 49, 218};
    value.elevatedSurface = {48, 50, 54, 218};
    value.border = {238, 238, 235, 150};
    value.subtleBorder = {220, 222, 220, 95};
    value.textPrimary = {246, 244, 238};
    value.textSecondary = {195, 196, 194};
    value.accent = {242, 79, 112};
    value.accentHovered = {255, 101, 132};
    value.accentPressed = {211, 58, 91};
    value.controlSelected = {190, 58, 87};
    value.radiusMedium = 12.0f;
    value.radiusLarge = 20.0f;
    value.shadow = {8, 8, 10, 120};
    value.shadowOffsetSmall = {0.0f, 5.0f};
    return value;
  }();
  return theme;
}
}  // namespace

void AntSimulationRuntime::BuildDashboard() {
  UI().Clear();
  UI().SetTheme(AntTheme());
  pipeframe::ProjectDefinition<AntSimulationRuntime> project;
  project.UI<AntTimePanel>({.size = {220, 86}, .interactive = false});
  project.UI<AntToolsPanel>({.title = "FOOD / TOOLS",
    .size = {300, 380},
    .drawer = true,
    .drawerEdge = DrawerEdge::Left,
    .anchor = DrawerAnchor::Center});
  project.UI<AntSettingsPanel>({.title = "SETTINGS",
    .size = {360, 520},
    .drawer = true,
    .drawerEdge = DrawerEdge::Left,
    .anchor = DrawerAnchor::End});
  project.UI<AntColonyPanel>({.title = "COLONY",
    .size = {340, 540},
    .drawer = true,
    .drawerEdge = DrawerEdge::Right,
    .anchor = DrawerAnchor::Start});
  project.UI<AntSelectionPanel>({.title = "SELECTED ANT",
    .size = {360, 620},
    .drawer = true,
    .drawerEdge = DrawerEdge::Right,
    .anchor = DrawerAnchor::End});
  project.UI<AntProfilerPanel>({.title = "PROFILER",
    .size = {300, 480},
    .drawer = true,
    .drawerEdge = DrawerEdge::Left,
    .anchor = DrawerAnchor::Start});
  project.UI<AntControlPanel>({.title = "CONTROL",
    .size = {180, 300},
    .drawer = true,
    .drawerEdge = DrawerEdge::Bottom,
    .anchor = DrawerAnchor::Center});
  project.InstallUI(*this, UI());
  previewResources = std::make_shared<pipeframe::GraphicsResourceService>();
  const auto textures = projectDirectory / "Assets" / "Textures";
  previewBody = previewResources->LoadTexture((textures / "ant_body_parts.png").string());
  previewLeg = previewResources->LoadTexture((textures / "ant_leg.png").string());
  previewFood = previewResources->LoadTexture((textures / "circle.png").string());
  previewGeometry.ResizeDetailed(1);
}

void AntSimulationRuntime::RefreshDashboard() {
  if (!UI().Dashboard() || !simulationWorld) return;
  const bool available = antInspector.GetData().available;
  if (available && !selectedAntWasAvailable)
    UI().SetDrawerOpen(static_cast<std::size_t>(AntPanelKind::SelectedAnt), true);
  else if (!available && selectedAntWasAvailable)
    UI().SetDrawerOpen(static_cast<std::size_t>(AntPanelKind::SelectedAnt), false);
  selectedAntWasAvailable = available;
}

void AntPanel::OnUpdate(float) {
  if (kind == AntPanelKind::Time) runtime.RefreshDashboard();
  RefreshView();
}

pipeframe::ui::View AntPanel::BuildView() {
  return runtime.BuildDashboardView(kind);
}

bool AntSimulationRuntime::HandleUIEvent(
  const pipeframe::InputEvent& event, RenderContext& context) {
  const bool consumed = ProjectRuntime::HandleUIEvent(event, context);
  if (consumed && simulationWorld) editorTool.CancelStroke(simulationWorld->GetEnvironment());
  return consumed;
}
}  // namespace ant_simulation

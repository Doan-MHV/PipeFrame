#include <PipeFrame/UI/SimulationDashboard.h>
#include "Runtime/AntSimulationRuntime.h"
#include <PipeFrame/Render/RenderContext.h>
namespace ant_simulation {
AntSimulationRuntime::~AntSimulationRuntime()=default;
bool AntSimulationRuntime::HasUIFocus() const{return dashboard && dashboard->HasKeyboardFocus();}
namespace {
const pipeframe::ui::ViewTheme &AntTheme() {
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
}
void AntSimulationRuntime::BuildDashboard() {
    dashboard=std::make_unique<SimulationDashboard>(uiResources,uiFont,"ANT SIMULATION",AntTheme());
    dashboard->SetTabbedDrawerVisible(false);
    dashboard->AddViewMetric(220,86,[this]{return BuildDashboardView(6);});
    dashboard->AddViewDrawer(DrawerEdge::Left,"FOOD / TOOLS",300,380,DrawerAnchor::Center,[this]{return BuildDashboardView(0);});
    dashboard->AddViewDrawer(DrawerEdge::Left,"SETTINGS",360,520,DrawerAnchor::End,[this]{return BuildDashboardView(1);});
    dashboard->AddViewDrawer(DrawerEdge::Right,"COLONY",340,540,DrawerAnchor::Start,[this]{return BuildDashboardView(2);});
    dashboard->AddViewDrawer(DrawerEdge::Right,"SELECTED ANT",360,620,DrawerAnchor::End,[this]{return BuildDashboardView(3);});
    dashboard->AddViewDrawer(DrawerEdge::Left,"PROFILER",300,480,DrawerAnchor::Start,[this]{return BuildDashboardView(4);});
    dashboard->AddViewDrawer(DrawerEdge::Bottom,"CONTROL",180,300,DrawerAnchor::Center,[this]{return BuildDashboardView(5);});
    previewResources=std::make_shared<pipeframe::GraphicsResourceService>();
    const auto textures=projectDirectory/"Assets"/"Textures";
    previewBody=previewResources->LoadTexture((textures/"ant_body_parts.png").string());
    previewLeg=previewResources->LoadTexture((textures/"ant_leg.png").string());
    previewFood=previewResources->LoadTexture((textures/"circle.png").string());
    previewGeometry.ResizeDetailed(1);
}
void AntSimulationRuntime::RefreshDashboard() {
    if (!dashboard || !simulationWorld) return;
    const bool available=antInspector.GetData().available;
    if (available && !selectedAntWasAvailable) dashboard->SetDrawerOpen(3,true);
    else if (!available && selectedAntWasAvailable) dashboard->SetDrawerOpen(3,false);
    selectedAntWasAvailable=available;
    dashboard->InvalidateViews();
}
void AntSimulationRuntime::RenderScreen(RenderContext &context) {
    if (!dashboard || !loaded)
        return;
    dashboard->SetVisible(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
    if (!dashboard->IsVisible())
        return;
    RefreshDashboard();
    dashboard->Layout(context);
    dashboard->Render(context);
}
bool AntSimulationRuntime::ConsumesPointerAt(pipeframe::Vector2i point, const RenderContext &) const {
    return dashboard && viewMode != pipeframe::ProjectRuntimeViewMode::Zen && dashboard->ContainsPoint(point);
}
bool AntSimulationRuntime::HandleUIEvent(const pipeframe::InputEvent &event, RenderContext &context) {
    if (!dashboard)
        return false;
    dashboard->SetVisible(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
    dashboard->Layout(context);
    const bool consumed = dashboard->HandleEvent(event);
    if (consumed && simulationWorld)
        editorTool.CancelStroke(simulationWorld->GetEnvironment());
    return consumed;
}
} // namespace ant_simulation

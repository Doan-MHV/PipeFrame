#include <PipeFrame/Backend/SFML/ViewThemeAdapter.h>
#include <PipeFrame/Backend/SFML/DashboardAccess.h>
#include <stdexcept>
namespace {
const sf::Font &CheckedFont(const pipeframe::GraphicsResourceService &resources, pipeframe::FontHandle handle) {
    const auto *font = pipeframe::backend::sfml::GraphicsResourceAccess::Font(resources, handle);
    if (!font)
        throw std::invalid_argument("Dashboard requires a loaded font handle");
    return *font;
}

} // namespace
struct SimulationDashboard::Impl {
    NativeSimulationDashboard host;
    Impl(const pipeframe::GraphicsResourceService &resources, pipeframe::FontHandle font, const std::string &title,
         const pipeframe::ui::ViewTheme &theme)
        : host(CheckedFont(resources, font), title, pipeframe::backend::sfml::ToBackendTheme(theme)) {}
};
SimulationDashboard::SimulationDashboard(const pipeframe::GraphicsResourceService &resources,
                                         pipeframe::FontHandle font, const std::string &title,
                                         const pipeframe::ui::ViewTheme &theme)
    : impl(std::make_unique<Impl>(resources, font, title, theme)) {}
SimulationDashboard::~SimulationDashboard() = default;
void SimulationDashboard::AddViewDrawer(DrawerEdge edge, const std::string &title, float width, float height,
                                        DrawerAnchor anchor, std::function<pipeframe::ui::View()> build) {
    impl->host.AddViewDrawer(edge, title, width, height, anchor, std::move(build));
}
void SimulationDashboard::AddViewMetric(float width, float height, std::function<pipeframe::ui::View()> build, bool interactive) {
    impl->host.AddViewMetric(width, height, std::move(build),interactive);
}
void SimulationDashboard::RefreshViews() { impl->host.RefreshViews(); }
void SimulationDashboard::SetTabbedDrawerVisible(bool visible) { impl->host.SetTabbedDrawerVisible(visible); }
void SimulationDashboard::SetDrawerOpen(std::size_t index, bool open, bool animate) {
    impl->host.GetIndependentDrawer(index).SetOpen(open, animate);
}
bool SimulationDashboard::IsDrawerOpen(std::size_t index) const {
    return impl->host.GetIndependentDrawer(index).IsOpen();
}
std::size_t SimulationDashboard::GetIndependentDrawerCount() const { return impl->host.GetIndependentDrawerCount(); }
void SimulationDashboard::Layout(const RenderContext &context) { impl->host.Layout(context); }
void SimulationDashboard::Render(RenderContext &context) { impl->host.Render(context); }
bool SimulationDashboard::ContainsPoint(pipeframe::Vector2i point) const { return impl->host.ContainsPoint(point); }
bool SimulationDashboard::HandleEvent(const pipeframe::InputEvent &event) { return impl->host.HandleEvent(event); }
bool SimulationDashboard::HasKeyboardFocus() const { return impl->host.HasKeyboardFocus(); }
void SimulationDashboard::SetVisible(bool visible) { impl->host.SetVisible(visible); }
bool SimulationDashboard::IsVisible() const { return impl->host.IsVisible(); }
void SimulationDashboard::Update(float seconds) { impl->host.Update(seconds); }
float SimulationDashboard::GetFrameTimeMs() const { return impl->host.GetFrameTimeMs(); }
NativeSimulationDashboard &pipeframe::backend::sfml::DashboardAccess::Host(SimulationDashboard &dashboard) {
    return dashboard.impl->host;
}

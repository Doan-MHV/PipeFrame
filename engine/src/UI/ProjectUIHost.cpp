#include <PipeFrame/Backend/SFML/DashboardAccess.h>
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/Backend/SFML/ViewThemeAdapter.h>
#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/SimulationDashboard.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace pipeframe::ui {
Vector2f PanelPlacement::Position(Vector2f viewport, Vector2f size) const {
    const int value = static_cast<int>(anchor);
    const Vector2f fraction{float(value % 3) * .5f, float(value / 3) * .5f};
    const Vector2f target =
        unit == Unit::Percent ? Vector2f{viewport.x * point.x / 100, viewport.y * point.y / 100} : point;
    return {std::clamp(target.x + offset.x - size.x * fraction.x, 0.f, std::max(0.f, viewport.x - size.x)),
            std::clamp(target.y + offset.y - size.y * fraction.y, 0.f, std::max(0.f, viewport.y - size.y))};
}

struct ProjectUIHost::Impl {
    struct Entry {
        std::unique_ptr<ViewPanel> panel;
        PanelOptions placement;
        std::size_t nativeIndex{};
        bool shown{true};
        std::uint64_t revision{};
    };
    GraphicsResourceService resources;
    FontHandle font;
    std::vector<Entry> entries;
    ViewTheme theme = ViewTheme::Dark();
    std::unique_ptr<SimulationDashboard> dashboard;
    std::optional<View> pendingDialog;
    Vector2f dialogSize{440, 280};
    bool closeDialog{}, visible{true};
    std::unique_ptr<UIManager> modal;
    PopupLayer* popup{};
    ViewBuilderPanel* dialogPanel{};
    std::chrono::steady_clock::time_point lastFrame = std::chrono::steady_clock::now();
    bool Prepare(std::string& error) {
        if (dashboard || (entries.empty() && !pendingDialog)) return true;
        if (resources.State(font) != ResourceState::Ready) font = resources.LoadFont(PIPEFRAME_UI_FONT, &error);
        if (resources.State(font) != ResourceState::Ready) return false;
        dashboard = std::make_unique<SimulationDashboard>(resources, font, "Project UI", theme);
        dashboard->SetTabbedDrawerVisible(false);
        std::size_t metrics = 0, drawers = 0;
        for (auto& entry : entries) {
            auto* panel = entry.panel.get();
            const auto& p = entry.placement;
            if (p.drawer) {
                entry.nativeIndex = drawers++;
                dashboard->AddViewDrawer(p.drawerEdge.value_or(DrawerEdge::Right), p.title, p.size.x, p.size.y,
                                         p.anchor, [panel] { return panel->DescribeView(); });
            } else {
                entry.nativeIndex = metrics++;
                dashboard->AddViewMetric(p.size.x, p.size.y, [panel] { return panel->DescribeView(); }, p.interactive);
            }
        }
        backend::sfml::DashboardAccess::Host(*dashboard).SetProjectLayout([this](sf::Vector2f extent) {
            auto& native = backend::sfml::DashboardAccess::Host(*dashboard);
            float stackedY = 8;
            std::vector<std::size_t> positionedDrawers;
            for (auto& entry : entries) {
                const auto& p = entry.placement;
                if (p.drawer && !p.placement && p.drawerEdge) continue;  // Legacy explicitly anchored drawers.
                const Vector2f viewport{extent.x, extent.y};
                const Vector2f size{std::min(p.size.x, viewport.x), std::min(p.size.y, viewport.y)};
                const auto placement =
                    p.placement.value_or(p.drawer ? PanelPlacement::AtPercent(100, 50).Anchor(PanelAnchor::CenterRight)
                                                  : PanelPlacement::AtPixels((viewport.x - size.x) / 2, stackedY));
                const auto position = placement.Position(viewport, size);
                if (!p.drawer && !p.placement) stackedY += size.y + 8;
                const sf::FloatRect bounds{{position.x, position.y}, {size.x, size.y}};
                if (!p.drawer)
                    native.PlaceMetric(entry.nativeIndex, bounds, entry.shown);
                else {
                    const float distances[] = {position.x, viewport.x - position.x - size.x, position.y,
                                               viewport.y - position.y - size.y};
                    auto edge = static_cast<DrawerEdge>(std::min_element(std::begin(distances), std::end(distances)) -
                                                        std::begin(distances));
                    native.PlaceDrawer(entry.nativeIndex, bounds, p.drawerEdge.value_or(edge), extent);
                    positionedDrawers.push_back(entry.nativeIndex);
                }
            }
            native.ArrangeProjectHandles(positionedDrawers, extent);
        });
        dashboard->SetVisible(visible);
        return true;
    }
    void FlushDialog() {
        if (closeDialog) {
            modal.reset();
            popup = nullptr;
            dialogPanel = nullptr;
            closeDialog = false;
        }
        if (!pendingDialog) return;
        if (dashboard) dashboard->HandleEvent({InputEventType::FocusChanged, FocusInput{false}});
        modal = std::make_unique<UIManager>();
        popup = &modal->CreateRoot<PopupLayer>(backend::sfml::ToBackendTheme(theme));
        popup->SetDismissOnBackgroundClick(false);
        popup->SetOnDismiss([this] { closeDialog = true; });
        auto content = std::move(*pendingDialog);
        pendingDialog.reset();
        dialogPanel = &popup->GetContent().CreateChild<ViewBuilderPanel>(
            *backend::sfml::GraphicsResourceAccess::Font(resources, font),
            [content = std::move(content)] { return content; }, backend::sfml::ToBackendTheme(theme));
        popup->Open();
    }
    void Layout(RenderContext& context) {
        if (dashboard) dashboard->Layout(context);
        if (popup) {
            const auto r = backend::sfml::ViewportBounds(context);
            const sf::Vector2f size{std::max(1.f, std::min(dialogSize.x, float(r.size.x))),
                                    std::max(1.f, std::min(dialogSize.y, float(r.size.y)))};
            popup->Arrange({{float(r.position.x), float(r.position.y)}, {float(r.size.x), float(r.size.y)}});
            popup->SetContentBounds({{(r.size.x - size.x) / 2, (r.size.y - size.y) / 2}, size});
            dialogPanel->Arrange({{}, size});
            modal->Update(0);
        }
    }
};
ProjectUIHost::ProjectUIHost() : impl(std::make_unique<Impl>()) {}
ProjectUIHost::~ProjectUIHost() = default;
ViewPanel& ProjectUIHost::Add(std::unique_ptr<ViewPanel> panel, PanelOptions placement) {
    if (!panel) throw std::invalid_argument("Project UI requires a panel");
    if (impl->dashboard) throw std::logic_error("Register panels before preparing project UI");
    const auto finite = [](Vector2f value) { return std::isfinite(value.x) && std::isfinite(value.y); };
    if (!finite(placement.size) || placement.size.x <= 0 || placement.size.y <= 0)
        throw std::invalid_argument("Panel size must be finite and positive");
    if (placement.placement) {
        const auto& p = *placement.placement;
        if (!finite(p.point) || !finite(p.offset) || static_cast<int>(p.anchor) < 0 || static_cast<int>(p.anchor) > 8 ||
            (p.unit == PanelPlacement::Unit::Percent &&
             (p.point.x < 0 || p.point.x > 100 || p.point.y < 0 || p.point.y > 100)))
            throw std::invalid_argument("Panel placement must be finite; percentages must be between 0 and 100");
    }
    const auto index = impl->entries.size();
    panel->BindVisibility([this, index](bool shown) {
        auto& entry = impl->entries.at(index);
        entry.shown = shown;
        if (impl->dashboard) {
            if (entry.placement.drawer) impl->dashboard->SetDrawerOpen(entry.nativeIndex, shown);
            if (!shown) impl->dashboard->HandleEvent({InputEventType::FocusChanged, FocusInput{false}});
        }
    });
    auto& result = *panel;
    impl->entries.push_back({std::move(panel), std::move(placement), 0, true});
    return result;
}
std::size_t ProjectUIHost::PanelCount() const {
    return impl->entries.size();
}
bool ProjectUIHost::Prepare(std::string& error) {
    return impl->Prepare(error);
}
void ProjectUIHost::Clear() {
    impl = std::make_unique<Impl>();
}
void ProjectUIHost::SetTheme(ViewTheme theme) {
    if (impl->dashboard) throw std::logic_error("Set UI theme before Prepare");
    impl->theme = theme;
}
void ProjectUIHost::SetVisible(bool visible) {
    if (impl->visible && !visible) {
        if (impl->dashboard) impl->dashboard->HandleEvent({InputEventType::FocusChanged, FocusInput{false}});
        if (impl->modal) impl->modal->HandleEvent(sf::Event::FocusLost{});
    }
    impl->visible = visible;
    if (impl->dashboard) impl->dashboard->SetVisible(visible);
}
bool ProjectUIHost::IsVisible() const {
    return impl->visible;
}
void ProjectUIHost::Render(RenderContext& context) {
    if (!impl->visible) return;
    std::string error;
    if (!Prepare(error)) throw std::runtime_error(error);
    if (!impl->dashboard) return;
    const auto now = std::chrono::steady_clock::now();
    const auto delta = std::clamp(std::chrono::duration<float>(now - impl->lastFrame).count(), 0.f, .1f);
    impl->lastFrame = now;
    bool dirty = false;
    for (auto& entry : impl->entries) {
        entry.panel->OnUpdate(delta);
        if (entry.revision != entry.panel->ViewRevision()) {
            entry.revision = entry.panel->ViewRevision();
            dirty = true;
        }
    }
    if (dirty) impl->dashboard->RefreshViews();
    impl->FlushDialog();
    impl->Layout(context);
    context.BeginScreen();
    impl->dashboard->Render(context);
    if (impl->modal) {
        impl->modal->Update(delta);
        impl->modal->Render(backend::sfml::GetTarget(context));
    }
}
bool ProjectUIHost::HandleEvent(const InputEvent& event, RenderContext& context) {
    if (!impl->visible) return false;
    std::string error;
    if (!Prepare(error)) return false;
    impl->FlushDialog();
    impl->Layout(context);
    if (impl->modal) {
        if (event.type == InputEventType::KeyPressed && std::get<KeyInput>(event.payload).key == InputKey::Escape) {
            CloseDialog();
            impl->FlushDialog();
            return true;
        }
        if (const auto native = backend::sfml::ToBackend(event)) impl->modal->HandleEvent(*native);
        impl->FlushDialog();
        return true;
    }
    const bool consumed = impl->dashboard && impl->dashboard->HandleEvent(event);
    impl->FlushDialog();
    return consumed;
}
bool ProjectUIHost::ContainsPoint(Vector2i p) const {
    return impl->visible && (HasDialog() || (impl->dashboard && impl->dashboard->ContainsPoint(p)));
}
bool ProjectUIHost::HasKeyboardFocus() const {
    return impl->visible && (HasDialog() || (impl->dashboard && impl->dashboard->HasKeyboardFocus()));
}
void ProjectUIHost::ShowDialog(View content, Vector2f size) {
    impl->dialogSize = size;
    impl->pendingDialog = std::move(content);
}
void ProjectUIHost::CloseDialog() {
    impl->pendingDialog.reset();
    impl->closeDialog = true;
}
bool ProjectUIHost::HasDialog() const {
    return impl->pendingDialog.has_value() || (bool(impl->modal) && !impl->closeDialog);
}
void ProjectUIHost::SetDrawerOpen(std::size_t index, bool open) {
    if (impl->dashboard) impl->dashboard->SetDrawerOpen(index, open);
}
float ProjectUIHost::FrameTimeMs() const {
    return impl->dashboard ? impl->dashboard->GetFrameTimeMs() : 0;
}
SimulationDashboard* ProjectUIHost::Dashboard() const {
    return impl->dashboard.get();
}
}  // namespace pipeframe::ui

#pragma once
#include <PipeFrame/Input/InputEvent.h>
#include <PipeFrame/UI/DrawerTypes.h>
#include <PipeFrame/UI/ViewPanel.h>
#include <PipeFrame/UI/ViewTheme.h>

#include <memory>
#include <optional>
#include <string>
class RenderContext;
class SimulationDashboard;
namespace pipeframe::ui {
enum class PanelAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};
struct PanelPlacement {
    enum class Unit { Pixels, Percent };
    Unit unit{Unit::Pixels};
    Vector2f point{}, offset{};
    PanelAnchor anchor{PanelAnchor::TopLeft};
    static PanelPlacement AtPixels(float x, float y) { return {Unit::Pixels, {x, y}}; }
    static PanelPlacement AtPercent(float x, float y) { return {Unit::Percent, {x, y}}; }
    PanelPlacement Anchor(PanelAnchor value) const {
        auto copy = *this;
        copy.anchor = value;
        return copy;
    }
    PanelPlacement Offset(float x, float y) const {
        auto copy = *this;
        copy.offset = {x, y};
        return copy;
    }
    Vector2f Position(Vector2f viewport, Vector2f size) const;
};
struct PanelOptions {
    std::string title;
    Vector2f size{280, 180};
    std::optional<PanelPlacement> placement;
    bool drawer{false};
    // Omit to collapse toward the nearest edge of the configured placement.
    std::optional<DrawerEdge> drawerEdge;
    DrawerAnchor anchor{DrawerAnchor::Start};
    bool interactive{true};
};
// Engine-owned project UI. Projects supply presenters and data, never native widgets.
class ProjectUIHost {
public:
    ProjectUIHost();
    ~ProjectUIHost();
    ProjectUIHost(const ProjectUIHost&) = delete;
    ProjectUIHost& operator=(const ProjectUIHost&) = delete;
    ViewPanel& Add(std::unique_ptr<ViewPanel> panel, PanelOptions placement = {});
    bool Prepare(std::string& error);
    std::size_t PanelCount() const;
    void Clear();
    void SetTheme(ViewTheme theme);
    void SetVisible(bool visible);
    bool IsVisible() const;
    void Render(RenderContext& context);
    bool HandleEvent(const InputEvent& event, RenderContext& context);
    bool ContainsPoint(Vector2i point) const;
    bool HasKeyboardFocus() const;
    void ShowDialog(View content, Vector2f size = {440, 280});
    void CloseDialog();
    bool HasDialog() const;
    void SetDrawerOpen(std::size_t index, bool open);
    float FrameTimeMs() const;
    // Compatibility bridge for existing dashboard integrations and native host tests.
    SimulationDashboard* Dashboard() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}  // namespace pipeframe::ui

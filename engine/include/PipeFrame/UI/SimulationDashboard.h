#pragma once
#include <PipeFrame/UI/View.h>
#include <PipeFrame/UI/ViewTheme.h>
#include <PipeFrame/UI/DrawerTypes.h>
#include <PipeFrame/Input/InputEvent.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <memory>
#include <functional>
#include <string>
#include <cstddef>
class RenderContext;
namespace pipeframe::backend::sfml { struct DashboardAccess; }

// Public declarative dashboard. Resource service must outlive the dashboard.
class SimulationDashboard {
public:
    SimulationDashboard(const pipeframe::GraphicsResourceService &,pipeframe::FontHandle,
        const std::string &title,const pipeframe::ui::ViewTheme &theme=pipeframe::ui::ViewTheme::Dark());
    ~SimulationDashboard();
    SimulationDashboard(const SimulationDashboard &)=delete;
    SimulationDashboard &operator=(const SimulationDashboard &)=delete;
    void AddViewDrawer(DrawerEdge,const std::string &,float width,float height,DrawerAnchor,
                       std::function<pipeframe::ui::View()>);
    void AddViewMetric(float width,float height,std::function<pipeframe::ui::View()>);
    void InvalidateViews();
    void SetTabbedDrawerVisible(bool);
    void SetDrawerOpen(std::size_t index,bool open,bool animate=true);
    bool IsDrawerOpen(std::size_t index) const;
    std::size_t GetIndependentDrawerCount() const;
    void Layout(const RenderContext &);
    void Render(RenderContext &);
    bool ContainsPoint(pipeframe::Vector2i) const;
    bool HandleEvent(const pipeframe::InputEvent &);
    bool HasKeyboardFocus() const;
    void SetVisible(bool);
    bool IsVisible() const;
    void Update(float seconds);
    float GetFrameTimeMs() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    friend struct pipeframe::backend::sfml::DashboardAccess;
};

#include <PipeFrame/Backend/SFML/DashboardAccess.h>
#include <stdexcept>
namespace {
const sf::Font &CheckedFont(const pipeframe::GraphicsResourceService &resources,pipeframe::FontHandle handle) {
    const auto *font=pipeframe::backend::sfml::GraphicsResourceAccess::Font(resources,handle);
    if(!font) throw std::invalid_argument("Dashboard requires a loaded font handle");
    return *font;
}
UITheme NativeTheme(const pipeframe::ui::ViewTheme &value) {
    UITheme result;
    result.applicationBackground={value.applicationBackground.r,value.applicationBackground.g,value.applicationBackground.b,value.applicationBackground.a};
    result.surface={value.surface.r,value.surface.g,value.surface.b,value.surface.a};
    result.elevatedSurface={value.elevatedSurface.r,value.elevatedSurface.g,value.elevatedSurface.b,value.elevatedSurface.a};
    result.inputBackground={value.inputBackground.r,value.inputBackground.g,value.inputBackground.b,value.inputBackground.a};
    result.floatingSurface={value.floatingSurface.r,value.floatingSurface.g,value.floatingSurface.b,value.floatingSurface.a};
    result.glassSurface={value.glassSurface.r,value.glassSurface.g,value.glassSurface.b,value.glassSurface.a};
    result.scrim={value.scrim.r,value.scrim.g,value.scrim.b,value.scrim.a};
    result.shadow={value.shadow.r,value.shadow.g,value.shadow.b,value.shadow.a};
    result.border={value.border.r,value.border.g,value.border.b,value.border.a};
    result.subtleBorder={value.subtleBorder.r,value.subtleBorder.g,value.subtleBorder.b,value.subtleBorder.a};
    result.textPrimary={value.textPrimary.r,value.textPrimary.g,value.textPrimary.b,value.textPrimary.a};
    result.textSecondary={value.textSecondary.r,value.textSecondary.g,value.textSecondary.b,value.textSecondary.a};
    result.textDisabled={value.textDisabled.r,value.textDisabled.g,value.textDisabled.b,value.textDisabled.a};
    result.accent={value.accent.r,value.accent.g,value.accent.b,value.accent.a};
    result.accentHovered={value.accentHovered.r,value.accentHovered.g,value.accentHovered.b,value.accentHovered.a};
    result.accentPressed={value.accentPressed.r,value.accentPressed.g,value.accentPressed.b,value.accentPressed.a};
    result.controlNormal={value.controlNormal.r,value.controlNormal.g,value.controlNormal.b,value.controlNormal.a};
    result.controlHovered={value.controlHovered.r,value.controlHovered.g,value.controlHovered.b,value.controlHovered.a};
    result.controlPressed={value.controlPressed.r,value.controlPressed.g,value.controlPressed.b,value.controlPressed.a};
    result.controlDisabled={value.controlDisabled.r,value.controlDisabled.g,value.controlDisabled.b,value.controlDisabled.a};
    result.controlSelected={value.controlSelected.r,value.controlSelected.g,value.controlSelected.b,value.controlSelected.a};
    result.danger={value.danger.r,value.danger.g,value.danger.b,value.danger.a};
    result.warning={value.warning.r,value.warning.g,value.warning.b,value.warning.a};
    result.success={value.success.r,value.success.g,value.success.b,value.success.a};
    result.spacing2=value.spacing2;
    result.spacing4=value.spacing4;
    result.spacing8=value.spacing8;
    result.spacing12=value.spacing12;
    result.spacing16=value.spacing16;
    result.spacing24=value.spacing24;
    result.smallControlHeight=value.smallControlHeight;
    result.controlHeight=value.controlHeight;
    result.largeControlHeight=value.largeControlHeight;
    result.borderThickness=value.borderThickness;
    result.radiusSmall=value.radiusSmall;
    result.radiusMedium=value.radiusMedium;
    result.radiusLarge=value.radiusLarge;
    result.radiusPill=value.radiusPill;
    result.shadowOffsetSmall={value.shadowOffsetSmall.x,value.shadowOffsetSmall.y};
    result.shadowOffsetLarge={value.shadowOffsetLarge.x,value.shadowOffsetLarge.y};
    result.motionFast=value.motionFast;
    result.motionNormal=value.motionNormal;
    result.motionSlow=value.motionSlow;
    result.captionTextSize=value.captionTextSize;
    result.bodyTextSize=value.bodyTextSize;
    result.headingTextSize=value.headingTextSize;
    return result;
}
}
struct SimulationDashboard::Impl {
    NativeSimulationDashboard host;
    Impl(const pipeframe::GraphicsResourceService &resources,pipeframe::FontHandle font,
        const std::string &title,const pipeframe::ui::ViewTheme &theme)
        :host(CheckedFont(resources,font),title,NativeTheme(theme)) {}
};
SimulationDashboard::SimulationDashboard(const pipeframe::GraphicsResourceService &resources,
    pipeframe::FontHandle font,const std::string &title,const pipeframe::ui::ViewTheme &theme)
    :impl(std::make_unique<Impl>(resources,font,title,theme)) {}
SimulationDashboard::~SimulationDashboard()=default;
void SimulationDashboard::AddViewDrawer(DrawerEdge edge,const std::string &title,float width,float height,
    DrawerAnchor anchor,std::function<pipeframe::ui::View()> build) { impl->host.AddViewDrawer(edge,title,width,height,anchor,std::move(build)); }
void SimulationDashboard::AddViewMetric(float width,float height,std::function<pipeframe::ui::View()> build) {impl->host.AddViewMetric(width,height,std::move(build));}
void SimulationDashboard::InvalidateViews(){impl->host.InvalidateViews();}
void SimulationDashboard::SetTabbedDrawerVisible(bool visible){impl->host.SetTabbedDrawerVisible(visible);}
void SimulationDashboard::SetDrawerOpen(std::size_t index,bool open,bool animate){impl->host.GetIndependentDrawer(index).SetOpen(open,animate);}
bool SimulationDashboard::IsDrawerOpen(std::size_t index)const{return impl->host.GetIndependentDrawer(index).IsOpen();}
std::size_t SimulationDashboard::GetIndependentDrawerCount()const{return impl->host.GetIndependentDrawerCount();}
void SimulationDashboard::Layout(const RenderContext &context){impl->host.Layout(context);}
void SimulationDashboard::Render(RenderContext &context){impl->host.Render(context);}
bool SimulationDashboard::ContainsPoint(pipeframe::Vector2i point)const{return impl->host.ContainsPoint(point);}
bool SimulationDashboard::HandleEvent(const pipeframe::InputEvent &event){return impl->host.HandleEvent(event);}
bool SimulationDashboard::HasKeyboardFocus()const{return impl->host.HasKeyboardFocus();}
void SimulationDashboard::SetVisible(bool visible){impl->host.SetVisible(visible);}
bool SimulationDashboard::IsVisible()const{return impl->host.IsVisible();}
void SimulationDashboard::Update(float seconds){impl->host.Update(seconds);}
float SimulationDashboard::GetFrameTimeMs()const{return impl->host.GetFrameTimeMs();}
NativeSimulationDashboard &pipeframe::backend::sfml::DashboardAccess::Host(SimulationDashboard &dashboard){return dashboard.impl->host;}

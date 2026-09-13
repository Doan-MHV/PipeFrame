#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include "WorkbenchLayout.h"
#include "../../Editor/ViewportToolbar.h"
#include "../../Editor/HierarchyPanel.h"
#include "../../Editor/InspectorPanel.h"
#include "../../Editor/AssetBrowserPanel.h"
#include <PipeFrame/Backend/SFML/DockSplitter.h>
#include "../../Editor/WorkspaceToolsPanel.h"
#include <algorithm>
#include <utility>
#include <PipeFrame/Backend/SFML/UI/Component.h>
#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Render/RenderContext.h>
namespace {
void Clear(Panel &panel) { panel.SetFillColor(sf::Color::Transparent); panel.SetOutlineThickness(0); panel.SetHitTestVisible(false); }
}
WorkbenchLayout::WorkbenchLayout(const sf::Font &font) {
    Clear(*this); zenShell=&CreateChild<ZenModeShell>();
    using namespace pipeframe::ui;
    Compose(GetChromeLayer(),Component<Column>("frame",[](Column &column) {
        Clear(column); column.SetPadding(Thickness{12}); column.SetSpacing(8);
    }));
    frame=static_cast<Column *>(GetChromeLayer().FindChildByKey("frame"));
    toolbar=&frame->CreateChild<pipeframe::backend::sfml::HostedViewPanel<ViewportToolbar>>(font);
    workspace=&frame->CreateChild<Column>(); Clear(*workspace); workspace->SetSpacing(6); frame->SetChildFlex(*workspace,1);
    mainArea=&workspace->CreateChild<StackPanel>(); Clear(*mainArea); mainArea->SetSpacing(6); workspace->SetChildFlex(*mainArea,1);
    viewport=&mainArea->CreateChild<OverlayPanel>(); Clear(*viewport); viewport->SetPadding(Thickness{8}); mainArea->SetChildFlex(*viewport,1);
    sideSplitter=&mainArea->CreateChild<pipeframe::backend::sfml::DockSplitter>(pipeframe::backend::sfml::DockSplitter::Axis::Horizontal);
    sideSplitter->SetSize({6,0}); sideSplitter->SetSizePolicy(SizePolicy::Fixed,SizePolicy::Stretch);
    tools=&mainArea->CreateChild<StackPanel>(); Clear(*tools); tools->SetSpacing(8);
    hierarchy=&tools->CreateChild<pipeframe::backend::sfml::HostedViewPanel<HierarchyPanel>>(font); tools->SetChildFlex(*hierarchy,0.35f);
    inspector=&tools->CreateChild<pipeframe::backend::sfml::HostedViewPanel<InspectorPanel>>(font); tools->SetChildFlex(*inspector,0.65f);
    assets=&tools->CreateChild<pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::AssetBrowserPanel>>(font); tools->SetChildFlex(*assets,0.65f);
    assets->SetVisible(false);
    bottomSplitter=&workspace->CreateChild<pipeframe::backend::sfml::DockSplitter>(pipeframe::backend::sfml::DockSplitter::Axis::Vertical);
    bottomSplitter->SetSize({0,6}); bottomSplitter->SetSizePolicy(SizePolicy::Stretch,SizePolicy::Fixed);
    bottomTools=&workspace->CreateChild<pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel>>(font);
    bottomTools->SetSize({0,bottomDockSize}); bottomTools->SetSizePolicy(SizePolicy::Stretch,SizePolicy::Fixed);
    bottomSplitter->SetVisible(false); bottomTools->SetVisible(false);
    workspaceManager.RegisterPanel({"scene-tools",pipeframe::editor_workspace::DockSite::Right,"scene",0,true,true,sideDockSize});
    workspaceManager.RegisterPanel({"standard-tools",pipeframe::editor_workspace::DockSite::Bottom,"output",0,false,true,bottomDockSize});
    workspaceManager.RegisterPanel({"scene-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",0,true,true});
    workspaceManager.RegisterPanel({"game-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",1,true,false});
    sideSplitter->SetOnDragged([this](const float delta) {
        const bool horizontal=mainArea->GetOrientation()==StackOrientation::Horizontal;
        sideDockSize=std::clamp(sideDockSize-delta,240.0f,horizontal?640.0f:420.0f);
        workspaceManager.Resize("scene-tools",sideDockSize); SaveWorkspaceLayout();
        if(frame)frame->RefreshLayout();
    });
    bottomSplitter->SetOnDragged([this](const float delta) {
        bottomDockSize=std::clamp(bottomDockSize-delta,160.0f,480.0f);
        workspaceManager.Resize("standard-tools",bottomDockSize); SaveWorkspaceLayout();
        if(frame)frame->RefreshLayout();
    });
    recovery=&GetEssentialLayer().CreateChild<Column>(); Clear(*recovery); recovery->SetPadding(Thickness{12}); recovery->SetSpacing(8);
    recovery->SetSize({360,196}); GetEssentialLayer().SetChildAlignment(*recovery,{HorizontalAlignment::End,VerticalAlignment::End});
    exit=&recovery->CreateChild<pipeframe::ui::ViewBuilderPanel>(font,[this] {
        return pipeframe::ui::views::Button("exit-zen","EXIT ZEN",exitAction).FillHeight();
    }); exit->SetSize({0,36});
    transport=&recovery->CreateChild<SimulationTransport>(font);
}
void WorkbenchLayout::SetAssetBrowserVisible(const bool visible) {
    assets->SetVisible(visible); inspector->SetVisible(!visible);
    hierarchy->SetVisible(!visible);
    if(frame)frame->RefreshLayout();
}
void WorkbenchLayout::SetBottomDockVisible(const bool visible) {
    bottomDockVisible=visible;
    if(visible)workspaceManager.Dock("standard-tools",pipeframe::editor_workspace::DockSite::Bottom,"output");
    workspaceManager.Show("standard-tools",visible);
    bottomSplitter->SetVisible(visible); bottomTools->SetVisible(visible);
    SaveWorkspaceLayout();
    if(frame)frame->RefreshLayout();
}
bool WorkbenchLayout::IsBottomDockVisible() const { return bottomDockVisible; }
void WorkbenchLayout::SetStandardToolsFloating(const bool floating,const sf::FloatRect bounds){
    if(floating){
        const sf::FloatRect safe=bounds.size.x>0&&bounds.size.y>0?bounds:sf::FloatRect{{80,176},{640,420}};
        workspaceManager.Float("standard-tools",{{safe.position.x,safe.position.y},{safe.size.x,safe.size.y}},"primary");
        workspaceManager.Show("standard-tools",true);bottomDockVisible=false;
        bottomSplitter->SetVisible(false);bottomTools->SetVisible(false);
    }else{
        workspaceManager.Dock("standard-tools",pipeframe::editor_workspace::DockSite::Bottom,"output");
    }
    SaveWorkspaceLayout();if(frame)frame->RefreshLayout();
}
bool WorkbenchLayout::AreStandardToolsFloating() const{
    const auto *panel=workspaceManager.Find("standard-tools");
    return panel&&panel->site==pipeframe::editor_workspace::DockSite::Floating&&panel->visible;
}
sf::FloatRect WorkbenchLayout::GetStandardToolsFloatingBounds() const{
    const auto *panel=workspaceManager.Find("standard-tools");
    if(!panel)return {{80,176},{640,420}};
    return {{panel->floatingBounds.position.x,panel->floatingBounds.position.y},
            {panel->floatingBounds.size.x,panel->floatingBounds.size.y}};
}
void WorkbenchLayout::SelectViewportHost(const bool game){
    workspaceManager.SelectTab(game?"game-viewport":"scene-viewport");SaveWorkspaceLayout();
}
bool WorkbenchLayout::IsGameViewportSelected() const{
    const auto *panel=workspaceManager.Find("game-viewport");return panel&&panel->selected;
}
void WorkbenchLayout::ResetWorkspaceLayout() {
    sideDockSize=360.0f; bottomDockSize=220.0f; bottomDockVisible=false;
    workspaceManager.Reset();
    workspaceManager.RegisterPanel({"scene-tools",pipeframe::editor_workspace::DockSite::Right,"scene",0,true,true,sideDockSize});
    workspaceManager.RegisterPanel({"standard-tools",pipeframe::editor_workspace::DockSite::Bottom,"output",0,false,true,bottomDockSize});
    workspaceManager.RegisterPanel({"scene-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",0,true,true});
    workspaceManager.RegisterPanel({"game-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",1,true,false});
    bottomSplitter->SetVisible(false); bottomTools->SetVisible(false);
    SaveWorkspaceLayout(); if(frame)frame->RefreshLayout();
}
void WorkbenchLayout::SetPersistencePath(std::filesystem::path path) { persistencePath=std::move(path); LoadWorkspaceLayout(); }
bool WorkbenchLayout::SaveWorkspaceLayout(std::string *error) const {
    return persistencePath.empty() || workspaceManager.Save(persistencePath,error);
}
bool WorkbenchLayout::LoadWorkspaceLayout(std::string *error) {
    if(persistencePath.empty()||!std::filesystem::exists(persistencePath))return false;
    if(!workspaceManager.Load(persistencePath,error))return false;
    if(!workspaceManager.Find("scene-tools"))workspaceManager.RegisterPanel({"scene-tools",pipeframe::editor_workspace::DockSite::Right,"scene",0,true,true,sideDockSize});
    if(!workspaceManager.Find("standard-tools"))workspaceManager.RegisterPanel({"standard-tools",pipeframe::editor_workspace::DockSite::Bottom,"output",0,false,true,bottomDockSize});
    if(!workspaceManager.Find("scene-viewport"))workspaceManager.RegisterPanel({"scene-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",0,true,true});
    if(!workspaceManager.Find("game-viewport"))workspaceManager.RegisterPanel({"game-viewport",pipeframe::editor_workspace::DockSite::Center,"viewport",1,true,false});
    if(const auto *side=workspaceManager.Find("scene-tools"))sideDockSize=side->dockSize;
    if(const auto *bottom=workspaceManager.Find("standard-tools")){bottomDockSize=bottom->dockSize;bottomDockVisible=bottom->visible;}
    if(AreStandardToolsFloating())bottomDockVisible=false;
    bottomSplitter->SetVisible(bottomDockVisible);bottomTools->SetVisible(bottomDockVisible);
    if(frame)frame->RefreshLayout(); return true;
}
void WorkbenchLayout::Layout(sf::Vector2u size,bool zen) {
    SetSize(sf::Vector2f(size)); SetZenMode(zen); recovery->SetVisible(zen);
    toolbar->SetSize({0,std::min(ViewportToolbar::PreferredHeight(float(size.x)-24),float(size.y)*.35f)});
    const bool narrow=size.x<1000;
    mainArea->SetOrientation(narrow ? StackOrientation::Vertical : StackOrientation::Horizontal);
    sideSplitter->SetAxis(narrow ? pipeframe::backend::sfml::DockSplitter::Axis::Vertical : pipeframe::backend::sfml::DockSplitter::Axis::Horizontal);
    sideSplitter->SetSize(narrow ? sf::Vector2f{0,6} : sf::Vector2f{6,0});
    sideSplitter->SetSizePolicy(narrow ? SizePolicy::Stretch : SizePolicy::Fixed,narrow ? SizePolicy::Fixed : SizePolicy::Stretch);
    tools->SetOrientation(narrow ? StackOrientation::Horizontal : StackOrientation::Vertical);
    const float narrowSideLimit=bottomDockVisible?size.y*0.25f:size.y*0.42f;
    tools->SetSize(narrow ? sf::Vector2f{0,std::min(sideDockSize,narrowSideLimit)} : sf::Vector2f{std::min(sideDockSize,size.x*0.42f),0});
    tools->SetSizePolicy(narrow ? SizePolicy::Stretch : SizePolicy::Fixed,narrow ? SizePolicy::Fixed : SizePolicy::Stretch);
    const bool showBottom=bottomDockVisible;
    bottomSplitter->SetVisible(showBottom);bottomTools->SetVisible(showBottom);
    const float bottomLimit=narrow?size.y*0.25f:size.y*0.38f;
    bottomTools->SetSize({0,std::min(bottomDockSize,std::max(120.0f,bottomLimit))});
    frame->Arrange({{},GetSize()});
}
sf::FloatRect WorkbenchLayout::GetWorldBounds() const { return IsZenMode() ? GetBounds() : viewport->GetBounds(); }
void WorkbenchLayout::ApplyCamera(sf::Vector2u size,sf::FloatRect bounds,RenderContext &context) {
    const sf::Vector2f safe{static_cast<float>(std::max(1u,size.x)),static_cast<float>(std::max(1u,size.y))};
    context.GetCamera().SetSize({std::max(1.0f,bounds.size.x),std::max(1.0f,bounds.size.y)});
    context.GetCamera().SetViewport({{bounds.position.x/safe.x,bounds.position.y/safe.y},{bounds.size.x/safe.x,bounds.size.y/safe.y}});
}

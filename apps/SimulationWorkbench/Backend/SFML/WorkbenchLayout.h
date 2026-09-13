#include "../../Editor/ProjectBrowser.h"
#include "../../Editor/WorkspaceToolsPanel.h"
#include "../../Editor/AssetBrowserPanel.h"
#include "../../Editor/ViewportToolbar.h"
#include "../../Editor/InspectorPanel.h"
#include "../../Editor/HierarchyPanel.h"
#pragma once
#include <PipeFrame/Backend/SFML/UI/ZenModeShell.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/Project/EditorWorkspace.h>
#include <SFML/Graphics/Font.hpp>
#include <filesystem>
class ViewportToolbar;
class HierarchyPanel;
class InspectorPanel;
namespace pipeframe::editor { class AssetBrowserPanel; }
namespace pipeframe::editor { class WorkspaceToolsPanel; }
namespace pipeframe::backend::sfml { class DockSplitter; }
class SimulationTransport;
class TextButton;
class RenderContext;
class WorkbenchLayout final : public OverlayPanel {
public:
    explicit WorkbenchLayout(const sf::Font &font);
    void Layout(sf::Vector2u size, bool zen);
    sf::FloatRect GetWorldBounds() const;
    pipeframe::backend::sfml::HostedViewPanel<ViewportToolbar> &Toolbar() const { return *toolbar; }
    pipeframe::backend::sfml::HostedViewPanel<HierarchyPanel> &Hierarchy() const { return *hierarchy; }
    pipeframe::backend::sfml::HostedViewPanel<InspectorPanel> &Inspector() const { return *inspector; }
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::AssetBrowserPanel> &Assets() const { return *assets; }
    void SetAssetBrowserVisible(bool visible);
    void SetBottomDockVisible(bool visible);
    bool IsBottomDockVisible() const;
    void SetStandardToolsFloating(bool floating, sf::FloatRect bounds = {});
    bool AreStandardToolsFloating() const;
    sf::FloatRect GetStandardToolsFloatingBounds() const;
    void SelectViewportHost(bool game);
    bool IsGameViewportSelected() const;
    void ResetWorkspaceLayout();
    void SetPersistencePath(std::filesystem::path path);
    bool SaveWorkspaceLayout(std::string *error = nullptr) const;
    bool LoadWorkspaceLayout(std::string *error = nullptr);
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel> &BottomTools() const { return *bottomTools; }
    OverlayPanel &Viewport() const { return *viewport; }
    SimulationTransport &ZenTransport() const { return *transport; }
    pipeframe::ui::ViewBuilderPanel &ExitZen() const { return *exit; }
    void SetOnExitZen(std::function<void()> callback) { exitAction=std::move(callback); exit->InvalidateView(); }
    static void ApplyCamera(sf::Vector2u size, sf::FloatRect bounds, RenderContext &context);
private:
    ZenModeShell *zenShell = nullptr;
    OverlayPanel &GetChromeLayer() { return zenShell->GetChromeLayer(); }
    OverlayPanel &GetEssentialLayer() { return zenShell->GetEssentialLayer(); }
    void SetZenMode(bool zen) { zenShell->SetZenMode(zen); }
    bool IsZenMode() const { return zenShell->IsZenMode(); }
    Column *frame = nullptr;
    StackPanel *workspace = nullptr;
    StackPanel *mainArea = nullptr;
    StackPanel *tools = nullptr;
    pipeframe::backend::sfml::DockSplitter *sideSplitter = nullptr;
    pipeframe::backend::sfml::DockSplitter *bottomSplitter = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel> *bottomTools = nullptr;
    OverlayPanel *viewport = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<ViewportToolbar> *toolbar = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<HierarchyPanel> *hierarchy = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<InspectorPanel> *inspector = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::AssetBrowserPanel> *assets = nullptr;
    Column *recovery = nullptr;
    pipeframe::ui::ViewBuilderPanel *exit = nullptr;
    std::function<void()> exitAction;
    SimulationTransport *transport = nullptr;
    pipeframe::editor_workspace::WorkspaceManager workspaceManager;
    std::filesystem::path persistencePath;
    float sideDockSize{360.0f};
    float bottomDockSize{220.0f};
    bool bottomDockVisible{false};
};


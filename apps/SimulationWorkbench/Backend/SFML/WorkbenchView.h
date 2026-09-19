#ifndef PIPEFRAME_WORKBENCH_VIEW_H
#define PIPEFRAME_WORKBENCH_VIEW_H
#include "../../Editor/ProjectBrowser.h"
#include "../../Editor/WorkspaceToolsPanel.h"
#include "../../Editor/AssetBrowserPanel.h"
#include "../../Editor/ViewportToolbar.h"
#include "../../Editor/InspectorPanel.h"
#include "../../Editor/HierarchyPanel.h"


#include <PipeFrame/UI/FloatingWindowController.h>
#include <PipeFrame/Render/WorldDebugView.h>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <array>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>

#include "DiagnosticsOverlay.h"

class HierarchyPanel;
class InspectorPanel;
class RenderContext;
class TextButton;
class ViewportToolbar;
class WorkbenchLayout;
class PopupLayer;
class Column;

namespace pipeframe::editor {
class FloatingWorkspaceWindow;

class ProjectBrowser;
class AssetBrowserPanel;
class ProjectSession;
class SimulationSession;
class WorkspaceToolsPanel;

enum class WorkbenchWorkspaceMode {
    Editor,
    Simulation,
    Zen,
};

enum class TransformTool {
    Move,
    Rotate,
    Scale,
};

enum class TransformHandle {
    None,
    MoveFree,
    MoveX,
    MoveY,
    Rotate,
    ScaleUniform,
    ScaleX,
    ScaleY,
};

enum class TransformPivotMode {
    Center,
    Individual,
};

class WorkbenchView final {
  public:
    pipeframe::WorldDebugOptions GetWorldDebugOptions() const { return worldDebugOptions; }

    struct Callbacks {
        std::function<void()> newProject;
        std::function<void()> openProject;
        std::function<void(const std::filesystem::path &)> openRecentProject;

        std::function<void()> save;
        std::function<void()> saveAs;
        std::function<void()> undo;
        std::function<void()> redo;
        std::function<void()> reloadRuntime;
        std::function<void()> buildRuntime;

        std::function<void()> addObject;
        std::function<void(const SceneObjectTypeId &,std::optional<Vector2f>)> createObjectOfType;
        std::function<void(Vector2f)> placeObject;
        std::function<void()> deleteObject;
        std::function<void()> createPrefab;
        std::function<void()> applyPrefab;
        std::function<void()> revertPrefab;
        std::function<void()> unpackPrefab;

        std::function<void(SceneObjectId)> selectionChanged;

        std::function<void()> toggleSimulation;

        std::function<void()> singleStep;

        std::function<void()> resetSimulation;

        std::function<void(SimulationSpeed)> setSimulationSpeed;

        std::function<void(WorkbenchWorkspaceMode)> workspaceModeChanged;

        std::function<void(const SceneTransform &)> transformCommitted;

        std::function<void(const std::string &, const PropertyValue &)> propertyCommitted;
        std::function<void(const std::string &, const std::string &, const PropertyValue &)>
            componentPropertyCommitted;
        std::function<void(const std::string &,bool)> componentAttachment;
        std::function<std::string(const std::string &,const std::string &)> generateModule;
        std::function<void(const std::string &,const std::string &,const std::string &)> assignAssetProperty;
        std::function<void(const std::string &)> assignAsset;
        pipeframe::editor::AssetBrowserPanel::VisualActions visualAssets;
        std::function<void()> createMap,stopMap,makeMapUnique;
        std::function<void(const std::string &)> editMap;
        AssetBrowserPanel::EnvironmentActions environmentAssets;
        std::function<void(std::size_t,std::uint32_t,int)> paintShape;
        std::function<void()> discoverAssets;
        std::function<void()> importAsset;
        std::function<void(const std::string &)> reimportAsset;
        std::function<void(std::uint64_t)> cancelAssetOperation;
    };

    bool Load(const std::filesystem::path &fontPath);

    void SetCallbacks(Callbacks callbacks);

    void Layout(sf::Vector2u windowSize, RenderContext &context);
    void SetWorkspaceLayoutPath(std::filesystem::path path);

    bool HandleEvent(const sf::Event &event);

    bool HasKeyboardFocus() const;
    void ShowContextMenu(sf::Vector2f position);
    void ShowCreateObjectPicker(std::optional<Vector2f> position = std::nullopt);
    bool HasBlockingOverlay() const;
    WorkbenchLayout &GetShell() const { return *shell; }
    PopupLayer &GetContextMenu() const { return *contextMenu; }
    void RenderUI(sf::RenderTarget &target) const;


    void SetPointerState(bool insideViewport, sf::Vector2f worldPosition);

    bool IsPointerInsideViewport() const;

    sf::Vector2f GetPointerWorldPosition() const;

    void ShowProjectBrowser(const std::vector<std::filesystem::path> &recentProjects, const std::string &message = {});

    void ShowWorkspace();

    bool IsWorkspaceVisible() const;
    bool IsAssetBrowserVisible() const { return assetsVisible; }

    void SetWorkspaceMode(WorkbenchWorkspaceMode mode);

    void CycleWorkspaceMode();

    [[nodiscard]]
    WorkbenchWorkspaceMode GetWorkspaceMode() const;

    void SetBuildProgress(bool running,const std::string &output);
    void Refresh(const ProjectSession &project, const SimulationSession &simulation, bool rebuildHierarchy);

    void Update(float deltaTime);

    void RenderWorldOverlay(RenderContext &context) const;
    void RenderWorldBackground(RenderContext &context) const;

    bool IsGridVisible() const { return gridVisible; }
    bool IsPositionSnapEnabled() const { return positionSnapEnabled; }
    float GetPositionSnapStep() const { return positionSnapStep; }
    bool IsRotationSnapEnabled() const { return rotationSnapEnabled; }
    float GetRotationSnapStep() const { return rotationSnapStep; }
    bool IsScaleSnapEnabled() const { return scaleSnapEnabled; }
    float GetScaleSnapStep() const { return scaleSnapStep; }
    float GetGridStep() const { return gridStep; }
    Vector2f GetGridOrigin() const { return {gridOrigin.x, gridOrigin.y}; }
    float GetDisplayedGridStep() const { return displayedGridStep; }
    TransformTool GetTransformTool() const { return transformTool; }
    bool IsLocalTransformSpace() const { return localTransformSpace; }
    TransformPivotMode GetTransformPivotMode() const { return pivotMode; }
    TransformHandle HitTestTransformHandle(sf::Vector2f worldPosition) const;
    void SetGridStep(float step);
    void SetGridOrigin(Vector2f origin);
    void CycleGridStep();
    void CyclePivotMode();
    bool IsPlacementPreviewVisible() const { return placementPreviewWorld.has_value(); }
    std::size_t GetAttachmentPreviewCount() const { return attachmentPreviewCount; }
    std::size_t GetConnectionPreviewCount() const { return connectionPreviewCount; }
    void ToggleAssetsPanel();
    void ToggleStandardPanels();
    void SetToolsFloating(bool floating);
    bool AreToolsFloating() const;
    sf::FloatRect GetFloatingToolsBounds() const;

    void SetTransformTool(TransformTool tool);
    void SetHoveredObject(std::optional<SceneObjectId> objectId);
    void SetSelectionBox(std::optional<sf::FloatRect> bounds);
    void FrameObjects(std::span<const SceneObjectData> objects);
    void FrameSelectionOrScene();
    void SaveCameraBookmark(std::size_t index);
    bool RecallCameraBookmark(std::size_t index);

    void RenderScreen(RenderContext &context, const SimulationSession &simulation);

  private:
    void CreateInterface();
    void BindCallbacks();

    void ApplyModeVisibility();

    void ApplyWorkspaceLayout();

    void RebuildHierarchy(const ProjectSession &project);

    void RefreshInspector(const ProjectSession &project, const SimulationSession &simulation);

    void RefreshToolbar(const ProjectSession &project, const SimulationSession &simulation);

    void RefreshSimulationControls(const SimulationSession &simulation);

    UIManager uiManager;
    DiagnosticsOverlay *diagnosticsOverlay = nullptr;
    WorkbenchLayout *shell = nullptr;
    PopupLayer *contextMenu = nullptr;
    PopupLayer *toolsPopup = nullptr;
    FloatingWorkspaceWindow *floatingToolsWindow = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel> *floatingToolsPanel = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel> *compactToolsPanel = nullptr;
    pipeframe::ui::ViewBuilderPanel *contextActions = nullptr;
    std::vector<pipeframe::ui::View> contextItems;
    std::vector<SceneObjectTypeDescriptor> availableObjectTypes;
    bool choosingModule{};
    std::string moduleKind{"Entity"},moduleName,moduleMessage;
    bool choosingObjectType{false}, canCreateObject{false};
    std::string objectTypeSearch;
    std::optional<Vector2f> objectCreationPosition;

    Callbacks callbacks;

    pipeframe::backend::sfml::HostedViewPanel<ViewportToolbar> *toolbar = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<HierarchyPanel> *hierarchyPanel = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<InspectorPanel> *inspectorPanel = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::AssetBrowserPanel> *assetBrowserPanel = nullptr;
    pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::ProjectBrowser> *projectBrowser = nullptr;
    pipeframe::ui::ViewBuilderPanel *zenExitButton = nullptr;

    sf::RectangleShape viewportBorder;
    sf::CircleShape worldCursor;

    sf::Vector2f pointerWorldPosition{
        0.0f,
        0.0f,
    };

    WorkbenchWorkspaceMode workspaceMode{WorkbenchWorkspaceMode::Editor};

    sf::Vector2u layoutWindowSize{1, 1};

    RenderContext *layoutContext = nullptr;

    bool pointerInsideViewport = false;
    bool metricsVisible = false;
    bool assetsVisible = false;
    bool panelsVisible = false;
    pipeframe::ui::FloatingWindowController floatingWindowController;
    pipeframe::WorldDebugOptions worldDebugOptions;
    bool gridVisible = true;
    bool positionSnapEnabled = false;
    float positionSnapStep = 10.0f;
    bool rotationSnapEnabled = false;
    float rotationSnapStep = 15.0f;
    bool scaleSnapEnabled = false;
    float scaleSnapStep = 0.1f;
    float gridStep = 0.0f;
    sf::Vector2f gridOrigin{};
    mutable float displayedGridStep = 0.0f;
    TransformTool transformTool{TransformTool::Move};
    bool localTransformSpace = false;
    TransformPivotMode pivotMode{TransformPivotMode::Center};
    std::optional<SceneObjectId> hoveredObjectId;
    std::optional<std::string> draggedAssetId;
    std::optional<sf::Vector2f> placementPreviewWorld;
    sf::Vector2f contextActionWorldPosition{};
    std::optional<sf::FloatRect> selectionBox;
    std::vector<SceneObjectData> selectionRenderObjects;
    std::vector<SceneObjectData> sceneRenderObjects;
    std::vector<SceneConnectionData> sceneConnections;
    std::vector<SceneComponentTypeDescriptor> componentTypes;
    std::size_t attachmentPreviewCount{};
    std::size_t connectionPreviewCount{};
    struct CameraState { pipeframe::Vector2f center{}; float zoom{1.0f}; bool valid{false}; };
    CameraState editorCamera;
    CameraState simulationCamera;
    std::array<CameraState,4> cameraBookmarks{};
    bool workspaceVisible = false;
    bool loaded = false;
};

} // namespace pipeframe::editor

#endif


#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include "WorkbenchView.h"
#include <PipeFrame/Environment/PlaygroundGeometry.h>

#include <iostream>
#include <array>
#include <cmath>
#include <sstream>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/Text.hpp>

#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>
#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

#include "../../Editor/HierarchyPanel.h"
#include "../../Editor/AssetBrowserPanel.h"
#include "../../Editor/InspectorPanel.h"
#include "../../Editor/ProjectBrowser.h"
#include "../../Editor/ProjectSession.h"
#include "../../Editor/ViewportToolbar.h"
#include "WorkbenchLayout.h"
#include "../../Editor/WorkspaceToolsPanel.h"

#include "../../Runtime/SimulationSession.h"

namespace pipeframe::editor {

class FloatingWorkspaceWindow final : public Column {
public:
    explicit FloatingWorkspaceWindow(const sf::Font &font) {
        const auto &theme=UITheme::Dark();
        SetFillColor(theme.floatingSurface);SetOutlineColor(theme.border);SetOutlineThickness(1.0f);
        SetCornerRadius(theme.radiusMedium);SetShadowColor(theme.shadow);SetShadowOffset(theme.shadowOffsetLarge);
        SetPadding(Thickness{8});SetSpacing(6);SetMinimumSize({360,240});
        header=&CreateChild<pipeframe::ui::ViewBuilderPanel>(font,[this] {
            using namespace pipeframe::ui;
            return views::Row("window-actions",{views::Text("title","WORKSPACE TOOLS"),
                views::Button("dock","DOCK",onDock).Width(62),views::Button("close","CLOSE",onClose).Width(66)});
        });
        header->SetSize({0,34}); header->SetSizePolicy(SizePolicy::Stretch,SizePolicy::Fixed);
        tools=&CreateChild<pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel>>(font);tools->SetSizePolicy(SizePolicy::Stretch,SizePolicy::Stretch);SetChildFlex(*tools,1);
    }
    pipeframe::backend::sfml::HostedViewPanel<WorkspaceToolsPanel> &Tools() const { return *tools; }
    void SetOnDock(std::function<void()> callback){onDock=std::move(callback);header->InvalidateView();}
    void SetOnClose(std::function<void()> callback){onClose=std::move(callback);header->InvalidateView();}
    bool HitTitlebar(const sf::Vector2f point) const {
        if(!header->Contains(point))return false;
        for(auto *hit=header->FindTopmostAt(point);hit&&hit!=header;hit=hit->GetParent())
            if(hit->GetKey()=="dock"||hit->GetKey()=="close")return false;
        return true;
    }
    bool HitResizeGrip(const sf::Vector2f point) const {
        const auto bounds=GetBounds();return point.x>=bounds.position.x+bounds.size.x-20&&
            point.y>=bounds.position.y+bounds.size.y-20&&bounds.contains(point);
    }
private:
    pipeframe::ui::ViewBuilderPanel *header{};
    std::function<void()> onDock,onClose;
    pipeframe::backend::sfml::HostedViewPanel<WorkspaceToolsPanel> *tools{};
};

namespace {
constexpr float Pi = 3.14159265358979323846f;

float DistanceToSegment(const sf::Vector2f point, const sf::Vector2f start, const sf::Vector2f end) {
    const sf::Vector2f segment = end - start;
    const float lengthSquared = segment.x * segment.x + segment.y * segment.y;
    if (lengthSquared <= 0.000001f) return std::hypot(point.x - start.x, point.y - start.y);
    const sf::Vector2f relative = point - start;
    const float t = std::clamp((relative.x * segment.x + relative.y * segment.y) / lengthSquared, 0.0f, 1.0f);
    const sf::Vector2f closest = start + segment * t;
    return std::hypot(point.x - closest.x, point.y - closest.y);
}

sf::Vector2f RotatePoint(const sf::Vector2f value, const float degrees) {
    const float angle = degrees * Pi / 180.0f;
    return {value.x * std::cos(angle) - value.y * std::sin(angle),
            value.x * std::sin(angle) + value.y * std::cos(angle)};
}
}

bool WorkbenchView::Load(const std::filesystem::path &fontPath) {
    if (!uiManager.LoadDefaultFont(fontPath)) {
        std::cerr << "Unable to load UI font: " << fontPath << '\n';

        return false;
    }

    const UITheme &theme = UITheme::Dark();

    viewportBorder.setFillColor(sf::Color::Transparent);

    viewportBorder.setOutlineColor(theme.border);

    viewportBorder.setOutlineThickness(1.0f);

    worldCursor.setRadius(7.0f);

    worldCursor.setOrigin({7.0f, 7.0f});

    worldCursor.setFillColor(sf::Color::Transparent);

    worldCursor.setOutlineColor(theme.success);

    worldCursor.setOutlineThickness(1.5f);


    CreateInterface();

    loaded = true;

    return true;
}

void WorkbenchView::SetCallbacks(Callbacks newCallbacks) {
    callbacks = std::move(newCallbacks);

    if (loaded) {
        BindCallbacks();
    }
}

void WorkbenchView::Layout(const sf::Vector2u windowSize, RenderContext &context) {
    layoutWindowSize = windowSize;
    layoutContext = &context;

    if (projectBrowser != nullptr) {
        projectBrowser->SetPosition({0.0f, 0.0f});

        projectBrowser->SetSize({static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
    }

    if(contextMenu) contextMenu->SetSize(sf::Vector2f(windowSize));
    if(toolsPopup){
        toolsPopup->SetSize(sf::Vector2f(windowSize));
        const float margin=16.0f;
        toolsPopup->SetContentBounds({{margin,std::min(164.0f,static_cast<float>(windowSize.y)*0.25f)},
                                      {std::max(1.0f,static_cast<float>(windowSize.x)-margin*2.0f),
                                       std::max(160.0f,static_cast<float>(windowSize.y)-180.0f)}});
        if(compactToolsPanel){compactToolsPanel->SetPosition({});compactToolsPanel->SetSize(toolsPopup->GetContent().GetSize());}
    }
    if(floatingToolsWindow&&floatingToolsWindow->IsVisible()){
        auto bounds=floatingToolsWindow->GetBounds();
        const auto constrained=pipeframe::ui::FloatingWindowController::Constrain(
            {{bounds.position.x,bounds.position.y},{bounds.size.x,bounds.size.y}},
            {float(windowSize.x),float(windowSize.y)});
        floatingToolsWindow->Arrange({{constrained.position.x,constrained.position.y},
                                      {constrained.size.x,constrained.size.y}});
    }
    ApplyWorkspaceLayout();
}

void WorkbenchView::SetWorkspaceLayoutPath(std::filesystem::path path) {
    if(!shell)return;
    shell->SetPersistencePath(std::move(path));
    panelsVisible=shell->IsBottomDockVisible()||shell->AreStandardToolsFloating();
    if(floatingToolsWindow&&shell->AreStandardToolsFloating()){
        floatingToolsWindow->SetVisible(true);floatingToolsWindow->Arrange(shell->GetStandardToolsFloatingBounds());
    }
    toolbar->SetPanelsVisible(panelsVisible);
}

bool WorkbenchView::HandleEvent(const sf::Event &event) {
    if(const auto *key=event.getIf<sf::Event::KeyPressed>();key&&!key->system&&!key->control&&!key->alt){
        if(key->code==sf::Keyboard::Key::F6){CycleWorkspaceMode();return true;}
        if(key->code==sf::Keyboard::Key::F7){ToggleAssetsPanel();return true;}
        if(key->code==sf::Keyboard::Key::F8){ToggleStandardPanels();return true;}
    }
    if(floatingToolsWindow&&floatingToolsWindow->IsVisible()){
        if(const auto *pressed=event.getIf<sf::Event::MouseButtonPressed>();pressed&&pressed->button==sf::Mouse::Button::Left){
            const sf::Vector2f point(pressed->position);
            if(floatingToolsWindow->HitResizeGrip(point)||floatingToolsWindow->HitTitlebar(point)){
                const auto bounds=floatingToolsWindow->GetBounds();
                floatingWindowController.Begin(floatingToolsWindow->HitResizeGrip(point)
                    ? pipeframe::ui::FloatingWindowController::Operation::Resize
                    : pipeframe::ui::FloatingWindowController::Operation::Move,
                    {point.x,point.y},{{bounds.position.x,bounds.position.y},{bounds.size.x,bounds.size.y}});
                return true;
            }
        }
        if(const auto *moved=event.getIf<sf::Event::MouseMoved>();moved&&floatingWindowController.IsActive()){
            const auto bounds=floatingWindowController.Update({float(moved->position.x),float(moved->position.y)},
                                                               {float(layoutWindowSize.x),float(layoutWindowSize.y)});
            floatingToolsWindow->Arrange({{bounds.position.x,bounds.position.y},{bounds.size.x,bounds.size.y}});
            return true;
        }
        if(const auto *released=event.getIf<sf::Event::MouseButtonReleased>();released&&released->button==sf::Mouse::Button::Left&&floatingWindowController.IsActive()){
            floatingWindowController.End();shell->SetStandardToolsFloating(true,floatingToolsWindow->GetBounds());return true;
        }
        if((event.is<sf::Event::FocusLost>()||event.is<sf::Event::MouseLeft>())&&floatingWindowController.IsActive()){
            floatingWindowController.End();shell->SetStandardToolsFloating(true,floatingToolsWindow->GetBounds());
        }
    }
    if(assetsVisible&&assetBrowserPanel){
        if(const auto *pressed=event.getIf<sf::Event::MouseButtonPressed>();pressed&&pressed->button==sf::Mouse::Button::Left)
            draggedAssetId=assetBrowserPanel->AssetAt({float(pressed->position.x),float(pressed->position.y)});
        if(const auto *moved=event.getIf<sf::Event::MouseMoved>();moved&&draggedAssetId&&layoutContext){
            placementPreviewWorld=shell->Viewport().Contains(sf::Vector2f(moved->position))
                                      ? std::optional{pipeframe::backend::sfml::ScreenToWorld(*layoutContext,moved->position)} : std::nullopt;
        }
        if(const auto *released=event.getIf<sf::Event::MouseButtonReleased>();released&&released->button==sf::Mouse::Button::Left&&draggedAssetId){
            const bool dropped=shell->Viewport().Contains(sf::Vector2f(released->position));
            const std::string assetId=*draggedAssetId; draggedAssetId.reset(); placementPreviewWorld.reset();
            const bool consumed=uiManager.HandleEvent(event);
            if(dropped&&callbacks.assignAsset)callbacks.assignAsset(assetId);
            return dropped||consumed;
        }
        if(event.is<sf::Event::FocusLost>()||event.is<sf::Event::MouseLeft>()){draggedAssetId.reset();placementPreviewWorld.reset();}
    }
    return uiManager.HandleEvent(event);
}

bool WorkbenchView::HasKeyboardFocus() const { return uiManager.HasKeyboardFocus(); }
void WorkbenchView::RenderUI(sf::RenderTarget &target) const { uiManager.Render(target); }
bool WorkbenchView::HasBlockingOverlay() const { return !workspaceVisible ||
    (contextMenu && contextMenu->IsOpen()) || (toolsPopup && toolsPopup->IsOpen()); }
void WorkbenchView::ShowContextMenu(sf::Vector2f position) {
    choosingObjectType=false;contextActions->InvalidateView();
    contextActionWorldPosition=pointerWorldPosition;
    contextMenu->SetContentBounds({position,{240,548}});
    contextActions->Arrange({{},contextMenu->GetContent().GetSize()});
    contextMenu->Open();
}

void WorkbenchView::ShowCreateObjectPicker(std::optional<Vector2f> position) {
    if(!canCreateObject)return;
    choosingObjectType=true;choosingModule=false;objectTypeSearch.clear();objectCreationPosition=position;
    if(!position && layoutContext) {
        const auto center=layoutContext->GetCamera().GetCenter();objectCreationPosition=Vector2f{center.x,center.y};
    }
    const float width=std::min(380.f,std::max(1.f,float(layoutWindowSize.x)-32.f));
    const float height=std::min(420.f,std::max(1.f,float(layoutWindowSize.y)-32.f));
    contextMenu->SetContentBounds({{(layoutWindowSize.x-width)*.5f,(layoutWindowSize.y-height)*.5f},{width,height}});
    contextActions->Arrange({{},contextMenu->GetContent().GetSize()});
    contextActions->InvalidateView();contextMenu->Open();
}


void WorkbenchView::SetPointerState(const bool insideViewport, const sf::Vector2f worldPosition) {
    pointerInsideViewport = insideViewport;

    pointerWorldPosition = worldPosition;

    worldCursor.setPosition(worldPosition);
}

void WorkbenchView::SetTransformTool(const TransformTool tool) {
    transformTool = tool;
    if (toolbar == nullptr) return;
    switch (tool) {
    case TransformTool::Move: toolbar->SetToolText("MOVE"); break;
    case TransformTool::Rotate: toolbar->SetToolText("ROTATE"); break;
    case TransformTool::Scale: toolbar->SetToolText("SCALE"); break;
    }
}

void WorkbenchView::SetGridStep(const float step) {
    gridStep = step <= 0.0f ? 0.0f : std::clamp(step, 0.001f, 100000.0f);
    if (toolbar) toolbar->SetGridStep(gridStep);
}

void WorkbenchView::CycleGridStep() {
    static constexpr std::array steps{0.0f, 1.0f, 5.0f, 10.0f, 25.0f, 50.0f};
    const auto current = std::ranges::find(steps, gridStep);
    SetGridStep(current == steps.end() || std::next(current) == steps.end() ? steps.front() : *std::next(current));
}

void WorkbenchView::SetGridOrigin(const Vector2f origin) {
    gridOrigin = {origin.x, origin.y};
    if (toolbar) toolbar->SetGridOrigin(gridOrigin.x, gridOrigin.y);
}

void WorkbenchView::CyclePivotMode() {
    pivotMode = pivotMode == TransformPivotMode::Center ? TransformPivotMode::Individual
                                                        : TransformPivotMode::Center;
    if (toolbar) toolbar->SetPivotText(pivotMode == TransformPivotMode::Center ? "CENTER" : "INDIVIDUAL");
}

TransformHandle WorkbenchView::HitTestTransformHandle(const sf::Vector2f worldPosition) const {
    if (!layoutContext || selectionRenderObjects.empty()) return TransformHandle::None;
    sf::Vector2f pivot{};
    for (const auto &object : selectionRenderObjects)
        pivot += pipeframe::backend::sfml::ToBackend(object.transform.position);
    pivot /= static_cast<float>(selectionRenderObjects.size());
    const auto viewport = pipeframe::backend::sfml::ViewportBounds(*layoutContext);
    const float pixelsPerWorld = static_cast<float>(std::max(1, viewport.size.x)) /
                                 std::max(0.001f, layoutContext->GetCamera().GetSize().x);
    const float tolerance = 7.0f / pixelsPerWorld;
    const float axisLength = 46.0f / pixelsPerWorld;
    const float angle = localTransformSpace ? selectionRenderObjects.back().transform.rotation * Pi / 180.0f : 0.0f;
    const sf::Vector2f xAxis{std::cos(angle), std::sin(angle)};
    const sf::Vector2f yAxis{-std::sin(angle), std::cos(angle)};
    const float centerDistance = std::hypot(worldPosition.x - pivot.x, worldPosition.y - pivot.y);
    if (transformTool == TransformTool::Rotate) {
        return std::abs(centerDistance - 42.0f / pixelsPerWorld) <= tolerance ? TransformHandle::Rotate
                                                                              : TransformHandle::None;
    }
    if (centerDistance <= 9.0f / pixelsPerWorld)
        return transformTool == TransformTool::Move ? TransformHandle::MoveFree : TransformHandle::ScaleUniform;
    if (DistanceToSegment(worldPosition, pivot + xAxis * (10.0f / pixelsPerWorld), pivot + xAxis * axisLength) <= tolerance)
        return transformTool == TransformTool::Move ? TransformHandle::MoveX : TransformHandle::ScaleX;
    if (DistanceToSegment(worldPosition, pivot + yAxis * (10.0f / pixelsPerWorld), pivot + yAxis * axisLength) <= tolerance)
        return transformTool == TransformTool::Move ? TransformHandle::MoveY : TransformHandle::ScaleY;
    return TransformHandle::None;
}

void WorkbenchView::SetHoveredObject(const std::optional<SceneObjectId> objectId) {
    hoveredObjectId = objectId;
}

void WorkbenchView::SetSelectionBox(const std::optional<sf::FloatRect> bounds) {
    selectionBox = bounds;
}

void WorkbenchView::FrameObjects(const std::span<const SceneObjectData> objects) {
    if (layoutContext == nullptr || objects.empty()) return;
    float left = objects.front().transform.position.x;
    float right = left;
    float top = objects.front().transform.position.y;
    float bottom = top;
    for (const auto &object : objects) {
        left = std::min(left, object.transform.position.x);
        right = std::max(right, object.transform.position.x);
        top = std::min(top, object.transform.position.y);
        bottom = std::max(bottom, object.transform.position.y);
        for(const auto &component:object.components)if(component.typeId==PlaygroundComponentTypeId){
            PlaygroundComponent ground;std::string error;
            if(!PlaygroundComponent::Schema().Apply(ground,component.properties,error))continue;
            Transform2DComponent pose;pose.position=object.transform.position;pose.rotation=object.transform.rotation;pose.scale=object.transform.scale;
            const auto bounds=PlaygroundBounds(pose,ground);
            left=std::min(left,bounds.position.x);top=std::min(top,bounds.position.y);
            right=std::max(right,bounds.position.x+bounds.size.x);bottom=std::max(bottom,bounds.position.y+bounds.size.y);
        }
    }
    auto &camera = layoutContext->GetCamera();
    camera.SetCenter({(left + right) * 0.5f, (top + bottom) * 0.5f});
    const auto viewport = pipeframe::backend::sfml::ViewportBounds(*layoutContext);
    const float requiredWidth = std::max(160.0f, (right - left) + 160.0f);
    const float requiredHeight = std::max(120.0f, (bottom - top) + 120.0f);
    const float baseWidth = std::max(1.0f, static_cast<float>(viewport.size.x));
    const float baseHeight = std::max(1.0f, static_cast<float>(viewport.size.y));
    camera.SetZoom(std::clamp(std::max(requiredWidth / baseWidth, requiredHeight / baseHeight), 0.1f, 10.0f));
}

void WorkbenchView::FrameSelectionOrScene() {
    if (selectionRenderObjects.empty()) FrameObjects(sceneRenderObjects);
    else FrameObjects(selectionRenderObjects);
}

void WorkbenchView::SaveCameraBookmark(const std::size_t index) {
    if (layoutContext == nullptr || index >= cameraBookmarks.size()) return;
    cameraBookmarks[index] = {layoutContext->GetCamera().GetCenter(), layoutContext->GetCamera().GetZoom(), true};
}

bool WorkbenchView::RecallCameraBookmark(const std::size_t index) {
    if (layoutContext == nullptr || index >= cameraBookmarks.size() || !cameraBookmarks[index].valid) return false;
    layoutContext->GetCamera().SetCenter(cameraBookmarks[index].center);
    layoutContext->GetCamera().SetZoom(cameraBookmarks[index].zoom);
    return true;
}

bool WorkbenchView::IsPointerInsideViewport() const { return pointerInsideViewport; }

sf::Vector2f WorkbenchView::GetPointerWorldPosition() const { return pointerWorldPosition; }

void WorkbenchView::ShowProjectBrowser(const std::vector<std::filesystem::path> &recentProjects,
                                       const std::string &message) {
    workspaceVisible = false;
    pointerInsideViewport = false;

    projectBrowser->SetRecentProjects(recentProjects);

    projectBrowser->SetMessage(message);

    ApplyModeVisibility();
}

void WorkbenchView::ShowWorkspace() {
    workspaceVisible = true;

    projectBrowser->SetMessage("");

    ApplyModeVisibility();
}

bool WorkbenchView::IsWorkspaceVisible() const { return workspaceVisible; }

void WorkbenchView::ToggleAssetsPanel() {
    assetsVisible=!assetsVisible;
    if (assetsVisible && callbacks.discoverAssets) callbacks.discoverAssets();
    shell->SetAssetBrowserVisible(assetsVisible);
    toolbar->SetAssetsVisible(assetsVisible);
    ApplyWorkspaceLayout();
}

void WorkbenchView::ToggleStandardPanels() {
    if(layoutWindowSize.x<800){
        if(toolsPopup->IsOpen())toolsPopup->Dismiss();else toolsPopup->Open();
        panelsVisible=toolsPopup->IsOpen();
    }else if(AreToolsFloating()){
        floatingToolsWindow->SetVisible(false);shell->SetStandardToolsFloating(false);shell->SetBottomDockVisible(false);
        panelsVisible=false;
    }else{
        panelsVisible=!shell->IsBottomDockVisible();shell->SetBottomDockVisible(panelsVisible);
    }
    toolbar->SetPanelsVisible(panelsVisible);ApplyWorkspaceLayout();
}

void WorkbenchView::SetToolsFloating(const bool floating) {
    floatingWindowController.End();
    if(floating){
        if(toolsPopup&&toolsPopup->IsOpen())toolsPopup->Dismiss();
        auto bounds=shell->GetStandardToolsFloatingBounds();
        if(bounds.size.x<=0||bounds.size.y<=0)bounds={{80,176},{640,420}};
        shell->SetStandardToolsFloating(true,bounds);floatingToolsWindow->SetVisible(true);
        floatingToolsWindow->Arrange(bounds);panelsVisible=true;
    }else{
        floatingToolsWindow->SetVisible(false);shell->SetStandardToolsFloating(false);
        shell->SetBottomDockVisible(true);panelsVisible=true;
    }
    toolbar->SetPanelsVisible(panelsVisible);ApplyWorkspaceLayout();
}

bool WorkbenchView::AreToolsFloating() const {
    return shell&&shell->AreStandardToolsFloating()&&floatingToolsWindow&&floatingToolsWindow->IsVisible();
}

sf::FloatRect WorkbenchView::GetFloatingToolsBounds() const {
    return floatingToolsWindow?floatingToolsWindow->GetBounds():sf::FloatRect{};
}

void WorkbenchView::SetWorkspaceMode(const WorkbenchWorkspaceMode mode) {
    if (workspaceMode == mode) {
        return;
    }

    if (layoutContext != nullptr) {
        CameraState &current = workspaceMode == WorkbenchWorkspaceMode::Simulation ? simulationCamera : editorCamera;
        current = {layoutContext->GetCamera().GetCenter(), layoutContext->GetCamera().GetZoom(), true};
    }

    workspaceMode = mode;
    if(shell&&mode!=WorkbenchWorkspaceMode::Zen)shell->SelectViewportHost(mode==WorkbenchWorkspaceMode::Simulation);

    if (layoutContext != nullptr && mode != WorkbenchWorkspaceMode::Zen) {
        const CameraState &next = mode == WorkbenchWorkspaceMode::Simulation ? simulationCamera : editorCamera;
        if (next.valid) {
            layoutContext->GetCamera().SetCenter(next.center);
            layoutContext->GetCamera().SetZoom(next.zoom);
        }
    }

    pointerInsideViewport = false;

    ApplyModeVisibility();
    ApplyWorkspaceLayout();

    if (callbacks.workspaceModeChanged) {
        callbacks.workspaceModeChanged(workspaceMode);
    }
}

void WorkbenchView::CycleWorkspaceMode() {
    switch (workspaceMode) {
    case WorkbenchWorkspaceMode::Editor:
        SetWorkspaceMode(WorkbenchWorkspaceMode::Simulation);
        break;

    case WorkbenchWorkspaceMode::Simulation:
        SetWorkspaceMode(WorkbenchWorkspaceMode::Zen);
        break;

    case WorkbenchWorkspaceMode::Zen:
        SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
        break;
    }
}

WorkbenchWorkspaceMode WorkbenchView::GetWorkspaceMode() const { return workspaceMode; }

void WorkbenchView::ApplyModeVisibility() {

    const bool zenMode = workspaceMode == WorkbenchWorkspaceMode::Zen;

    if(shell) shell->SetVisible(workspaceVisible);
    if(contextMenu) contextMenu->Dismiss();
    if (toolsPopup && (!workspaceVisible || zenMode)) toolsPopup->Dismiss();
    if(floatingToolsWindow)floatingToolsWindow->SetVisible(workspaceVisible&&!zenMode&&shell->AreStandardToolsFloating());
    if (toolbar != nullptr) {
        toolbar->SetVisible(workspaceVisible && !zenMode);

        switch (workspaceMode) {
        case WorkbenchWorkspaceMode::Editor:
            toolbar->SetViewModeText("EDITOR");
            break;

        case WorkbenchWorkspaceMode::Simulation:
            toolbar->SetViewModeText("SIMULATION");
            break;

        case WorkbenchWorkspaceMode::Zen:
            toolbar->SetViewModeText("ZEN");
            break;
        }
    }

    if (hierarchyPanel != nullptr) {
        hierarchyPanel->SetVisible(workspaceVisible);
    }

    if (shell != nullptr) shell->SetAssetBrowserVisible(workspaceVisible && assetsVisible);

    if (projectBrowser != nullptr) {
        projectBrowser->SetVisible(!workspaceVisible);
    }

    if (zenExitButton != nullptr) {
        zenExitButton->SetVisible(workspaceVisible && zenMode);
    }

    diagnosticsOverlay->SetVisible(workspaceVisible && metricsVisible && !zenMode);
}

void WorkbenchView::ApplyWorkspaceLayout() {
    if (layoutContext == nullptr || toolbar == nullptr || hierarchyPanel == nullptr || inspectorPanel == nullptr) {
        return;
    }

    shell->Layout(layoutWindowSize,workspaceMode==WorkbenchWorkspaceMode::Zen);
    const auto bounds=shell->GetWorldBounds();
    viewportBorder.setPosition(bounds.position); viewportBorder.setSize(bounds.size);
    WorkbenchLayout::ApplyCamera(layoutWindowSize,bounds,*layoutContext);

}

void WorkbenchView::Refresh(const ProjectSession &project, const SimulationSession &simulation,
                            const bool rebuildHierarchy) {
    if (!loaded || !workspaceVisible) {
        return;
    }

    if (rebuildHierarchy) {
        RebuildHierarchy(project);
    }

    if (hierarchyPanel != nullptr) {
        hierarchyPanel->SetSelectedObject(project.GetSelectedObjectId());

        hierarchyPanel->SetAuthoringEnabled(simulation.CanAuthorScene());
    }

    const auto objectTypes=project.GetObjectTypes();
    availableObjectTypes.assign(objectTypes.begin(),objectTypes.end());
    canCreateObject=simulation.CanAuthorScene();
    sceneRenderObjects.assign(project.GetDocument().GetObjects().begin(), project.GetDocument().GetObjects().end());
    sceneConnections.assign(project.GetDocument().GetConnections().begin(), project.GetDocument().GetConnections().end());
    componentTypes.assign(project.GetComponentTypes().begin(), project.GetComponentTypes().end());
    selectionRenderObjects.clear();
    for (const SceneObjectId objectId : project.GetSelectedObjectIds()) {
        if (const SceneObjectData *object = project.GetDocument().FindObject(objectId)) {
            selectionRenderObjects.push_back(*object);
        }
    }
    attachmentPreviewCount = 0;
    for (const auto &object : selectionRenderObjects)
        for (const auto &component : object.components)
            if (const auto descriptor = std::ranges::find(componentTypes, component.typeId,
                                                          &SceneComponentTypeDescriptor::typeId);
                descriptor != componentTypes.end())
                attachmentPreviewCount += descriptor->attachments.size();
    connectionPreviewCount = sceneConnections.size();

    const ProjectRuntimeStatistics statistics = project.GetRuntime().GetStatistics();

    diagnosticsOverlay->SetPopulationRenderStats(statistics.candidateObjectCount, statistics.visibleObjectCount,
                                                statistics.vertexCount, statistics.geometryTimeMs,
                                                statistics.usingQuads);

    diagnosticsOverlay->SetPopulationSimulationStats(statistics.movementTimeMs, statistics.spatialGridTimeMs);

    RefreshInspector(project, simulation);

    if(assetBrowserPanel){
        std::vector<AssetBrowserPanel::AssignmentTarget> targets;
        const auto append=[&](const auto &type,const std::string &component){
            for(const auto &field:type.properties)if(field.editable&&field.kind==PropertyKind::AssetReference)
                targets.push_back({component,field.key,type.displayName+" / "+field.displayName,field.editorHint});
        };
        if(const auto *object=project.GetSelectedObject()){
            if(const auto *type=project.FindObjectType(object->typeId))append(*type,{});
            for(const auto &component:object->components)if(const auto *type=project.FindComponentType(component.typeId))append(*type,component.typeId);
        }
        assetBrowserPanel->SetAssignmentTargets(std::move(targets));
        assetBrowserPanel->SetEnvironmentContext(project.GetTilemapTargetName(),project.GetTilemapWorldScale());
    }
    if (assetBrowserPanel != nullptr) assetBrowserPanel->Refresh(project.GetAssetDatabase(),&project.GetTilemapEditor(),project.CanMakeSelectedTilemapUnique(),&project.GetVisualAssetEditor());

    if (shell != nullptr) {
        const auto telemetry = project.GetRuntime().GetLiveComponentProperties();
        shell->BottomTools().Refresh(statistics, simulation.GetController(),
                                     project.GetDocument().GetConnections(), telemetry,
                                     project.GetRuntime().GetExtensions(),project.GetRuntime().GetActions(),
                                     project.GetRuntime().GetSystems());
        if(floatingToolsPanel)floatingToolsPanel->Refresh(statistics,simulation.GetController(),
                                     project.GetDocument().GetConnections(),telemetry,
                                     project.GetRuntime().GetExtensions(),project.GetRuntime().GetActions(),
                                     project.GetRuntime().GetSystems());
        if(compactToolsPanel)compactToolsPanel->Refresh(statistics,simulation.GetController(),
                                     project.GetDocument().GetConnections(),telemetry,
                                     project.GetRuntime().GetExtensions(),project.GetRuntime().GetActions(),
                                     project.GetRuntime().GetSystems());
    }

    RefreshSimulationControls(simulation);

    RefreshToolbar(project, simulation);
}

void WorkbenchView::Update(const float deltaTime) {
    uiManager.Update(deltaTime);

}

void WorkbenchView::RenderWorldOverlay(RenderContext &context) const {
    if (!workspaceVisible) {
        return;
    }

    const sf::IntRect pixelViewport = pipeframe::backend::sfml::ViewportBounds(context);

    const float pixelWidth = static_cast<float>(std::max(1, pixelViewport.size.x));

    const float worldWidth = std::max(0.001f, context.GetCamera().GetSize().x);

    const float pixelsPerWorldUnit = pixelWidth / worldWidth;

    const float cursorRadius = 7.0f / pixelsPerWorldUnit;

    const float outlineThickness = 1.5f / pixelsPerWorldUnit;

    auto &target = pipeframe::backend::sfml::GetTarget(context);

    const auto findObject = [this](const SceneObjectId id) -> const SceneObjectData * {
        const auto found=std::ranges::find(sceneRenderObjects,id,&SceneObjectData::id);
        return found==sceneRenderObjects.end()?nullptr:&*found;
    };
    const auto findComponentType = [this](const std::string &id) -> const SceneComponentTypeDescriptor * {
        const auto found=std::ranges::find(componentTypes,id,&SceneComponentTypeDescriptor::typeId);
        return found==componentTypes.end()?nullptr:&*found;
    };
    const auto attachmentWorld = [&](const SceneConnectionEndpoint &endpoint) {
        const SceneObjectData *object=findObject(endpoint.objectId);
        if(!object)return sf::Vector2f{};
        sf::Vector2f local{};
        for(const auto &component:object->components){
            const auto *type=findComponentType(component.typeId);if(!type)continue;
            const auto attachment=std::ranges::find(type->attachments,endpoint.attachmentId,&SceneAttachmentDescriptor::id);
            if(attachment!=type->attachments.end()){
                local={attachment->localTransform.position.x*object->transform.scale.x,
                       attachment->localTransform.position.y*object->transform.scale.y};break;
            }
        }
        return pipeframe::backend::sfml::ToBackend(object->transform.position)+RotatePoint(local,object->transform.rotation);
    };

    for(const auto &connection:sceneConnections){
        const sf::Vector2f from=attachmentWorld(connection.from),to=attachmentWorld(connection.to);
        const sf::Color color=connection.kind==SceneConnectionKind::Mechanical?sf::Color{245,190,70,190}:
                              connection.kind==SceneConnectionKind::Power?sf::Color{235,85,95,190}:
                                                                              sf::Color{80,180,245,190};
        const sf::Vertex line[]{sf::Vertex{from,color},sf::Vertex{to,color}};
        target.draw(line,2,sf::PrimitiveType::Lines);
    }

    for (const auto &object : selectionRenderObjects) {
        const sf::Vector2f position = pipeframe::backend::sfml::ToBackend(object.transform.position);
        sf::CircleShape ring(16.0f / pixelsPerWorldUnit);
        ring.setOrigin({ring.getRadius(), ring.getRadius()});
        ring.setPosition(position);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color{74, 163, 255, 230});
        ring.setOutlineThickness(2.0f / pixelsPerWorldUnit);
        target.draw(ring);

        for(const auto &component:object.components){
            const auto *type=findComponentType(component.typeId);if(!type)continue;
            if(component.typeId=="pipeframe.collider2d"){
                const auto sizeValue=component.properties.find("size");
                if(sizeValue!=component.properties.end())if(const auto *size=std::get_if<Vector2f>(&sizeValue->second)){
                    sf::RectangleShape bounds({size->x*object.transform.scale.x,size->y*object.transform.scale.y});
                    bounds.setOrigin(bounds.getSize()*0.5f);bounds.setPosition(position);
                    bounds.setRotation(sf::degrees(object.transform.rotation));bounds.setFillColor(sf::Color::Transparent);
                    bounds.setOutlineColor(sf::Color{255,184,72,190});bounds.setOutlineThickness(1.0f/pixelsPerWorldUnit);
                    target.draw(bounds);
                }
            }
            for(const auto &property:type->properties){
                if(property.editorHint!="sensor-range")continue;
                const auto value=component.properties.find(property.key);if(value==component.properties.end())continue;
                const auto *range=std::get_if<double>(&value->second);if(!range||*range<=0)continue;
                sf::CircleShape preview(static_cast<float>(*range));preview.setOrigin({preview.getRadius(),preview.getRadius()});
                preview.setPosition(position);preview.setFillColor(sf::Color{70,165,255,18});
                preview.setOutlineColor(sf::Color{70,165,255,150});preview.setOutlineThickness(1.0f/pixelsPerWorldUnit);
                target.draw(preview);
            }
            for(const auto &attachment:type->attachments){
                sf::Vector2f local{attachment.localTransform.position.x*object.transform.scale.x,
                                   attachment.localTransform.position.y*object.transform.scale.y};
                sf::CircleShape point(5.0f/pixelsPerWorldUnit);point.setOrigin({point.getRadius(),point.getRadius()});
                point.setPosition(position+RotatePoint(local,object.transform.rotation));
                point.setFillColor(sf::Color{245,190,70,220});point.setOutlineColor(sf::Color::White);
                point.setOutlineThickness(1.0f/pixelsPerWorldUnit);target.draw(point);
            }
        }
    }

    if(!selectionRenderObjects.empty()){
        sf::Vector2f pivot{};for(const auto &object:selectionRenderObjects)pivot+=pipeframe::backend::sfml::ToBackend(object.transform.position);
        pivot/=static_cast<float>(selectionRenderObjects.size());
        const float axis=46.0f/pixelsPerWorldUnit;
        const float angle=localTransformSpace?selectionRenderObjects.back().transform.rotation*Pi/180.0f:0.0f;
        const sf::Vector2f xDirection{std::cos(angle),std::sin(angle)},yDirection{-std::sin(angle),std::cos(angle)};
        if(transformTool==TransformTool::Rotate){
            sf::CircleShape rotateRing(42.0f/pixelsPerWorldUnit,64);rotateRing.setOrigin({rotateRing.getRadius(),rotateRing.getRadius()});
            rotateRing.setPosition(pivot);rotateRing.setFillColor(sf::Color::Transparent);rotateRing.setOutlineColor(sf::Color{74,163,255,230});
            rotateRing.setOutlineThickness(2.0f/pixelsPerWorldUnit);target.draw(rotateRing);
        }else{
            const sf::Vertex gizmo[]{sf::Vertex{pivot,sf::Color{231,79,91}},sf::Vertex{pivot+xDirection*axis,sf::Color{231,79,91}},
                                     sf::Vertex{pivot,sf::Color{87,201,116}},sf::Vertex{pivot+yDirection*axis,sf::Color{87,201,116}}};
            target.draw(gizmo,4,sf::PrimitiveType::Lines);
            const auto drawHandle=[&](sf::Vector2f at,sf::Color color){sf::RectangleShape handle({9.0f/pixelsPerWorldUnit,9.0f/pixelsPerWorldUnit});
                handle.setOrigin(handle.getSize()*0.5f);handle.setPosition(at);handle.setFillColor(color);target.draw(handle);};
            drawHandle(pivot+xDirection*axis,sf::Color{231,79,91});drawHandle(pivot+yDirection*axis,sf::Color{87,201,116});
            drawHandle(pivot,sf::Color{230,230,235});
        }
        sf::Text coordinates(uiManager.GetDefaultFont(),"("+std::to_string(static_cast<int>(std::round(pivot.x)))+", "+
                            std::to_string(static_cast<int>(std::round(pivot.y)))+")",11);
        coordinates.setScale({1.0f/pixelsPerWorldUnit,1.0f/pixelsPerWorldUnit});coordinates.setPosition(pivot+sf::Vector2f{10,12}/pixelsPerWorldUnit);
        coordinates.setFillColor(sf::Color{205,214,228,220});target.draw(coordinates);
        if(selectionRenderObjects.size()>1){
            const auto first=pipeframe::backend::sfml::ToBackend(selectionRenderObjects.front().transform.position);
            const auto last=pipeframe::backend::sfml::ToBackend(selectionRenderObjects.back().transform.position);
            const float distance=std::hypot(last.x-first.x,last.y-first.y);
            const sf::Vertex measure[]{sf::Vertex{first,sf::Color{210,220,235,150}},sf::Vertex{last,sf::Color{210,220,235,150}}};target.draw(measure,2,sf::PrimitiveType::Lines);
            sf::Text label(uiManager.GetDefaultFont(),std::to_string(distance).substr(0,6),10);label.setScale({1.0f/pixelsPerWorldUnit,1.0f/pixelsPerWorldUnit});
            label.setPosition((first+last)*0.5f);label.setFillColor(sf::Color{210,220,235,220});target.draw(label);
        }
    }

    if (hoveredObjectId.has_value()) {
        const auto hover = std::ranges::find_if(selectionRenderObjects, [this](const SceneObjectData &object) {
            return object.id == *hoveredObjectId;
        });
        if (hover == selectionRenderObjects.end()) {
            // Runtime hit testing identifies the object; a small cursor halo provides feedback
            // even when the runtime does not publish editor geometry.
            sf::CircleShape halo(10.0f / pixelsPerWorldUnit);
            halo.setOrigin({halo.getRadius(), halo.getRadius()});
            halo.setPosition(pointerWorldPosition);
            halo.setFillColor(sf::Color::Transparent);
            halo.setOutlineColor(sf::Color{255, 196, 74, 210});
            halo.setOutlineThickness(1.5f / pixelsPerWorldUnit);
            target.draw(halo);
        }
    }

    if (selectionBox.has_value()) {
        sf::RectangleShape box(selectionBox->size);
        box.setPosition(selectionBox->position);
        box.setFillColor(sf::Color{74, 163, 255, 35});
        box.setOutlineColor(sf::Color{74, 163, 255, 220});
        box.setOutlineThickness(1.0f / pixelsPerWorldUnit);
        target.draw(box);
    }

    if(placementPreviewWorld){
        sf::CircleShape preview(18.0f/pixelsPerWorldUnit,24);preview.setOrigin({preview.getRadius(),preview.getRadius()});
        preview.setPosition(*placementPreviewWorld);preview.setFillColor(sf::Color{74,163,255,45});
        preview.setOutlineColor(sf::Color{74,163,255,230});preview.setOutlineThickness(2.0f/pixelsPerWorldUnit);target.draw(preview);
        const float arm=24.0f/pixelsPerWorldUnit;const sf::Vertex cross[]{
            sf::Vertex{*placementPreviewWorld-sf::Vector2f{arm,0},sf::Color{74,163,255,220}},sf::Vertex{*placementPreviewWorld+sf::Vector2f{arm,0},sf::Color{74,163,255,220}},
            sf::Vertex{*placementPreviewWorld-sf::Vector2f{0,arm},sf::Color{74,163,255,220}},sf::Vertex{*placementPreviewWorld+sf::Vector2f{0,arm},sf::Color{74,163,255,220}}};target.draw(cross,4,sf::PrimitiveType::Lines);
    }

    if (!pointerInsideViewport) return;

    sf::CircleShape cursor = worldCursor;

    cursor.setRadius(cursorRadius);

    cursor.setOrigin({
        cursorRadius,
        cursorRadius,
    });

    cursor.setOutlineThickness(outlineThickness);

    cursor.setPosition(pointerWorldPosition);

    target.draw(cursor);
}

void WorkbenchView::RenderWorldBackground(RenderContext &context) const {
    if (!workspaceVisible || !gridVisible || workspaceMode == WorkbenchWorkspaceMode::Zen) {
        return;
    }
    const auto viewport = pipeframe::backend::sfml::ViewportBounds(context);
    if (viewport.size.x <= 0 || viewport.size.y <= 0) {
        return;
    }
    const auto topLeft = pipeframe::backend::sfml::ScreenToWorld(context,viewport.position);
    const auto bottomRight = pipeframe::backend::sfml::ScreenToWorld(context,viewport.position + viewport.size);
    const float left = std::min(topLeft.x, bottomRight.x);
    const float right = std::max(topLeft.x, bottomRight.x);
    const float top = std::min(topLeft.y, bottomRight.y);
    const float bottom = std::max(topLeft.y, bottomRight.y);
    const float worldPerPixel = std::max((right - left) / static_cast<float>(viewport.size.x), 0.0001f);
    const float targetStep = worldPerPixel * 32.0f;
    const float decade = std::pow(10.0f, std::floor(std::log10(targetStep)));
    const float normalized = targetStep / decade;
    const float automaticStep = (normalized <= 1.0f ? 1.0f : normalized <= 2.0f ? 2.0f : normalized <= 5.0f ? 5.0f : 10.0f) * decade;
    float minorStep = gridStep > 0.0f ? gridStep : automaticStep;
    while ((right-left)/minorStep > 500.0f) minorStep *= 2.0f;
    displayedGridStep = minorStep;
    const float majorStep = minorStep * 5.0f;
    std::vector<sf::Vertex> minor;
    std::vector<sf::Vertex> major;
    const sf::Color minorColor{78, 88, 105, 42};
    const sf::Color majorColor{98, 111, 132, 82};
    const auto appendLine = [](std::vector<sf::Vertex> &vertices, sf::Vector2f a, sf::Vector2f b, sf::Color color) {
        vertices.emplace_back(a, color);
        vertices.emplace_back(b, color);
    };
    const auto firstX = gridOrigin.x + std::floor((left-gridOrigin.x) / minorStep) * minorStep;
    const auto firstY = gridOrigin.y + std::floor((top-gridOrigin.y) / minorStep) * minorStep;
    for (float x = firstX; x <= right + minorStep * 0.5f; x += minorStep) {
        const bool isMajor = std::abs((x-gridOrigin.x) / majorStep - std::round((x-gridOrigin.x) / majorStep)) < 0.001f;
        appendLine(isMajor ? major : minor, {x, top}, {x, bottom}, isMajor ? majorColor : minorColor);
    }
    for (float y = firstY; y <= bottom + minorStep * 0.5f; y += minorStep) {
        const bool isMajor = std::abs((y-gridOrigin.y) / majorStep - std::round((y-gridOrigin.y) / majorStep)) < 0.001f;
        appendLine(isMajor ? major : minor, {left, y}, {right, y}, isMajor ? majorColor : minorColor);
    }
    auto &target = pipeframe::backend::sfml::GetTarget(context);
    if (!minor.empty()) target.draw(minor.data(), minor.size(), sf::PrimitiveType::Lines);
    if (!major.empty()) target.draw(major.data(), major.size(), sf::PrimitiveType::Lines);
    const std::array axes{
        sf::Vertex{{left, gridOrigin.y}, sf::Color{102, 190, 125, 170}},
        sf::Vertex{{right, gridOrigin.y}, sf::Color{102, 190, 125, 170}},
        sf::Vertex{{gridOrigin.x, top}, sf::Color{224, 92, 105, 170}},
        sf::Vertex{{gridOrigin.x, bottom}, sf::Color{224, 92, 105, 170}},
    };
    target.draw(axes.data(), axes.size(), sf::PrimitiveType::Lines);

    const float labelScale=worldPerPixel;
    float labelStep=majorStep;
    while(labelStep/worldPerPixel<64.0f)labelStep*=2.0f;
    const float rulerTop=top+18.0f*worldPerPixel;
    const float rulerLeft=left+34.0f*worldPerPixel;
    const sf::Color rulerColor{155,169,191,210};
    const sf::Vertex rulers[]{sf::Vertex{{left,rulerTop},rulerColor},sf::Vertex{{right,rulerTop},rulerColor},
                              sf::Vertex{{rulerLeft,top},rulerColor},sf::Vertex{{rulerLeft,bottom},rulerColor}};
    target.draw(rulers,4,sf::PrimitiveType::Lines);
    std::size_t labels=0;
    for(float x=gridOrigin.x+std::floor((left-gridOrigin.x)/labelStep)*labelStep;
        x<=right&&labels<24;x+=labelStep,++labels){
        sf::Text label(uiManager.GetDefaultFont(),std::to_string(static_cast<int>(std::round(x-gridOrigin.x))),10);
        label.setScale({labelScale,labelScale});label.setPosition({x+2.0f*worldPerPixel,top+2.0f*worldPerPixel});
        label.setFillColor(rulerColor);target.draw(label);
        const sf::Vertex tick[]{sf::Vertex{{x,rulerTop-4.0f*worldPerPixel},rulerColor},sf::Vertex{{x,rulerTop+4.0f*worldPerPixel},rulerColor}};
        target.draw(tick,2,sf::PrimitiveType::Lines);
    }
    labels=0;
    for(float y=gridOrigin.y+std::floor((top-gridOrigin.y)/labelStep)*labelStep;
        y<=bottom&&labels<24;y+=labelStep,++labels){
        sf::Text label(uiManager.GetDefaultFont(),std::to_string(static_cast<int>(std::round(y-gridOrigin.y))),10);
        label.setScale({labelScale,labelScale});label.setPosition({left+2.0f*worldPerPixel,y+2.0f*worldPerPixel});
        label.setFillColor(rulerColor);target.draw(label);
        const sf::Vertex tick[]{sf::Vertex{{rulerLeft-4.0f*worldPerPixel,y},rulerColor},sf::Vertex{{rulerLeft+4.0f*worldPerPixel,y},rulerColor}};
        target.draw(tick,2,sf::PrimitiveType::Lines);
    }
}

void WorkbenchView::RenderScreen(RenderContext &context, const SimulationSession &simulation) {
    context.BeginScreen();

    if (workspaceVisible && workspaceMode != WorkbenchWorkspaceMode::Zen) {
        pipeframe::backend::sfml::GetTarget(context).draw(viewportBorder);
    }

    if(workspaceVisible) diagnosticsOverlay->Refresh(simulation.GetController(),context.GetCamera(),pointerWorldPosition);
    uiManager.Render(pipeframe::backend::sfml::GetTarget(context));
}

void WorkbenchView::CreateInterface() {
    const sf::Font &font = uiManager.GetDefaultFont();

    shell=&uiManager.CreateRoot<WorkbenchLayout>(font);
    toolbar=&shell->Toolbar(); hierarchyPanel=&shell->Hierarchy(); inspectorPanel=&shell->Inspector();
    assetBrowserPanel=&shell->Assets();
    zenExitButton=&shell->ExitZen();
    diagnosticsOverlay=&shell->Viewport().CreateChild<DiagnosticsOverlay>(font);
    shell->Viewport().SetChildAlignment(*diagnosticsOverlay,{HorizontalAlignment::Start,VerticalAlignment::Start});
    diagnosticsOverlay->SetVisible(false);
    projectBrowser=&uiManager.CreateRoot<pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::ProjectBrowser>>(font);
    floatingToolsWindow=&uiManager.CreateRoot<FloatingWorkspaceWindow>(font);
    floatingToolsPanel=&floatingToolsWindow->Tools();
    floatingToolsWindow->SetVisible(false);
    floatingToolsWindow->SetOnDock([this] { SetToolsFloating(false); });
    floatingToolsWindow->SetOnClose([this] {
        floatingToolsWindow->SetVisible(false);shell->SetStandardToolsFloating(false);shell->SetBottomDockVisible(false);
        panelsVisible=false;toolbar->SetPanelsVisible(false);ApplyWorkspaceLayout();
    });
    contextMenu=&uiManager.CreateRoot<PopupLayer>();
    contextActions=&contextMenu->GetContent().CreateChild<pipeframe::ui::ViewBuilderPanel>(font,[this] {
        using namespace pipeframe::ui;
        if(choosingObjectType && choosingModule) {
            std::vector<View> kinds;
            for(const std::string kind:{"Entity","Component","Behaviour","Brush"})
                kinds.push_back(views::Button("kind:"+kind,kind,[this,kind]{moduleKind=kind;contextActions->InvalidateView();}).Selected(moduleKind==kind));
            return views::Column("create-module",{
                views::Text("title","CREATE PROJECT SOURCE").FitHeight(),
                views::Row("kinds",std::move(kinds)),
                views::TextField("module-name","Class name",moduleName,[this](const std::string &name){moduleName=name;}),
                views::Text("result",moduleMessage).FitHeight(),
                views::Button("generate","GENERATE",[this] {
                    moduleMessage=callbacks.generateModule?callbacks.generateModule(moduleKind,moduleName):"No active project.";
                    contextActions->InvalidateView();
                }),
                views::Button("back","BACK",[this]{choosingModule=false;contextActions->InvalidateView();})
            }).Padding(12).FillHeight();
        }
        if(choosingObjectType) {
            std::vector<View> choices;
            for(const auto &type:availableObjectTypes) {
                if(!objectTypeSearch.empty() && type.displayName.find(objectTypeSearch)==std::string::npos &&
                   type.typeId.find(objectTypeSearch)==std::string::npos)continue;
                choices.push_back(views::Button("create-type:"+type.typeId,type.displayName,[this,id=type.typeId] {
                    contextMenu->Dismiss();
                    if(callbacks.createObjectOfType)callbacks.createObjectOfType(id,objectCreationPosition);
                }).Leading());
            }
            if(choices.empty())choices.push_back(views::Text("empty-types","No matching object types").FitHeight());
            return views::Column("create-object",{
                views::Text("title","CREATE OBJECT").FitHeight(),
                views::TextField("type-search","Search object types",objectTypeSearch,[this](const std::string &value) {
                    objectTypeSearch=value;contextActions->InvalidateView();
                }),
                views::Scroll("types",views::Column("choices",std::move(choices))).Expanded(),
                views::Button("new-source","NEW ENTITY / COMPONENT / BEHAVIOUR",[this]{choosingModule=true;moduleName.clear();moduleMessage.clear();contextActions->InvalidateView();}).Enabled(bool(callbacks.generateModule)),
                views::Button("cancel-create","CANCEL",[this]{contextMenu->Dismiss();})
            }).Padding(12).FillHeight();
        }
        return pipeframe::ui::views::Scroll("context-scroll",pipeframe::ui::views::Column("context",contextItems).Padding(8)).FillHeight();
    });
    const auto action=[&](const char *name,std::function<void()> callback) {
        contextItems.push_back(pipeframe::ui::views::Button(name,name,[this,callback] {
            contextMenu->Dismiss(); if(callback) callback();
        }).Height(36));
    };
    action("ADD OBJECT",[this] { if(callbacks.createObjectOfType)ShowCreateObjectPicker();else if(callbacks.addObject)callbacks.addObject(); });
    action("PLACE OBJECT HERE",[this] {
        if(callbacks.createObjectOfType)ShowCreateObjectPicker(Vector2f{contextActionWorldPosition.x,contextActionWorldPosition.y});
        else if(callbacks.placeObject) callbacks.placeObject({contextActionWorldPosition.x,contextActionWorldPosition.y});
    });
    action("GRID ORIGIN HERE",[this] { SetGridOrigin({contextActionWorldPosition.x,contextActionWorldPosition.y}); });
    action("RESET GRID ORIGIN",[this] { SetGridOrigin({}); });
    action("DELETE SELECTED",[this] { if(callbacks.deleteObject) callbacks.deleteObject(); });
    action("CREATE PREFAB",[this] { if(callbacks.createPrefab) callbacks.createPrefab(); });
    action("APPLY PREFAB",[this] { if(callbacks.applyPrefab) callbacks.applyPrefab(); });
    action("REVERT PREFAB",[this] { if(callbacks.revertPrefab) callbacks.revertPrefab(); });
    action("UNPACK PREFAB",[this] { if(callbacks.unpackPrefab) callbacks.unpackPrefab(); });
    action("FLOAT / DOCK TOOLS",[this] { SetToolsFloating(!AreToolsFloating()); });
    action("RESET LAYOUT",[this] {
        if (toolsPopup) toolsPopup->Dismiss();
        if (floatingToolsWindow) floatingToolsWindow->SetVisible(false);
        shell->ResetWorkspaceLayout();
        panelsVisible=false;
        toolbar->SetPanelsVisible(false);
        ApplyWorkspaceLayout();
    });
    action("SAVE",[this] { if(callbacks.save) callbacks.save(); });
    action("CLOSE",[] {});
    toolsPopup=&uiManager.CreateRoot<PopupLayer>();
    toolsPopup->SetDismissOnBackgroundClick(true);
    compactToolsPanel=&toolsPopup->GetContent().CreateChild<pipeframe::backend::sfml::HostedViewPanel<pipeframe::editor::WorkspaceToolsPanel>>(font);
    compactToolsPanel->SetVisible(true);
    toolsPopup->SetOnDismiss([this] { panelsVisible=false; toolbar->SetPanelsVisible(false); });
    ApplyModeVisibility();
    BindCallbacks();
}

void WorkbenchView::BindCallbacks() {
    auto &zenTransport=shell->ZenTransport();
    zenTransport.SetOnPlayPause([this] { if(callbacks.toggleSimulation) callbacks.toggleSimulation(); });
    zenTransport.SetOnSingleStep([this] { if(callbacks.singleStep) callbacks.singleStep(); });
    zenTransport.SetOnReset([this] { if(callbacks.resetSimulation) callbacks.resetSimulation(); });
    zenTransport.SetOnSpeedSelected([this](auto speed) { if(callbacks.setSimulationSpeed) callbacks.setSimulationSpeed(speed); });

    shell->SetOnExitZen([this]() {
        SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
    });

    toolbar->SetOnNewProject([this]() {
        if (callbacks.newProject) {
            callbacks.newProject();
        }
    });

    toolbar->SetOnOpenProject([this]() {
        if (callbacks.openProject) {
            callbacks.openProject();
        }
    });

    toolbar->SetOnSave([this]() {
        if (callbacks.save) {
            callbacks.save();
        }
    });

    toolbar->SetOnSaveAs([this]() {
        if (callbacks.saveAs) {
            callbacks.saveAs();
        }
    });

    toolbar->SetOnUndo([this]() {
        if (callbacks.undo) {
            callbacks.undo();
        }
    });

    toolbar->SetOnRedo([this]() {
        if (callbacks.redo) {
            callbacks.redo();
        }
    });

    toolbar->SetOnMetrics([this]() {
        metricsVisible = !metricsVisible;

        diagnosticsOverlay->SetVisible(workspaceVisible && metricsVisible);

        toolbar->SetMetricsVisible(metricsVisible);
    });

    toolbar->SetOnAssets([this]() { ToggleAssetsPanel(); });

    toolbar->SetOnPanels([this]() { ToggleStandardPanels(); });

    toolbar->SetOnBuild([this](){if(callbacks.buildRuntime)callbacks.buildRuntime();});
    toolbar->SetOnReload([this]() { if(callbacks.reloadRuntime)callbacks.reloadRuntime(); });

    assetBrowserPanel->SetOnAssignProperty(callbacks.assignAssetProperty);
    assetBrowserPanel->SetOnAssign([this](const std::string &assetId) {
        if(callbacks.assignAsset)callbacks.assignAsset(assetId);
    });
    assetBrowserPanel->SetOnMakeUnique([this]{if(callbacks.makeMapUnique)callbacks.makeMapUnique();});
    assetBrowserPanel->SetVisualActions(callbacks.visualAssets);
    assetBrowserPanel->SetEnvironmentActions(callbacks.environmentAssets);
    assetBrowserPanel->SetMapActions([this]{if(callbacks.createMap)callbacks.createMap();},
        [this](const std::string &id){if(callbacks.editMap)callbacks.editMap(id);},
        [this]{if(callbacks.stopMap)callbacks.stopMap();},
        [this](std::size_t layer,pipeframe::TileId tile,int shape){if(callbacks.paintShape)callbacks.paintShape(layer,tile,shape);});
    assetBrowserPanel->SetOnImport([this] { if(callbacks.importAsset)callbacks.importAsset(); });
    assetBrowserPanel->SetOnReimport([this](const std::string &assetId) {
        if(callbacks.reimportAsset)callbacks.reimportAsset(assetId);
    });
    assetBrowserPanel->SetOnCancel([this](const std::uint64_t operationId) {
        if(callbacks.cancelAssetOperation)callbacks.cancelAssetOperation(operationId);
    });

    toolbar->SetOnPhysicsDebug([this] { worldDebugOptions.physics=!worldDebugOptions.physics; toolbar->SetPhysicsDebugVisible(worldDebugOptions.physics); });
    toolbar->SetOnMeshDebug([this] { worldDebugOptions.mesh=!worldDebugOptions.mesh; toolbar->SetMeshDebugVisible(worldDebugOptions.mesh); });
    toolbar->SetOnGrid([this]() {
        gridVisible = !gridVisible;
        toolbar->SetGridVisible(gridVisible);
    });

    toolbar->SetOnGridStep([this]() { CycleGridStep(); });
    toolbar->SetOnGridOrigin([this]() { SetGridOrigin({pointerWorldPosition.x,pointerWorldPosition.y}); });

    toolbar->SetOnSnap([this]() {
        positionSnapEnabled = !positionSnapEnabled;
        toolbar->SetSnapEnabled(positionSnapEnabled, positionSnapStep);
    });

    toolbar->SetOnRotationSnap([this]() {
        rotationSnapEnabled = !rotationSnapEnabled;
        toolbar->SetRotationSnapEnabled(rotationSnapEnabled, rotationSnapStep);
    });

    toolbar->SetOnScaleSnap([this]() {
        scaleSnapEnabled = !scaleSnapEnabled;
        toolbar->SetScaleSnapEnabled(scaleSnapEnabled, scaleSnapStep);
    });

    toolbar->SetOnTool([this]() {
        switch (transformTool) {
        case TransformTool::Move: SetTransformTool(TransformTool::Rotate); break;
        case TransformTool::Rotate: SetTransformTool(TransformTool::Scale); break;
        case TransformTool::Scale: SetTransformTool(TransformTool::Move); break;
        }
    });

    toolbar->SetOnTransformSpace([this]() {
        localTransformSpace = !localTransformSpace;
        toolbar->SetTransformSpaceText(localTransformSpace ? "LOCAL" : "WORLD");
    });

    toolbar->SetOnPivot([this]() { CyclePivotMode(); });

    toolbar->SetOnFrame([this]() { FrameSelectionOrScene(); });

    toolbar->SetOnViewMode([this]() { CycleWorkspaceMode(); });

    hierarchyPanel->SetOnAdd([this]() {
        if(callbacks.createObjectOfType) { ShowCreateObjectPicker();return; }
        if (callbacks.addObject) {
            callbacks.addObject();
        }
    });

    hierarchyPanel->SetOnDelete([this]() {
        if (callbacks.deleteObject) {
            callbacks.deleteObject();
        }
    });

    hierarchyPanel->SetOnSelectionChanged([this](const SceneObjectId objectId) {
        if (callbacks.selectionChanged) {
            callbacks.selectionChanged(objectId);
        }
    });

    inspectorPanel->SetOnPlayPause([this]() {
        if (callbacks.toggleSimulation) {
            callbacks.toggleSimulation();
        }
    });

    inspectorPanel->SetOnSingleStep([this]() {
        if (callbacks.singleStep) {
            callbacks.singleStep();
        }
    });

    inspectorPanel->SetOnReset([this]() {
        if (callbacks.resetSimulation) {
            callbacks.resetSimulation();
        }
    });

    inspectorPanel->SetOnSpeedSelected([this](const SimulationSpeed speed) {
        if (callbacks.setSimulationSpeed) {
            callbacks.setSimulationSpeed(speed);
        }
    });

    inspectorPanel->SetOnTransformCommitted([this](const SceneTransform &transform) {
        if (callbacks.transformCommitted) {
            callbacks.transformCommitted(transform);
        }
    });

    inspectorPanel->SetOnPropertyCommitted([this](const std::string &key, const PropertyValue &value) {
        if (callbacks.propertyCommitted) {
            callbacks.propertyCommitted(key, value);
        }
    });

    inspectorPanel->SetOnComponentAttachment([this](const std::string &id,bool add) {if(callbacks.componentAttachment)callbacks.componentAttachment(id,add);});
    inspectorPanel->SetOnComponentPropertyCommitted(
        [this](const std::string &componentTypeId, const std::string &key, const PropertyValue &value) {
            if (callbacks.componentPropertyCommitted) {
                callbacks.componentPropertyCommitted(componentTypeId, key, value);
            }
        });

    projectBrowser->SetOnNewProject([this]() {
        if (callbacks.newProject) {
            callbacks.newProject();
        }
    });

    projectBrowser->SetOnOpenProject([this]() {
        if (callbacks.openProject) {
            callbacks.openProject();
        }
    });

    projectBrowser->SetOnOpenRecent([this](const std::filesystem::path &path) {
        if (callbacks.openRecentProject) {
            callbacks.openRecentProject(path);
        }
    });
}

void WorkbenchView::RebuildHierarchy(const ProjectSession &project) {
    std::vector<HierarchyPanel::Item> items;
    items.reserve(project.GetDocument().GetObjects().size());

    const auto &document = project.GetDocument();
    std::unordered_set<SceneObjectId> emitted;
    const auto append = [&](const auto &self, const SceneObjectId parentId, const std::size_t depth) -> void {
        for (const SceneObjectData *object : document.GetChildren(parentId)) {
            if (!emitted.insert(object->id).second) continue;
            std::string label(depth * 2, ' ');
            label += depth == 0 ? "" : "└ ";
            label += object->name;
            if (!object->visible) label += "  [HIDDEN]";
            if (object->locked) label += "  [LOCKED]";
            items.push_back({object->id, std::move(label)});
            self(self, object->id, depth + 1);
        }
    };
    append(append, 0, 0);
    for (const auto &object : document.GetObjects()) {
        if (!emitted.contains(object.id)) {
            std::string label = "! " + object.name;
            items.push_back({object.id, std::move(label)});
        }
    }

    hierarchyPanel->SetItems(items);

    hierarchyPanel->SetSelectedObject(project.GetSelectedObjectId());
}

void WorkbenchView::RefreshInspector(const ProjectSession &project, const SimulationSession &simulation) {
    const SceneObjectData *object = project.GetSelectedObject();

    if (object == nullptr) {
        inspectorPanel->ClearSelection();
    } else {
        inspectorPanel->SetLiveComponentProperties(project.GetRuntime().GetLiveComponentProperties());
        std::vector<const SceneObjectData *> objects{object};
        for (const SceneObjectId id : project.GetSelectedObjectIds())
            if (id != object->id)
                if (const auto *selected = project.GetDocument().FindObject(id)) objects.push_back(selected);
        std::vector<SceneObjectData> liveObjects;
        for (const auto *selected : objects)
            if (auto components=project.GetRuntime().InspectObjectComponents(selected->id)) {
                auto live=*selected; live.components=std::move(*components); liveObjects.push_back(std::move(live));
            }
        inspectorPanel->SetLiveObjects(std::move(liveObjects));
        inspectorPanel->SetSelection(objects, project.FindObjectType(object->typeId), project.GetComponentTypes(),
                                     &project.GetRuntime().GetExtensions());
    }

    inspectorPanel->SetAuthoringEnabled(simulation.CanAuthorScene());
}

void WorkbenchView::RefreshToolbar(const ProjectSession &project, const SimulationSession &simulation) {
    const ProjectManifest *manifest = project.GetManifest();

    toolbar->SetProjectName(manifest != nullptr ? manifest->name : "NO PROJECT");

    const bool canAuthor = simulation.CanAuthorScene();

    toolbar->SetAuthoringEnabled(canAuthor);

    toolbar->SetHistoryEnabled(canAuthor && project.CanUndo(), canAuthor && project.CanRedo());

    std::ostringstream status;

    if (!project.GetRuntime().HasRuntime()) {
        status << "NO RUNTIME  |  ";
    }

    status << ((project.GetDocument().IsDirty() || project.GetTilemapEditor().IsDirty() || project.GetVisualAssetEditor().IsDirty()) ? "UNSAVED" : "SAVED") << "  |  OBJECTS "
           << project.GetDocument().GetObjects().size();

    toolbar->SetStatusText(status.str());
}

void WorkbenchView::RefreshSimulationControls(const SimulationSession &simulation) {
    inspectorPanel->SetSimulationState(simulation.IsPlaying(), simulation.IsPreviewActive(), simulation.GetSpeed());
    shell->ZenTransport().SetSimulationState(simulation.IsPlaying(),simulation.IsPreviewActive(),simulation.GetSpeed());
}

} // namespace pipeframe::editor

namespace pipeframe::editor {
void WorkbenchView::SetBuildProgress(bool running,const std::string &output){
    if(toolbar)toolbar->SetBuildRunning(running);
    if(shell)shell->BottomTools().SetBuildOutput(output);
    if(floatingToolsPanel)floatingToolsPanel->SetBuildOutput(output);
    if(compactToolsPanel)compactToolsPanel->SetBuildOutput(output);
}
}

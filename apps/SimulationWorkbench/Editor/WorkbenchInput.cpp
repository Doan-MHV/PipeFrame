#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include "WorkbenchInput.h"

#include <cmath>
#include <algorithm>

#include <utility>

#include "ProjectSession.h"
#include "../Backend/SFML/WorkbenchView.h"

#include "../Runtime/SimulationSession.h"


#include <PipeFrame/Backend/SFML/Input/Input.h>
#include <PipeFrame/Input/Key.h>
#include <PipeFrame/Render/RenderContext.h>

namespace pipeframe::editor {
namespace {
void DispatchCamera(CameraController2D &camera,const InputEvent &event,RenderContext &context){
    camera.HandleEvent(event,context);
}
bool DispatchRuntimeUI(ProjectRuntimeHost &runtime,const InputEvent &event,RenderContext &context){
    return runtime.HandleUIEvent(event,context);
}
void DispatchRuntimeEvent(ProjectRuntimeHost &runtime,const InputEvent &event,RenderContext &context){
    runtime.HandleEvent(event,context);
}
// Temporary native-view bridge; runtime and camera routing are already neutral.
bool DispatchView(WorkbenchView &view,const InputEvent &event){
    const auto native=pipeframe::backend::sfml::ToBackend(event);
    return native && view.HandleEvent(*native);
}

}


WorkbenchInput::WorkbenchInput(WorkbenchView &view, ProjectSession &project, SimulationSession &simulation)
    : view(view), project(project), simulation(simulation) {}

void WorkbenchInput::SetOnStateChanged(StateChangedCallback callback) { onStateChanged = std::move(callback); }

void WorkbenchInput::HandleEvent(const InputEvent &event, RenderContext &context) {

    if(simulation.IsPlaying())project.GetTilemapEditor().SetEnabled(false);
    if(project.GetTilemapEditor().HasPointerCapture() && project.HandleTilemapEvent(event,context)){
        if(onStateChanged)onStateChanged(false); return;
    }

    if ((event.type == InputEventType::FocusChanged && event.GetIf<FocusInput>() && !event.GetIf<FocusInput>()->focused) || (event.type == InputEventType::PointerPresenceChanged && event.GetIf<PointerPresenceInput>() && !event.GetIf<PointerPresenceInput>()->inside)) {

        CancelDrag();
        DispatchView(view,event);
        DispatchRuntimeUI(project.GetRuntime(),event,context);
        DispatchRuntimeEvent(project.GetRuntime(),event,context);
        cameraController.CancelInteraction();

        view.SetPointerState(false, view.GetPointerWorldPosition());

        return;
    }

    if((event.type == InputEventType::KeyReleased)) DispatchCamera(cameraController,event,context);

    if (view.HasBlockingOverlay()) {
        cameraController.CancelInteraction();
        CancelDrag();
        DispatchRuntimeUI(project.GetRuntime(),InputEvent{InputEventType::FocusChanged,FocusInput{false}},context);
        DispatchRuntimeEvent(project.GetRuntime(),InputEvent{InputEventType::FocusChanged,FocusInput{false}},context);
    } else if (project.GetRuntime().HasWorldPointerCapture() &&
               ((event.type == InputEventType::PointerMoved) || (event.type == InputEventType::PointerReleased))) {
        DispatchRuntimeEvent(project.GetRuntime(),event,context);
        ApplyRuntimeEdits();
        return;
    }
    if (!view.HasBlockingOverlay() && project.GetRuntime().HasUIFocus() &&
        ((event.type == InputEventType::KeyPressed) || (event.type == InputEventType::KeyReleased) || (event.type == InputEventType::TextEntered)) &&
        DispatchRuntimeUI(project.GetRuntime(),event,context)) {
        ApplyRuntimeEdits(); return;
    }

    if (const auto *moved = (event.type == InputEventType::PointerMoved ? event.GetIf<PointerMoveInput>() : nullptr)) {

        if (dragging) {
            HandlePointerMove(event, moved->position, context);

            return;
        }
    }

    if (const auto *released = (event.type == InputEventType::PointerReleased ? event.GetIf<PointerInput>() : nullptr)) {

        if (dragging && released->button == PointerButton::Left) {

            HandlePointerRelease(*released, context);

            return;
        }
    }

    const bool uiConsumed = DispatchView(view,event);

    if (uiConsumed) {
        if((event.type == InputEventType::PointerPressed))
            DispatchRuntimeUI(project.GetRuntime(),InputEvent{InputEventType::FocusChanged,FocusInput{false}},context);
        if ((event.type == InputEventType::PointerMoved)) {
            project.GetTilemapEditor().ClearHover();
            view.SetPointerState(false, view.GetPointerWorldPosition());
        }

        if ((event.type == InputEventType::PointerReleased)) {

            DispatchCamera(cameraController,event, context);
        }

        return;
    }

    if (!simulation.IsPlaying() && project.HandleTilemapEvent(event,context)) {
        if(onStateChanged)onStateChanged(false);
        return;
    }

    if (DispatchRuntimeUI(project.GetRuntime(),event,context)) {
        ApplyRuntimeEdits();
        return;
    }

    if (const auto *pressed = (event.type == InputEventType::KeyPressed ? event.GetIf<KeyInput>() : nullptr)) {

        if (HandleKeyboard(*pressed, context)) {
            return;
        }

        if (view.HasKeyboardFocus()) {
            return;
        }

        DispatchCamera(cameraController,event,context);
        DispatchRuntimeEvent(project.GetRuntime(),event, context);
        ApplyRuntimeEdits();
        return;
    }

    if (const auto *moved = (event.type == InputEventType::PointerMoved ? event.GetIf<PointerMoveInput>() : nullptr)) {

        HandlePointerMove(event, moved->position, context);

        DispatchCamera(cameraController,event, context);

        return;
    }

    if (const auto *pressed = (event.type == InputEventType::PointerPressed ? event.GetIf<PointerInput>() : nullptr)) {

        HandlePointerPress(*pressed, context);

        return;
    }

    if (const auto *released = (event.type == InputEventType::PointerReleased ? event.GetIf<PointerInput>() : nullptr)) {

        HandlePointerRelease(*released, context);

        return;
    }

    if (const auto *wheel = (event.type == InputEventType::WheelScrolled ? event.GetIf<WheelInput>() : nullptr)) {

        const bool insideViewport = context.IsInsideWorldViewport({wheel->position.x,wheel->position.y});

        if (!insideViewport) {
            return;
        }

        DispatchCamera(cameraController,event, context);

        const Vector2f worldPosition = context.MapPixelToWorld(wheel->position);

        view.SetPointerState(true, pipeframe::backend::sfml::ToBackend(worldPosition));

        DispatchRuntimeEvent(project.GetRuntime(),event, context);
    }
}

void WorkbenchInput::CancelDrag() {
    if (!dragging) {
        return;
    }

    if (dragKind == DragKind::Transform) {
        project.CancelContinuousEdit();
    }

    view.SetSelectionBox(std::nullopt);
    dragStartTransforms.clear();
    dragKind = DragKind::None;
    activeHandle = TransformHandle::None;

    dragging = false;

    NotifyStateChanged(false);
}

bool WorkbenchInput::HandleKeyboard(const KeyInput &event, RenderContext &context) {
    const bool primaryModifier = event.system || event.control;

    if (!primaryModifier && !event.alt && event.key == InputKey::F6) {
        view.CycleWorkspaceMode();
        NotifyStateChanged(false);
        return true;
    }
    if (!primaryModifier && !event.alt && event.key == InputKey::F7) {
        view.ToggleAssetsPanel();
        return true;
    }
    if (!primaryModifier && !event.alt && event.key == InputKey::F8) {
        view.ToggleStandardPanels();
        return true;
    }

    if (view.HasKeyboardFocus()) {
        return false;
    }

    const auto bookmarkIndex = [&]() -> std::optional<std::size_t> {
        switch (event.key) {
        case InputKey::Num1: return 0;
        case InputKey::Num2: return 1;
        case InputKey::Num3: return 2;
        case InputKey::Num4: return 3;
        default: return std::nullopt;
        }
    }();
    if (bookmarkIndex.has_value()) {
        if (primaryModifier) view.SaveCameraBookmark(*bookmarkIndex);
        else view.RecallCameraBookmark(*bookmarkIndex);
        (void)context;
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::W) {
        view.SetTransformTool(TransformTool::Move);
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::E) {
        view.SetTransformTool(TransformTool::Rotate);
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::R) {
        view.SetTransformTool(TransformTool::Scale);
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::F) {
        std::vector<SceneObjectData> selected;
        for (const SceneObjectId id : project.GetSelectedObjectIds()) {
            if (const SceneObjectData *object = project.GetDocument().FindObject(id)) selected.push_back(*object);
        }
        if (selected.empty()) view.FrameObjects(project.GetDocument().GetObjects());
        else view.FrameObjects(selected);
        return true;
    }

    if (primaryModifier && event.key == InputKey::Z) {

        if (!simulation.CanAuthorScene()) {
            return true;
        }

        if (event.shift) {
            project.Redo();
        } else {
            project.Undo();
        }

        NotifyStateChanged(true);
        return true;
    }

    if (primaryModifier && event.key == InputKey::S) {

        if (simulation.CanAuthorScene()) {
            project.Save();
            NotifyStateChanged(false);
        }

        return true;
    }

    if (!primaryModifier && !event.alt &&
        (event.key == InputKey::Delete || event.key == InputKey::Backspace)) {

        if (simulation.CanAuthorScene() && project.DeleteSelectedObject()) {

            NotifyStateChanged(true);
        }

        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::Escape) {

        if (dragging) {
            CancelDrag();
        } else {
            project.SetSelectedObject(std::nullopt);

            NotifyStateChanged(false);
        }

        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::P) {

        simulation.Toggle(project.GetDocument().GetObjects());

        NotifyStateChanged(false);
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::S) {
        simulation.SetSpeed(simulation.IsFullSpeed()
                                ? SimulationSpeed::Realtime
                                : SimulationSpeed::Maximum);
        NotifyStateChanged(false);
        return true;
    }

    if (!primaryModifier && !event.alt && event.key == InputKey::Period) {

        simulation.RequestSingleStep(project.GetDocument().GetObjects());

        NotifyStateChanged(false);
        return true;
    }

    return false;
}

void WorkbenchInput::HandlePointerMove(const InputEvent &event, const Vector2i pixelPosition,
                                       RenderContext &context) {

    const bool insideViewport = context.IsInsideWorldViewport({pixelPosition.x,pixelPosition.y});

    const Vector2f worldPosition = context.MapPixelToWorld(pixelPosition);

    view.SetPointerState(insideViewport, pipeframe::backend::sfml::ToBackend(worldPosition));

    if (dragging) {
        UpdateDrag(worldPosition);
        return;
    }

    if (insideViewport) {
        view.SetHoveredObject(project.GetRuntime().HitTest(worldPosition));
        DispatchRuntimeEvent(project.GetRuntime(),event, context);
    } else {
        view.SetHoveredObject(std::nullopt);
    }
}

void WorkbenchInput::HandlePointerPress(const PointerInput &event, RenderContext &context) {

    if (!context.IsInsideWorldViewport({event.position.x,event.position.y})) {

        return;
    }

    const Vector2f worldPosition = context.MapPixelToWorld(event.position);

    view.SetPointerState(true, pipeframe::backend::sfml::ToBackend(worldPosition));

    const bool runtimeOverlay = project.GetRuntime().ConsumesPointerAt(event.position, context);

    if (!runtimeOverlay && !project.GetRuntime().UsesRightClickTool() && event.button == PointerButton::Right) {
        project.SetSelectedObject(project.GetRuntime().HitTest(worldPosition));
        NotifyStateChanged(false);
        view.ShowContextMenu(sf::Vector2f{float(event.position.x),float(event.position.y)});
        return;
    }

    const bool selectionClick = !runtimeOverlay && event.button == PointerButton::Left &&
                                !cameraController.IsPanModifierActive();

    if (selectionClick) {
        BeginSelectionOrDrag(worldPosition);
    }

    const InputEvent forwardedEvent{InputEventType::PointerPressed,event};

    DispatchRuntimeEvent(project.GetRuntime(),forwardedEvent, context);
    // A live project tool owns the world view; keep any unsaved map document,
    // but stop its opaque authoring overlay from hiding the live simulation.
    if(project.GetRuntime().HasWorldPointerCapture())project.GetTilemapEditor().SetEnabled(false);

    ApplyRuntimeEdits();

    if (!runtimeOverlay) {
        DispatchCamera(cameraController,forwardedEvent, context);
    }
}

void WorkbenchInput::HandlePointerRelease(const PointerInput &event, RenderContext &context) {

    if (dragging && event.button == PointerButton::Left) {

        CommitDrag();
    }

    const InputEvent forwardedEvent{InputEventType::PointerReleased,event};

    const bool runtimeOverlay = project.GetRuntime().ConsumesPointerAt(event.position, context);

    if (!runtimeOverlay) {
        DispatchCamera(cameraController,forwardedEvent, context);
    }

    if (context.IsInsideWorldViewport({event.position.x,event.position.y})) {

        DispatchRuntimeEvent(project.GetRuntime(),forwardedEvent, context);
        ApplyRuntimeEdits();
    }
}

void WorkbenchInput::ApplyRuntimeEdits() {
    if (!simulation.CanAuthorScene()) {
        project.GetRuntime().ConsumeSceneEdits();
        return;
    }
    if (project.ApplyRuntimeSceneEdits(project.GetRuntime().ConsumeSceneEdits())) {
        NotifyStateChanged(true);
    }
}

void WorkbenchInput::BeginSelectionOrDrag(const Vector2f worldPosition) {

    const TransformHandle handle = view.HitTestTransformHandle(pipeframe::backend::sfml::ToBackend(worldPosition));
    if (handle != TransformHandle::None && simulation.CanAuthorScene() &&
        BeginTransformDrag(worldPosition, handle)) return;

    const std::optional<SceneObjectId> hitObject = project.GetRuntime().HitTest(worldPosition);

    additiveSelection = Input::IsKeyDown(Key::Shift);
    subtractiveSelection = Input::IsKeyDown(Key::Control);
    dragStartWorld = worldPosition;
    dragCurrentWorld = worldPosition;

    if (!hitObject.has_value()) {
        if (!simulation.CanAuthorScene()) return;
        dragKind = DragKind::BoxSelection;
        dragging = true;
        view.SetSelectionBox(pipeframe::backend::sfml::ToBackend(Rectanglef{worldPosition, {0.0f, 0.0f}}));
        return;
    }

    if (subtractiveSelection) {
        if (project.IsObjectSelected(*hitObject)) project.ToggleSelectedObject(*hitObject);
        NotifyStateChanged(false);
        return;
    }

    if (additiveSelection) {
        project.AddSelectedObject(*hitObject);
    } else if (!project.IsObjectSelected(*hitObject)) {
        project.SetSelectedObject(hitObject);
    }
    NotifyStateChanged(false);

    if (!simulation.CanAuthorScene()) return;
    BeginTransformDrag(worldPosition, TransformHandle::None);
}

bool WorkbenchInput::BeginTransformDrag(const Vector2f worldPosition, const TransformHandle handle) {
    const SceneObjectData *object = project.GetSelectedObject();
    if (object == nullptr || !project.BeginContinuousEdit()) return false;
    dragStartWorld = worldPosition;
    dragCurrentWorld = worldPosition;
    dragOffset = object->transform.position - worldPosition;
    dragStartTransforms.clear();
    selectionPivot = {};
    for (const SceneObjectId objectId : project.GetSelectedObjectIds()) {
        if (const SceneObjectData *selected = project.GetDocument().FindObject(objectId)) {
            dragStartTransforms.emplace_back(objectId, selected->transform);
            selectionPivot += selected->transform.position;
        }
    }
    if (!dragStartTransforms.empty()) selectionPivot /= static_cast<float>(dragStartTransforms.size());

    dragKind = DragKind::Transform;
    activeHandle = handle;
    dragging = true;
    return true;
}

void WorkbenchInput::UpdateDrag(const Vector2f worldPosition) {

    dragCurrentWorld = worldPosition;

    if (dragKind == DragKind::BoxSelection) {
        const Vector2f origin{std::min(dragStartWorld.x, worldPosition.x),
                                 std::min(dragStartWorld.y, worldPosition.y)};
        const Vector2f size{std::abs(worldPosition.x - dragStartWorld.x),
                               std::abs(worldPosition.y - dragStartWorld.y)};
        view.SetSelectionBox(pipeframe::backend::sfml::ToBackend(Rectanglef{origin, size}));
        return;
    }

    if (dragStartTransforms.empty()) {
        CancelDrag();
        return;
    }

    std::vector<std::pair<SceneObjectId, SceneTransform>> updates;
    updates.reserve(dragStartTransforms.size());
    const Vector2f delta = worldPosition - dragStartWorld;
    const float primaryAngle = view.IsLocalTransformSpace() ? dragStartTransforms.back().second.rotation *
                                                              3.14159265f / 180.0f : 0.0f;
    const Vector2f xAxis{std::cos(primaryAngle), std::sin(primaryAngle)};
    const Vector2f yAxis{-std::sin(primaryAngle), std::cos(primaryAngle)};
    const auto dot = [](const Vector2f a, const Vector2f b) { return a.x*b.x+a.y*b.y; };

    for (const auto &[objectId, start] : dragStartTransforms) {
        SceneTransform transform = start;
        switch (view.GetTransformTool()) {
        case TransformTool::Move: {
            Vector2f constrained = delta;
            if (activeHandle == TransformHandle::MoveX) constrained = xAxis * dot(delta, xAxis);
            if (activeHandle == TransformHandle::MoveY) constrained = yAxis * dot(delta, yAxis);
            Vector2f position = start.position + constrained;
            if (view.IsPositionSnapEnabled()) {
                const float step = std::max(0.001f, view.GetPositionSnapStep());
                const auto origin = view.GetGridOrigin();
                position.x = origin.x + std::round((position.x-origin.x) / step) * step;
                position.y = origin.y + std::round((position.y-origin.y) / step) * step;
            }
            transform.position = position;
            break;
        }
        case TransformTool::Rotate: {
            const auto startVector = dragStartWorld - selectionPivot;
            const auto currentVector = worldPosition - selectionPivot;
            float degrees = (std::atan2(currentVector.y, currentVector.x) -
                             std::atan2(startVector.y, startVector.x)) * 180.0f / 3.14159265f;
            transform.rotation = start.rotation + degrees;
            if (view.IsRotationSnapEnabled()) {
                const float step = std::max(0.001f, view.GetRotationSnapStep());
                transform.rotation = std::round(transform.rotation / step) * step;
            }
            if (view.GetTransformPivotMode() == TransformPivotMode::Center && dragStartTransforms.size() > 1) {
                const Vector2f relative = start.position - selectionPivot;
                const float angle = (transform.rotation-start.rotation)*3.14159265f/180.0f;
                const Vector2f rotated{relative.x*std::cos(angle)-relative.y*std::sin(angle),
                                           relative.x*std::sin(angle)+relative.y*std::cos(angle)};
                transform.position = selectionPivot+rotated;
            }
            break;
        }
        case TransformTool::Scale: {
            float xFactor=1.0f,yFactor=1.0f;
            if(activeHandle==TransformHandle::ScaleX)xFactor=std::max(0.01f,1.0f+dot(delta,xAxis)/100.0f);
            else if(activeHandle==TransformHandle::ScaleY)yFactor=std::max(0.01f,1.0f+dot(delta,yAxis)/100.0f);
            else {
                const float factor=std::max(0.01f,1.0f+delta.x/100.0f);
                xFactor=yFactor=factor;
            }
            transform.scale = {start.scale.x*xFactor,start.scale.y*yFactor};
            if (view.IsScaleSnapEnabled()) {
                const float step = std::max(0.001f, view.GetScaleSnapStep());
                transform.scale.x = std::max(step, std::round(transform.scale.x / step) * step);
                transform.scale.y = std::max(step, std::round(transform.scale.y / step) * step);
            }
            if(view.GetTransformPivotMode()==TransformPivotMode::Center&&dragStartTransforms.size()>1){
                const Vector2f relative=start.position-selectionPivot;
                const Vector2f scaled=xAxis*(dot(relative,xAxis)*xFactor)+yAxis*(dot(relative,yAxis)*yFactor);
                transform.position=selectionPivot+scaled;
            }
            break;
        }
        }
        updates.emplace_back(objectId, transform);
    }

    if (project.UpdateSelectedTransforms(updates)) {

        NotifyStateChanged(false);
    }
}

void WorkbenchInput::CommitDrag() {
    if (!dragging) {
        return;
    }

    if (dragKind == DragKind::Transform) {
        project.CommitContinuousEdit();
    } else if (dragKind == DragKind::BoxSelection) {
        const Rectanglef bounds{
            {std::min(dragStartWorld.x, dragCurrentWorld.x), std::min(dragStartWorld.y, dragCurrentWorld.y)},
            {std::abs(dragCurrentWorld.x - dragStartWorld.x), std::abs(dragCurrentWorld.y - dragStartWorld.y)}};
        std::vector<SceneObjectId> hits;
        if (additiveSelection || subtractiveSelection) {
            hits.assign(project.GetSelectedObjectIds().begin(), project.GetSelectedObjectIds().end());
        }
        for (const SceneObjectData &object : project.GetDocument().GetObjects()) {
            const Vector2f position = object.transform.position;
            if (!bounds.Contains(position)) continue;
            const auto existing = std::ranges::find(hits, object.id);
            if (subtractiveSelection) {
                if (existing != hits.end()) hits.erase(existing);
            } else if (existing == hits.end()) {
                hits.push_back(object.id);
            }
        }
        project.SetSelectedObjects(std::move(hits));
    }

    view.SetSelectionBox(std::nullopt);
    dragStartTransforms.clear();
    dragKind = DragKind::None;
    activeHandle = TransformHandle::None;

    dragging = false;

    NotifyStateChanged(false);
}

void WorkbenchInput::NotifyStateChanged(const bool rebuildHierarchy) {

    if (onStateChanged) {
        onStateChanged(rebuildHierarchy);
    }
}

} // namespace pipeframe::editor

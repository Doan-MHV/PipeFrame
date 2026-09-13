#ifndef PIPEFRAME_WORKBENCH_INPUT_H
#define PIPEFRAME_WORKBENCH_INPUT_H

#include <functional>
#include <optional>
#include <vector>

#include <PipeFrame/Input/InputEvent.h>

#include <PipeFrame/Render/CameraController2D.h>

#include "SceneTypes.h"

class RenderContext;

namespace pipeframe::editor {

class ProjectSession;
class SimulationSession;
class WorkbenchView;
enum class TransformHandle;

class WorkbenchInput final {
public:
    using StateChangedCallback =
        std::function<void(bool rebuildHierarchy)>;

    WorkbenchInput(
        WorkbenchView &view,
        ProjectSession &project,
        SimulationSession &simulation);

    void SetOnStateChanged(
        StateChangedCallback callback);

    void HandleEvent(
        const InputEvent &event,
        RenderContext &context);

    void CancelDrag();

private:
    bool HandleKeyboard(
        const KeyInput &event,
        RenderContext &context);

    void HandlePointerMove(
        const InputEvent &event,
        Vector2i pixelPosition,
        RenderContext &context);

    void HandlePointerPress(
        const PointerInput &event,
        RenderContext &context);

    void HandlePointerRelease(
        const PointerInput &event,
        RenderContext &context);

    void BeginSelectionOrDrag(
        Vector2f worldPosition);

    bool BeginTransformDrag(Vector2f worldPosition, TransformHandle handle);

    void UpdateDrag(Vector2f worldPosition);
    void CommitDrag();

    enum class DragKind { None, Transform, BoxSelection };

    void NotifyStateChanged(bool rebuildHierarchy);

    void ApplyRuntimeEdits();

    WorkbenchView &view;
    ProjectSession &project;
    SimulationSession &simulation;

    CameraController2D cameraController;

    StateChangedCallback onStateChanged;

    Vector2f dragOffset{
        0.0f,
        0.0f,
    };

    Vector2f dragStartWorld{};
    Vector2f dragCurrentWorld{};
    Vector2f selectionPivot{};
    std::vector<std::pair<SceneObjectId, SceneTransform>> dragStartTransforms;
    bool additiveSelection = false;
    bool subtractiveSelection = false;
    DragKind dragKind{DragKind::None};
    TransformHandle activeHandle{};

    bool dragging = false;
};

} // namespace pipeframe::editor

#endif

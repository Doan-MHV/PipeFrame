#ifndef PIPEFRAME_VIEWPORT_TOOLBAR_H
#define PIPEFRAME_VIEWPORT_TOOLBAR_H

#include <functional>
#include <string>

#include <PipeFrame/UI/ViewPanel.h>
#include <map>

class ViewportToolbar : public pipeframe::ui::ViewPanel {
  public:
    using ActionCallback = std::function<void()>;

    ViewportToolbar();

    void SetOnNewProject(ActionCallback callback);
    void SetOnOpenProject(ActionCallback callback);
    void SetOnSave(ActionCallback callback);
    void SetOnSaveAs(ActionCallback callback);

    void SetOnMetrics(ActionCallback callback);
    void SetOnAssets(ActionCallback callback);
    void SetOnPanels(ActionCallback callback);
    void SetOnReload(ActionCallback callback);
    void SetOnBuild(ActionCallback callback);
    void SetBuildRunning(bool running);
    void SetOnPhysicsDebug(ActionCallback callback);
    void SetOnMeshDebug(ActionCallback callback);
    void SetPhysicsDebugVisible(bool visible);
    void SetMeshDebugVisible(bool visible);
    void SetOnGrid(ActionCallback callback);
    void SetOnGridStep(ActionCallback callback);
    void SetOnGridOrigin(ActionCallback callback);
    void SetOnSnap(ActionCallback callback);
    void SetOnRotationSnap(ActionCallback callback);
    void SetOnScaleSnap(ActionCallback callback);
    void SetOnTool(ActionCallback callback);
    void SetOnTransformSpace(ActionCallback callback);
    void SetOnPivot(ActionCallback callback);
    void SetOnFrame(ActionCallback callback);
    void SetOnViewMode(ActionCallback callback);
    void SetOnUndo(ActionCallback callback);
    void SetOnRedo(ActionCallback callback);

    void SetProjectName(const std::string &name);
    void SetMetricsVisible(bool visible);
    void SetAssetsVisible(bool visible);
    void SetPanelsVisible(bool visible);
    void SetGridVisible(bool visible);
    void SetGridStep(float step);
    void SetGridOrigin(float x, float y);
    void SetSnapEnabled(bool enabled, float step);
    void SetRotationSnapEnabled(bool enabled, float step);
    void SetScaleSnapEnabled(bool enabled, float step);
    void SetToolText(const std::string &text);
    void SetTransformSpaceText(const std::string &text);
    void SetPivotText(const std::string &text);
    void SetViewModeText(const std::string &text);
    void SetHistoryEnabled(bool undoEnabled, bool redoEnabled);
    void SetAuthoringEnabled(bool enabled);
    void SetStatusText(const std::string &text);
    static float PreferredHeight(float width);

  private:
    struct Action { std::string text; ActionCallback callback; bool enabled{true}; };
    std::map<std::string,Action> actions;
    std::string title, status;
    pipeframe::ui::View BuildView() override;
};
#endif

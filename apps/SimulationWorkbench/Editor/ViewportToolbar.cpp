#include "ViewportToolbar.h"
#include <cmath>
#include <utility>
using namespace pipeframe::ui;
ViewportToolbar::ViewportToolbar()  {
    SetPreferredSize({1000,148});
    actions["newProjectButton"].text="NEW";
    actions["openProjectButton"].text="OPEN";
    actions["undoButton"].text="UNDO";
    actions["redoButton"].text="REDO";
    actions["saveButton"].text="SAVE";
    actions["saveAsButton"].text="SAVE AS";
    actions["viewModeButton"].text="EDITOR";
    actions["assetsButton"].text="ASSETS";
    actions["panelsButton"].text="PANELS";
    actions["reloadButton"].text="RELOAD";
    actions["buildButton"].text="BUILD & RELOAD";
    actions["metricsButton"].text="METRICS";
    actions["toolButton"].text="MOVE";
    actions["transformSpaceButton"].text="WORLD";
    actions["pivotButton"].text="CENTER";
    actions["gridButton"].text="GRID ON";
    actions["gridStepButton"].text="GRID AUTO";
    actions["gridOriginButton"].text="ORIGIN 0,0";
    actions["snapButton"].text="POS OFF";
    actions["rotationSnapButton"].text="ANG OFF";
    actions["scaleSnapButton"].text="SCL OFF";
    actions["frameButton"].text="FRAME";
    actions["physicsDebugButton"].text="PHYSICS OFF";
    actions["meshDebugButton"].text="MESH OFF";
}
View ViewportToolbar::BuildView() {
    const auto row=[&](const char *key, std::initializer_list<const char *> ids) {
        std::vector<View> children;
        for (const auto *id : ids) { const auto &action=actions.at(id);
            children.push_back(views::Button(id,action.text,action.callback).Enabled(action.enabled).Height(32)); }
        return views::Wrap(key,std::move(children),88).Spacing(4);
    };
    return views::Scroll("toolbar-scroll",views::Column("toolbar",{
        views::Row("identity",{views::Text("title",title).FitHeight(),views::Text("status",status).FitHeight()}),
        row("actions",{"newProjectButton","openProjectButton","undoButton","redoButton","saveButton","saveAsButton"}),
        row("workspace",{"viewModeButton","assetsButton","panelsButton","buildButton","reloadButton","metricsButton","physicsDebugButton","meshDebugButton"}),
        row("tools",{"toolButton","transformSpaceButton","pivotButton","gridButton","gridStepButton","gridOriginButton","snapButton","rotationSnapButton","scaleSnapButton","frameButton"})
    }).Padding(8).Spacing(4)).FillHeight();
}
void ViewportToolbar::SetOnNewProject(ActionCallback callback) { actions["newProjectButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnOpenProject(ActionCallback callback) { actions["openProjectButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnSave(ActionCallback callback) { actions["saveButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnSaveAs(ActionCallback callback) { actions["saveAsButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnMetrics(ActionCallback callback) { actions["metricsButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnAssets(ActionCallback callback) { actions["assetsButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnPanels(ActionCallback callback) { actions["panelsButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnReload(ActionCallback callback) { actions["reloadButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnGrid(ActionCallback callback) { actions["gridButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnGridStep(ActionCallback callback) { actions["gridStepButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnGridOrigin(ActionCallback callback) { actions["gridOriginButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnSnap(ActionCallback callback) { actions["snapButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnRotationSnap(ActionCallback callback) { actions["rotationSnapButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnScaleSnap(ActionCallback callback) { actions["scaleSnapButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnTool(ActionCallback callback) { actions["toolButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnTransformSpace(ActionCallback callback) { actions["transformSpaceButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnPivot(ActionCallback callback) { actions["pivotButton"].callback=std::move(callback); InvalidateView(); }
void ViewportToolbar::SetOnFrame(ActionCallback callback) { actions["frameButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnViewMode(ActionCallback callback) { actions["viewModeButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnUndo(ActionCallback callback) { actions["undoButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetOnRedo(ActionCallback callback) { actions["redoButton"].callback=std::move(callback); InvalidateView(); }

void ViewportToolbar::SetProjectName(const std::string &name) {
    if (title != (name.empty() ? "PIPEFRAME" : "PIPEFRAME  |  " + name)) { title=name.empty() ? "PIPEFRAME" : "PIPEFRAME  |  " + name; InvalidateView(); }
}

void ViewportToolbar::SetMetricsVisible(const bool visible) { if (actions["metricsButton"].text != (visible ? "CLOSE" : "METRICS")) { actions["metricsButton"].text=visible ? "CLOSE" : "METRICS"; InvalidateView(); } }
void ViewportToolbar::SetAssetsVisible(const bool visible) { if (actions["assetsButton"].text != (visible ? "INSPECT" : "ASSETS")) { actions["assetsButton"].text=visible ? "INSPECT" : "ASSETS"; InvalidateView(); } }
void ViewportToolbar::SetPanelsVisible(const bool visible) { if (actions["panelsButton"].text != (visible ? "PANELS ON" : "PANELS")) { actions["panelsButton"].text=visible ? "PANELS ON" : "PANELS"; InvalidateView(); } }

void ViewportToolbar::SetGridVisible(const bool visible) { if (actions["gridButton"].text != (visible ? "GRID ON" : "GRID OFF")) { actions["gridButton"].text=visible ? "GRID ON" : "GRID OFF"; InvalidateView(); } }

void ViewportToolbar::SetGridStep(const float step) {
    if (actions["gridStepButton"].text != (step <= 0.0f ? "GRID AUTO" : "GRID " + std::to_string(step).substr(0,4))) { actions["gridStepButton"].text=step <= 0.0f ? "GRID AUTO" : "GRID " + std::to_string(step).substr(0,4); InvalidateView(); }
}

void ViewportToolbar::SetGridOrigin(const float x, const float y) {
    if (actions["gridOriginButton"].text != ("ORIGIN " + std::to_string(static_cast<int>(std::round(x))) + "," +
                              std::to_string(static_cast<int>(std::round(y))))) { actions["gridOriginButton"].text="ORIGIN " + std::to_string(static_cast<int>(std::round(x))) + "," +
                              std::to_string(static_cast<int>(std::round(y))); InvalidateView(); }
}

void ViewportToolbar::SetSnapEnabled(const bool enabled, const float step) {
    if (actions["snapButton"].text != (enabled ? "POS " + std::to_string(static_cast<int>(step)) : "POS OFF")) { actions["snapButton"].text=enabled ? "POS " + std::to_string(static_cast<int>(step)) : "POS OFF"; InvalidateView(); }
}

void ViewportToolbar::SetRotationSnapEnabled(const bool enabled, const float step) {
    if (actions["rotationSnapButton"].text != (enabled ? "ANG " + std::to_string(static_cast<int>(step)) : "ANG OFF")) { actions["rotationSnapButton"].text=enabled ? "ANG " + std::to_string(static_cast<int>(step)) : "ANG OFF"; InvalidateView(); }
}

void ViewportToolbar::SetScaleSnapEnabled(const bool enabled, const float step) {
    if (actions["scaleSnapButton"].text != (enabled ? "SCL " + std::to_string(step).substr(0,3) : "SCL OFF")) { actions["scaleSnapButton"].text=enabled ? "SCL " + std::to_string(step).substr(0,3) : "SCL OFF"; InvalidateView(); }
}

void ViewportToolbar::SetToolText(const std::string &text) { if (actions["toolButton"].text != (text)) { actions["toolButton"].text=text; InvalidateView(); } }
void ViewportToolbar::SetTransformSpaceText(const std::string &text) { if (actions["transformSpaceButton"].text != (text)) { actions["transformSpaceButton"].text=text; InvalidateView(); } }
void ViewportToolbar::SetPivotText(const std::string &text) { if (actions["pivotButton"].text != (text)) { actions["pivotButton"].text=text; InvalidateView(); } }

void ViewportToolbar::SetViewModeText(const std::string &text) { if (actions["viewModeButton"].text != (text)) { actions["viewModeButton"].text=text; InvalidateView(); } }

void ViewportToolbar::SetHistoryEnabled(const bool undoEnabled, const bool redoEnabled) {

    if (actions["undoButton"].enabled != (undoEnabled)) { actions["undoButton"].enabled=undoEnabled; InvalidateView(); }
    if (actions["redoButton"].enabled != (redoEnabled)) { actions["redoButton"].enabled=redoEnabled; InvalidateView(); }
}

void ViewportToolbar::SetAuthoringEnabled(const bool enabled) {
    if (actions["newProjectButton"].enabled != (enabled)) { actions["newProjectButton"].enabled=enabled; InvalidateView(); }
    if (actions["openProjectButton"].enabled != (enabled)) { actions["openProjectButton"].enabled=enabled; InvalidateView(); }
    if (actions["saveButton"].enabled != (enabled)) { actions["saveButton"].enabled=enabled; InvalidateView(); }
    if (actions["saveAsButton"].enabled != (enabled)) { actions["saveAsButton"].enabled=enabled; InvalidateView(); }
}

void ViewportToolbar::SetStatusText(const std::string &text) { if (status != (text)) { status=text; InvalidateView(); } }

float ViewportToolbar::PreferredHeight(float width) {
    const float columns=std::max(1.0f,std::floor((width-16+4)/92));
    const float lines=std::ceil(6/columns)+std::ceil(8/columns)+std::ceil(10/columns);
    return 40+lines*36;
}

void ViewportToolbar::SetOnBuild(ActionCallback callback){actions["buildButton"].callback=std::move(callback);InvalidateView();}
void ViewportToolbar::SetBuildRunning(bool running){const std::string text=running?"CANCEL BUILD":"BUILD & RELOAD";if(actions["buildButton"].text!=text){actions["buildButton"].text=text;InvalidateView();}}

void ViewportToolbar::SetOnPhysicsDebug(ActionCallback callback){actions["physicsDebugButton"].callback=std::move(callback);InvalidateView();}
void ViewportToolbar::SetOnMeshDebug(ActionCallback callback){actions["meshDebugButton"].callback=std::move(callback);InvalidateView();}
void ViewportToolbar::SetPhysicsDebugVisible(bool value){actions["physicsDebugButton"].text=value?"PHYSICS ON":"PHYSICS OFF";InvalidateView();}
void ViewportToolbar::SetMeshDebugVisible(bool value){actions["meshDebugButton"].text=value?"MESH ON":"MESH OFF";InvalidateView();}

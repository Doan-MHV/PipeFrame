# R5 — shared declarative editor and Ant UI

Status: R5 UI-content migration implemented and verified — September 12, 2026.

R5 migrates the active Ant and editor **UI content** to the same public view API. The engine remains general-purpose. SailBoat is excluded and its UI has not been migrated by this work. This is not a claim of Pezzza visual parity, complete backend isolation, or completed R6 authoring workflows.

## One UI authoring path

Applications build `pipeframe::ui::View` descriptions. `StatefulView` publishes state revisions; `MountedView` and its engine-owned host reconcile controls at the next UI frame/input boundary. The retained widgets and backend rendering stay behind that host. There is no new editor-only rendering or input system.

For native dock embedding, the engine now provides `ViewPanel` and `ViewBuilderPanel`. `ViewPanel` owns a mounted `StatefulView` and coalesces `InvalidateView()` calls. A presenter implements `BuildView()` and owns application data/commands; it does not create child widgets or synchronize widget pointers. The host handles measurement, clipping, focus, input and destruction. Standalone UI can continue using `UIManager::MountView(state.Describe(...))`, as demonstrated in [R4](R4_UI_RUNTIME.md).

The native window/font/dock integration remains a distinct boundary. In particular, `AntUIHost.cpp`, `WorkbenchLayout.cpp` and the host portions of `WorkbenchView.cpp` still contain native integration. R6 must consolidate/isolate these adapters and generated-project wiring. A `ViewPanel` constructor accepting a native font is an embedding adapter, not the API that simulation view builders use. This distinction must not be presented as “every SFML dependency is gone.”

## Reference implementations

| Surface | Implementation | Preserved behavior |
|---|---|---|
| Editor toolbar | [ViewportToolbar.cpp](../../apps/SimulationWorkbench/Editor/ViewportToolbar.cpp) | File/history actions, workspace modes, assets, reload, metrics, transform tools and snapping; responsive action wrapping |
| Hierarchy | [HierarchyPanel.cpp](../../apps/SimulationWorkbench/Editor/HierarchyPanel.cpp) | Selection, add/delete, selected styling and scrolling |
| Inspector | [InspectorPanel.cpp](../../apps/SimulationWorkbench/Editor/InspectorPanel.cpp) | Actual component discovery, metadata fields, multi-selection/mixed values, custom presentation, foldouts, read-only telemetry and existing edit transactions |
| Asset browser | [AssetBrowserPanel.cpp](../../apps/SimulationWorkbench/Editor/AssetBrowserPanel.cpp) | Search, type filter, import/assign/reimport/cancel, selection and drag hit testing; former 48-row cap removed |
| Workspace tools | [WorkspaceToolsPanel.cpp](../../apps/SimulationWorkbench/Editor/WorkspaceToolsPanel.cpp) | All eight tabs, scrollable output, command search/invocation; former 12-command cap removed |
| Welcome/project browser | [ProjectBrowser.cpp](../../apps/SimulationWorkbench/Editor/ProjectBrowser.cpp) | New/open/recent projects and messages; scrollable list |
| Context menu, floating header, Zen recovery | [WorkbenchView.cpp](../../apps/SimulationWorkbench/Editor/WorkbenchView.cpp), [WorkbenchLayout.cpp](../../apps/SimulationWorkbench/Editor/WorkbenchLayout.cpp) | Existing host gestures/modal routing with declarative content and actions |
| Diagnostics | [DiagnosticsOverlay.cpp](../../apps/SimulationWorkbench/DiagnosticsOverlay.cpp) | Sampled performance metrics and scrollable detail |
| Shared simulation transport | [SimulationTransportView.h](../../engine/include/PipeFrame/UI/SimulationTransportView.h) | Play/pause, step/reset availability, speed selection; reused by Inspector and Zen transport |
| All six Ant panels and timer | [AntDashboard.cpp](../../examples/AntSimulation/Source/Editor/AntDashboard.cpp) | Environment tools/radius, render options/intensity, colony selection/history charts, selected-ant preview/actions/details/bars, profiler, playback and simulation time |

Ant no longer stores pointers to dashboard labels, sliders, cards, tables, charts or progress bars. Its native host installs builders into the engine dashboard's mounted slots. Native dashboard chrome is still implemented by the engine's retained controls; legacy APIs remain for other consumers, including the excluded SailBoat project. The old Ant-specific selected-preview widget/renderer was removed; the engine's `Mesh` view renders the same detailed textured Ant geometry.

## Shared controls

[View.h](../../engine/include/PipeFrame/UI/View.h) now also exposes:

- `Wrap`: equal-width action rows that wrap according to width constraints.
- `Card`, `Chart` and `Progress`: neutral data descriptions backed by existing engine controls.
- `Mesh`: normalized triangle layers, texture handles and shared resource ownership for previews. Applications supply geometry; they do not receive a native drawing target.
- `NumberField` and `TextField`: captions above inputs with retained editing buffers.
- Selected and leading-text presentation for actions and list items.

Buttons wrap text and clip painting to their own bounds. The shared Scroll view retains its offset after reconciliation, clamps against final content bounds and exposes an overflow indicator. It currently supports wheel/trackpad scrolling; the indicator is not draggable.

## Following the pattern in a new project

A simulation view builder can use only neutral data and callbacks:

```cpp
#include <PipeFrame/UI/View.h>
using namespace pipeframe::ui;

View BuildRobotPanel(float battery, bool lidarEnabled,
                     std::function<void(bool)> setLidar) {
    return views::Scroll("robot-panel", views::Column("content", {
        views::Card("status", "ROBOT", "Connected"),
        views::Text("battery-label", "Battery").FitHeight(),
        views::Progress("battery", battery),
        views::Toggle("lidar", "LIDAR", lidarEnabled, std::move(setLidar))
    }).Padding(12)).FillHeight();
}
```

Keep keys stable. The callback changes the robot/model state; publishing that state through `StatefulView::SetState` or invalidating the native presenter's view schedules a rebuild. Do not retain `TextButton*`, poll `Build()` manually, invoke a private renderer or create another input dispatcher. Ant's view builder is the full simulation example; the toolbar and asset browser demonstrate editor compositions using the same API.

For exposed properties, register component schemas and route edits through the editor's transaction callback. R5 keeps the verified R2 validation/undo/save path. The selected-ant panel's optional **Live Components** disclosure enumerates the selected Ant's actual registry-backed ECS components and renders them with `SchemaInspector`. These transient runtime values are read-only telemetry; they are not written into scene authoring history. Colony authoring fields remain editable through the main Inspector.

## Verification and boundaries

- `MountedViewRegression`: existing lifecycle/focus/disposal checks plus responsive action wrapping, real hit regions, resize and chart/progress updates.
- `AntComponentEditingAcceptance`: real ECS edits, history/persistence, exact integers, and a short Inspector retaining scroll through refresh/resizing/selection changes.
- `AntDashboardAcceptance`: all six panels at 1000×800 and 640×480; press/refresh/release, close/reopen, environment mode changes, render toggles, selected-ant follow/component disclosure and playback.
- `WorkbenchUIAcceptance`: component foldouts, assets/drag assignment, toolbar commands, all eight tab clicks, dock/float/Zen/input and existing runtime integration checks.
- Dependency lint rejects imperative widget construction/synchronization in the migrated presenters.

The saved [evidence gallery](evidence/r5/README.md) covers editor Inspector/assets/menu/projects/output tabs at 1440×900 and 800×700, all Ant panels at both viewport sizes, and the actual Ant project running in the editor. Generic editor screenshots use the test runtime to exercise multiple property kinds; `ant-workbench.png` uses the real Ant runtime. Neither is a comparison against YouTube frames.

A visual review found and corrected a timer showing through an open Ant panel. The timer is now hidden while a panel is open. Narrow toolbar height adapts to wrapped action rows; remaining overflow is scrollable. Existing R0 reference-comparison and R1 transform-hierarchy gaps are unaffected. R6 remains responsible for backend-host consolidation and create/build/reload/attach workflows; R7 owns final cross-stage cleanup and demonstration.


### Shared-UI compatibility review

The UI gallery transport test now locates stable view keys instead of depending on private child-array indexes. ThermalLab snapshots flush/settle mounted UI while the simulation is paused. Its three golden images were visually reviewed and updated for the intentional transport migration; comparison thresholds are unchanged. Pixel differences against the previous baselines were confined to the transport region: narrow `(19,651)–(621,782)`, wide `(19,651)–(1181,782)`, scaled `(28,977)–(1171,1173)`. Legacy unwrapped TextButton measurement remains unchanged for retained consumers.

## Final acceptance — September 12, 2026

The build succeeded and the full Ant-only rework suite passed **64/64 tests** in 60.68 seconds. See the [full results](R5_ACCEPTANCE_RESULTS.txt). Following the final tab-width adjustment, [Workbench interaction acceptance and screenshot capture](R5_EDITOR_ACCEPTANCE_RESULTS.txt) passed again; [Ant dashboard acceptance and captures](R5_ANT_ACCEPTANCE_RESULTS.txt) also passed. The final floating-window capture was visually checked for readable wrapped tabs. SailBoat remained excluded.

Reproduce the evidence after building:

```sh
cmake-build-debug/apps/SimulationWorkbench/WorkbenchUITests --r5-evidence docs/rework/evidence/r5
cmake-build-debug/examples/AntSimulation/AntDashboardTests /tmp/r5-ant-collapsed.png docs/rework/evidence/r5
```

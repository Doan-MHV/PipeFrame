# R4 — mounted declarative UI runtime

Scope: general engine UI, with Ant as the active integration reference. R5 still owns the full Ant/editor panel conversion. This is not a claim of Unity/Flutter API compatibility, completed editor design, or Pezzza visual parity.

## One authoring path

New UI is composed with backend-neutral `View` descriptions, `StatefulView<State>`, and `MountedView`. The existing retained widgets implement measurement, drawing and interaction behind the private `ViewRenderer` adapter. The new runtime does not introduce another renderer, input dispatcher or simulation loop.

The native host initializes its font/theme and calls `UIManager::Update` using real UI time. `HandleEvent` also flushes pending UI work before dispatch, so a removed popup or control cannot consume the next event. Simulation pause does not stop UI rebuilds. Native host consolidation and generated project wiring remain R6.

```cpp
#include <PipeFrame/UI/StatefulView.h>

using namespace pipeframe::ui;

// ui is the engine-owned, initialized UI host.
StatefulView<int> counter(0,
    [](const int &count, const StatefulView<int>::Setter &set) {
        return views::Column("counter-panel", {
            views::Text("count", std::to_string(count)).FitHeight(),
            views::Button("increment", "Increment", [set] {
                set([](int &value) { ++value; });
            })
        }).Padding(12);
    });

auto mounted = ui.MountView(counter.Describe("counter"));
mounted.SetBounds(0, 0, 320, 200);
// No polling counter.Build(), refreshing buttons, or manual event forwarding.
// The native host owns frame/input dispatch.
```

For application/model changes, `counter.SetState(mutation)` schedules the same next-frame rebuild. Mutations operate on a copy and publish only when they succeed. A builder runs only after that source's revision changes; multiple mutations before a frame coalesce into one build. A dirty nested source rebuilds even when its parent builder stays cached.

Builders describe UI and must not perform application commands. Event callbacks perform commands, then publish the resulting state. The setter passed to a reactive builder holds a weak source reference and a mount generation: retaining an old event callback cannot mutate a disposed or subsequently remounted source.

## Ownership and lifecycle

- The UI host owns mounted widgets. `MountedView` is a non-owning handle. Dropping the handle does not destroy the UI; `Unmount()` schedules removal at the next frame/input boundary.
- Host destruction unmounts its sources and invalidates handles. Child sources receive cleanup before their parents.
- `SetLifecycle(onMount, onUnmount)` configures resource setup/cleanup. Use `RetainForMount(subscription)` during `onMount` for automatic RAII subscription disposal. Existing `State<T>::Observe` subscriptions work with it.
- A source has one mounted location. Duplicate sources or moving a live source to a different key are rejected. Unmount it first, or create separate view instances backed by shared application state.
- Source state survives an explicit remount while its `StatefulView` handle lives. Rendered callbacks and mount subscriptions do not survive removal.
- Sibling keys are unique and stable. Same key and control type retain focus, text edit buffers and scroll position. Reordering keys changes layout/hit-test order. Type/source replacement destroys the old subtree.
- Mounted reconciliation destroys absent controls and releases their captures and bindings. Legacy retained `Compose` keeps its old hide/retain semantics for compatibility; new project UI should use the mounted path.
- Invalid descriptions/builds are rejected before retained reconciliation. A failed builder leaves the previous tree intact and can recover after the source is corrected. Allocation failures or exceptions inside native widget configuration are not a general transactional rendering guarantee.
- UI state, mounting and dispatch are UI-thread operations. Lifecycle-created roots are deferred until traversal ends. Recursive UI updates are rejected.

## Layout and input

Descriptions currently expose Row, Column, Text, Button/Toggle, Input, Slider, Scroll, Stack, Popup and nested Stateful sources. Row/Column support spacing, padding, weighted expansion and content height; fixed size and fill-height modes remain available. Text measurement uses the same wrapping implementation as painting. Flex rows measure against allocated widths. Scrolling clamps offset and clips both rendering and hit testing.

A popup has one content view (usually a Column). Its host-sized modal overlay prevents background pointer and keyboard input; compose a close button that changes state/removes the popup. Stack is deliberately an overlay layout. Use Row/Column for sequential controls.

The UI manager holds weak lifetime checks alongside focus/hover/capture pointers. Removed widgets cannot leave dangling input targets. Disabled/hidden controls lose ownership at refresh. Scoped subscriptions also check widget lifetime if an observer callback was already queued for delivery.

Specialized controls such as charts, network views, full tabs/dropdowns, asset pickers and viewport composition still have retained implementations. Their neutral project-facing composition and use throughout Ant/editor surfaces belong to R5. Do not treat this runtime gate as full control-library or project UI completion.

## Schema-generated Inspector

`SchemaInspector(key, descriptor, values, commit, error)` builds fields from the existing `ComponentSchema` metadata. It handles boolean, integer, number, string, vector, color, enum, asset-reference and object-reference value types. Labels include units; read-only fields are disabled; malformed/nonfinite numeric text, range violations and invalid enum choices are rejected before the commit callback.

The caller supplies an edit command, not widget construction. For a standalone component this can call `schema.Apply(component, {{key, value}}, error)` and publish state after success. In the editor this must go through its selection/undo/persistence transaction. The authored colony live ECS/editor bridge is now verified in [R2](R2_COMPONENT_EDITING.md); full panel migration remains R5. Reference controls currently edit IDs as text; they do not claim to resolve assets or validate object existence. Enum fields currently validate text against registered choices.

The acceptance fixture uses a non-Ant movement component to demonstrate engine reuse. It commits a field through `ComponentSchema::Apply`, rejects invalid values, and renders at a narrow 340-pixel width.

## Verification

`MountedViewRegression` exercises real UI-manager event dispatch and retained controls:

- Independent nested rebuilds, clean-frame caching, mount/remount and child teardown.
- Stale setter cancellation and RAII observer disposal.
- Keyed reorder, focused text buffer retention and same-key type replacement.
- Captured control removal, disabled controls and callback resource release.
- Constrained text wrapping, scroll offset retention and wheel input.
- Modal background blocking, keyboard traversal and restored input after removal.
- Builder failure/recovery, duplicate-source rejection and host destruction.
- Root creation from a lifecycle callback.
- Schema-generated edits and rejected range/nonfinite values.

The existing UI framework, Workbench and Ant dashboard checks remain enabled. SailBoat runtime and tests are excluded through `PIPEFRAME_ANT_REWORK_ONLY=ON`.

Final validation: build passed; **62/62 registered tests passed** in the Ant-only rework configuration.

Saved results: [R4 acceptance results](R4_ACCEPTANCE_RESULTS.txt).
Saved engine Inspector fixture: [R4 Inspector](evidence/r4-inspector.png).

The image is an engine acceptance fixture, not a screenshot of the finished Ant/editor UI.

R5 follow-up: editor and Ant view content now use this mounted path, with shared responsive actions, charts, progress, previews and transport. See [R5 migration](R5_UI_MIGRATION.md) for current coverage and remaining native-host boundaries.

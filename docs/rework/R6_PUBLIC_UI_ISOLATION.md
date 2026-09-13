# R6 package 1 — public UI and editor backend isolation

**Complete September 12, 2026.** Verification is recorded below. This closes the backend-boundary
package only; the other R6 authoring and performance packages retain their own gates.

## Public contract

All 14 headers under `engine/include/PipeFrame/UI` are neutral. They expose View,
ViewPanel, StatefulView, ViewSource, MountedView, component-schema inspectors,
SimulationDashboard, transport views, themes and interaction controllers. They do not
include native widgets or backend adapters, directly or transitively.

All 17 headers under `apps/SimulationWorkbench/Editor` compile independently without
backend include paths. The six editor panel presenters still build the same declarative
View trees used by project UI. The native window, docks, workbench rendering host and
diagnostics host now live under `apps/SimulationWorkbench/Backend/SFML`.

44 legacy imperative UI headers, including Widget, Controls, UIManager, Layout, UITheme,
SimulationTransport and their dependent widgets, are explicitly backend APIs beneath
`PipeFrame/Backend/SFML/UI`. Their implementation files move with them. The legacy native
Application/Scene/Input contracts also live beneath `PipeFrame/Backend/SFML`. There are
no forwarding headers or aliases at the retired neutral paths.

`PipeFrame::Engine` links its backend privately. Project consumers no longer inherit SFML
headers or link requirements. Native desktop hosts and native test fixtures explicitly
opt into `PipeFrame::BackendSFML`. Ant Source remains entirely neutral and its actual
runtime now compiles without inherited native include paths.

## How to write project UI

Use `PipeFrame/UI/View.h` and `PipeFrame/UI/ViewPanel.h` for declarative content; use
`PipeFrame/UI/SimulationDashboard.h` to host simulation drawers with resource/font handles,
InputEvent and RenderContext. Ant's `Source/Editor/AntDashboard.cpp` and the Workbench's
`Editor/InspectorPanel.cpp` are real consumers. New runtime logic uses ProjectRuntime or
SceneProjectRuntime and engine scene/components, not the legacy native host Scene.

Existing imperative/native integrations must explicitly include backend headers and link
`PipeFrame::BackendSFML`. This is an intentional source API migration, not an attempt to
present native Widget as backend-neutral. The native renderer remains SFML; this package
does not replace the rendering backend. SailBoat is outside the configured Ant rework scope
and was not migrated or tested here.

## Regression protection

- `UIEditorPublicHeadersCompile`: generates a separate translation unit for every public
  UI/editor header, with only engine include paths, no Engine/backend linkage.
- `PublicEngineConsumerCompile`: links the real Engine usage requirements and fails if an
  SFML header becomes discoverable. Covers accidental public CMake dependency leaks.
- Existing AntUIBoundaryCompile, WorkbenchPresenterBoundaryCompile and
  RenderPublicHeadersTests continue checking actual consumers.
- Dependency lint now scans all non-backend SDK headers and all public editor headers,
  not a hand-picked inventory. Backend includes/types outside the boundary fail the lint.

The runtime plugin interface and data layouts are unchanged: plugin ABI stays 7. Existing
scene/asset serialization is unchanged. Header moves do require recompilation of legacy
source consumers. No simulation algorithms or UI interaction behavior changed here.

## Verification

- Full configured Debug suite: **71/71 passed**, 100.87 seconds, including native UI,
  generated-project configure/build/reload, Ant behavior and stress tests.
- Full Debug and Release builds pass, including 31 individual public-header compilations
  and the Engine consumer dependency check.
- Native Workbench acceptance captures at 1100×800 and 800×800 passed and were visually
  inspected: toolbar wrapping, panel separation and Inspector clipping/scrollbar retained.
- Ant dashboard interaction passed; all six drawers were captured at normal and narrow
  sizes. The 1000-pixel editor drawer was visually inspected.
- Evidence and actual neutral compiler commands: `evidence/r6-public-ui-isolation/`.

The five-colony performance target remains the separate R6 package 7 gate. These changes
preserve the existing implementation algorithms; this report makes no new FPS claim.

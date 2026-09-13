# R6 package 1 — neutral editor panel presenters

Historical checkpoint. The remaining package-1 boundary is now closed; see [R6_PUBLIC_UI_ISOLATION.md](R6_PUBLIC_UI_ISOLATION.md).

Implemented September 12, 2026. Package 1 remains in progress for the remaining native
window/dock/widget interfaces. This is not tilemap or brush implementation.

## Public authoring model

ViewPanel is now a neutral declarative presenter, with BuildView, invalidation revision,
preferred initial size and an engine-bound semantic hit-test callback. It no longer
inherits native Panel, requires a font, or includes SFML, layout or animation backend
headers. It is non-copyable because it owns presenter state and host bindings.

The engine's HostedViewPanel adapter supplies the native embedding. It mounts the same
View/StatefulView pipeline, coalesces invalidations, preserves native layout/input and
unmounts before destroying presenter state. Projects describe controls; their panel
constructors no longer need to manage fonts or native resources.

Migrated actual Workbench presenters:

- HierarchyPanel
- InspectorPanel
- ViewportToolbar
- AssetBrowserPanel
- WorkspaceToolsPanel
- ProjectBrowser

Asset hit testing requests a semantic key through the neutral host callback. Native
clipping and tree traversal stay in the engine host; the presenter receives the asset
ID, not a Widget pointer. Existing asset dragging still uses this route.

WorkbenchLayout and WorkbenchView explicitly embed these presenters through the adapter.
DiagnosticsOverlay, SimulationTransport and native ViewBuilderPanel still use the legacy
native host; that debt is visible rather than hidden behind the public presenter name.

## Tile editor contract

The forthcoming palette, layers, material settings, brush settings, dialogs and toolbars
must derive from/use this same ViewPanel and View/StatefulView authoring model. No new UI
framework is introduced for tile editing. Preview geometry belongs to the neutral viewport
rendering API, and tool gestures/transactions belong to the engine's tool/edit services.

A developer writes a parameterless panel that overrides BuildView and returns ordinary
views. Setters update their data and call InvalidateView. Existing InspectorPanel.cpp and
HierarchyPanel.cpp are concrete examples. Native font/container construction is hosting
infrastructure, not code each brush developer should reproduce.

## Compatibility and checks

Plugin ABI is now 6. Rebuild project libraries with this SDK; the existing host rejects
previous ABI libraries before registration. No scene serialization version changes.

WorkbenchPresenterBoundaryCompile compiles all six real .cpp files using only engine
and editor includes, without linking Engine or inheriting backend include paths.
RenderPublicHeadersTests includes the new ViewPanel contract. Dependency lint guards
all six presenter headers and ViewPanel against direct native type/include regressions.

UIHardeningTests verifies initial mounting/preferred size, caching unchanged frames,
coalesced invalidation and native lifetime release. Existing native Workbench and Ant
component-editing tests exercise the migrated presenters through the actual host.
Final verification: **69/69 tests passed** in 97.49 seconds. Debug and Release builds
passed, including all six actual presenter sources without backend includes. Native
Workbench interactions passed. Saved 800-pixel Inspector and 1440-pixel asset-browser
images were inspected. Evidence is saved in evidence/r6-presenters/.

## Still open

Native Widget/Controls, Workbench window/dock/input/render host headers, DiagnosticsOverlay
and SimulationTransport boundaries still need work. Other R6 packages (playground/assets,
brushes, shared geometry, Ant environment migration, performance and integrated acceptance)
are not completed by this presenter migration. Follow R6_EXECUTION_PLAN.md.

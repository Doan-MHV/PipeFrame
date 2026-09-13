# 18F — Docking, layouts, and professional workspace

Status: complete (2026-09-11).

## Delivered behavior

SimulationWorkbench now uses one responsive workspace shell with a central
viewport, a resizable hierarchy/inspector or asset dock, and a resizable bottom
tool dock. The **Panels** toolbar action opens the bottom dock on wide screens.
On compact screens it opens the same tools as one modal panel, so the controls
remain usable without stacking panels over the viewport. Clicking outside the
panel or pressing Escape closes it and removes its input barrier.

The standard tool area supplies Console, Profiler, Learning, Game,
Connections, and Telemetry tabs. Profiler data comes from the active project
runtime, authored connections populate the Connections tab, and live component
edits populate Telemetry. Every tab has a useful empty state before a project
provides data. Hierarchy, Inspector, Assets, and the existing project simulation
drawers remain part of the same workspace.

The side and bottom splitters clamp their sizes so neither dock can consume the
entire editor. Their values and panel visibility are saved in
`projects/.workbench.layout`, restored on launch, and reset through the
workspace context menu. The layout switches between columns and rows at compact
widths and keeps the viewport, hierarchy, inspector, asset browser, and tool
panels disjoint.

The standard tool suite can also leave the bottom dock as a working floating
workspace window. Its title bar supports direct movement, its lower-right grip
supports resizing, and **Dock** returns it to the split layout. Movement and
resizing enforce a usable minimum and clamp the complete window to the current
work area. **Close**, the toolbar toggle, Reset Layout, Zen mode, and project
browser transitions remove the corresponding input surface. Floating mode and
bounds use the same saved layout record as the docked panel.

Editor and Simulation select independent center viewport hosts in the persisted
`viewport` tab group, preserving their independent camera state. F6 cycles the
workspace, F7 toggles Assets, and F8 toggles standard tools even when another
toolbar control has focus. Tab traversal continues to cover the controls within
the active surface.

## Reusable workspace contract

`PipeFrame/Project/EditorWorkspace.h` provides the backend-neutral layout
model. A panel has a stable ID, dock site, tab group and order, selected and
visible state, dock size, optional floating bounds, display ID, and workspace
DPI scale. The manager can register, remove, show, dock, float, resize, and
select panels, and can save or load the versioned layout format.

Layout reconciliation removes unavailable plugin panels and moves a floating
panel back into the primary work area when its monitor no longer exists. A
floating panel remains assigned to an available secondary display and is
clamped against that display's DPI-aware work area. Center and floating dock
sites let additional scene/game viewport hosts use the same persistence model;
project-defined render content and panel registration use the 18G extension
contract.

The Workbench implementation remains in the application backend layer. Generic
workspace state does not expose SFML and can be reused by another renderer or
window backend.

## Interaction and visual rules

- The central viewport always receives the remaining workspace area after docks
  are measured.
- Hidden docks are removed from layout and input hit testing.
- Compact tools use one complete modal surface rather than detached handles or
  partially visible panels.
- Tab content uses the shared dark-theme tokens, spacing, borders, selection,
  focus, and empty-state conventions.
- Tab buttons resize their labels with their final arranged bounds, preserving
  alignment at compact and wide sizes.
- Zen mode and the project browser dismiss tool popups so no invisible overlay
  blocks the next workspace.
- Reset Layout restores safe default dimensions and closes transient panels.
- The title/status row, command toolbar, context menu, searchable Assets and
  Commands views, selected/disabled/focus states, empty states, importer
  progress/errors, and modal input barriers use the shared UI theme and control
  primitives.

## Verification

`EditorWorkspaceRegression` covers unique stable IDs, movement across every
dock edge, scene/game center tab selection, resize, floating placement on an
available secondary display, missing-monitor recovery, save/load,
missing-plugin reconciliation, validation, and reset.

`WorkbenchUIAcceptance` covers 640×800, 1280×720, 1920×1080, 2560×1440, and
3440×1440 layouts; viewport/panel separation; all eight standard and extension
tabs; the wide bottom dock; compact modal presentation; Escape dismissal;
floating movement, bounds persistence, re-docking; F7/F8 reachability; and
Editor/Simulation viewport switching. Reviewed screenshots are stored in
`docs/milestone18/screenshots`, including a floating-window state and the
1440p/ultrawide sizes. The final checkpoint builds the complete Debug tree and
runs all registered tests.

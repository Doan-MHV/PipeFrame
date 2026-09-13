# Workbench UI (Milestone 16F)

The Workbench uses the shared retained UI framework for its toolbar, hierarchy,
inspector, diagnostics, project browser, context menu, and Zen controls.
`WorkbenchLayout` composes rows, columns, an overlay viewport, and a
`ZenModeShell`. The only window-coordinate adapters are the camera viewport
and pointer-anchored context menu. Native file and directory pickers remain
platform dialogs.

At widths below 1000 pixels the hierarchy and inspector move below the world;
larger windows place them beside it. Inspector fields and project-browser
content scroll when necessary. Acceptance checks cover 640×800, 1100×800,
and 1920×1080. Zen expands the world while retaining transport and Exit Zen.

## Editing and input

- Editor and Simulation modes both expose hierarchy, inspector, and authoring
  actions. Adding, deleting, editing properties/transforms, dragging, saving,
  and undo/redo remain available during playback.
- Edits use the existing runtime synchronization contract. The host preserves
  play/pause state; individual runtimes may rebuild domain state during
  synchronization. Continuous simulation history is not guaranteed across edits.
- Left click selects and drags. An active world drag retains capture across UI
  panels. Focus loss or leaving the window cancels the uncommitted drag.
- Right click selects and opens Add Object, Delete Selected, Save, and Close.
  Project-browser and context-menu barriers isolate keyboard and pointer input.
  Escape dismisses the context menu; overlays preempt active world drags.
- Tab/Shift-Tab traverse UI controls. F6 cycles workspace modes when no control
  owns keyboard focus. P toggles playback, S toggles maximum speed, and period
  steps a paused simulation. The visible transport provides the same actions.
- Opening or creating another project stops the previous preview before the
  runtime is replaced.

## Verification

Build `SimulationWorkbench` and `WorkbenchUITests`, then run:

```sh
ctest --test-dir cmake-build-debug -R WorkbenchUIAcceptance --output-on-failure
```

The test uses an isolated runtime that deliberately stops on synchronization
to verify resumed playback and preserved pause state. It exercises responsive
viewport bounds, live authoring, right-click selection, overlay isolation,
undoable drags, focus loss, capture across panels, Tab focus, and Zen recovery.
The existing serializer, project-manager, scene-history, and runtime-edit
regressions cover persistence and authored edit semantics.

For a reproducible visual check:

```sh
cmake-build-debug/apps/SimulationWorkbench/WorkbenchUITests --snapshot /tmp/workbench.png 640 metrics
```

The optional final argument can be `metrics`, `menu`, `browser`, or `zen`;
omit it for the editor. Snapshot telemetry comes from a deterministic fixture,
not a performance benchmark. Visual verification covered desktop and narrow
editor layouts, the browser, metrics, context menu, and Zen transport.

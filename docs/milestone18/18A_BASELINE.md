# Milestone 18A implementation baseline

Status: **complete**

Completed: 2026-09-11

## Delivered viewport workflow

- Adaptive 1/2/5 world-space grid spacing, stronger major lines, distinct axes,
  rulers, collision-free ruler labels, coordinate labels, and multi-selection
  distance measurements.
- Visible controls for grid visibility, automatic or fixed grid step, grid
  origin, position snap, angle snap, scale snap, world/local space, and
  center/individual pivot mode.
- Position snapping relative to the authored grid origin. Position, rotation,
  and scale edits remain continuous transactions that commit as one undo step.
- Hover, click, rectangle, additive, subtractive, and multi-object selection.
- Direct screen-space hit targets for free and axis-constrained movement, the
  rotation ring, uniform scale, and independent X/Y scale handles. Handles stay
  the same usable pixel size across camera zoom.
- Shared-center and individual-pivot multi-object transforms with world/local
  axes.
- Frame-selection, frame-scene, four camera bookmarks, and separate editor and
  simulation camera state.
- Asset drag placement feedback and context actions for object creation at the
  captured world position, grid-origin placement/reset, deletion, and prefab
  operations.
- Registered component attachment points, authored mechanical/power/signal
  connection lines, generic collider bounds, metadata-driven sensor ranges,
  and backend-neutral debug geometry submitted by registered project systems.
- Popup dismissal releases input immediately while its visual fade completes,
  so an invisible or closing overlay cannot leave viewport and toolbar controls
  unclickable.
- `RenderContext` supports both windows and offscreen render textures, allowing
  deterministic screenshots of the complete world and editor surface.

## Acceptance evidence

`WorkbenchUIAcceptance` covers the generic plugin workflow end to end:
creation and placement, selection and multi-selection, move/rotate/non-uniform
scale handles, local/world transforms, grid-origin snapping, center/individual
pivots, preview metadata, registered debug geometry, undo/redo behavior, popup
input release, live simulation edits, and layouts at 640x800, 1100x800, and
1920x1080. Handle hit targets are also checked at 0.5x and 2x camera scale.

Stored visual evidence:

- [640x800 compact workspace](screenshots/workbench-640x800.png)
- [1100x800 standard workspace](screenshots/workbench-1100x800.png)
- [1920x1080 wide workspace](screenshots/workbench-1920x1080.png)
- [1100x800 context action panel](screenshots/workbench-1100x800-context.png)

Verification at completion:

- Complete Debug build: passed.
- `WorkbenchUIAcceptance`: passed.
- Full Debug CTest suite: **70/70 passed**.

## Shared foundation used

- `ProjectSession` and `SceneHistory` provide cancellable continuous edits,
  commit, undo, and redo.
- `CameraController2D` and `RenderContext` provide pointer-centered zoom,
  panning, viewport-local conversion, and offscreen acceptance rendering.
- `SceneComponentTypeDescriptor` supplies typed attachment metadata, while
  component property editor hints drive generic sensor previews.
- `RenderSubmissionQueue` lets scheduled project systems draw debug geometry
  without exposing SFML in the project registration contract.

The authoring metadata added during this phase remains shared foundation for
18B-18G. Completion of 18A does not change the overall Milestone 18 status;
the audited open work in later phases and 18H still blocks Milestone 19.

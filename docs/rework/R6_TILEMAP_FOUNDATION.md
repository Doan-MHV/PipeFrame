# R6 tilemap foundation and input boundary

September 12, 2026. Status: verified engine/asset foundation; editor workflow unfinished.

## Implemented engine API

All new environment APIs are in `engine/include/PipeFrame/Environment/` and compile
against Foundation alone:

- `Tilemap2D.h`: positive finite cell size/origin, bounded dimensions, named layers,
  tile definitions with tint/solidity, independent visibility/collision/lock flags,
  clipped local-space picking and a revision counter. Uses existing Grid2D storage.
  Maximum total layer cells is 4,194,304; maximum layers is 32. Bounds are not walls.
- `TilemapEdit.h`: EnvironmentBrush is the custom effect base, TileBrush is the built-in
  replacement/erase effect. Sample, Line, Rectangle, Circle and Fill share clipping,
  deterministic preview patches and one commit. Preview never mutates the source map.
  Line segments are clipped before rasterizing. Fill is bounded and handles no-op brushes.
  Commit rejects a map changed since begin. Cancel clears pending edits. TilemapPatch
  validates every expected old value before applying forward or backward for undo/redo;
  locked layers and duplicate patch cells are rejected before writes.
- `TilemapSerializer.h`: version-1 deterministic text format, definitions/cells/flags and
  local geometry round-trip. Rejects unsupported versions, undefined tiles, duplicate
  definitions, invalid dimensions, excessive allocations and truncated/trailing data.
- `TilemapGeometry.h`: plain-color triangles for visible layers within a supplied cell
  range, using the same definitions as IsSolid. Callers can cache by map revision.
  Existing RaycastGrid reads IsSolid; a regression proves a painted wall is hit.

A TilemapEdit borrows its map: the map must outlive the edit. Preview patches are data;
scene-history integration, asset ownership and gesture activation belong to the host.
Do not use a pending edit across map replacement. These services do not yet implement
rotated/scaled world-space picking, outside-origin ray entry, sensors, physics solver
integration, crop/resize history or texture-atlas UVs.

## Asset/editor integration implemented

AssetType::Tilemap was appended without changing existing numeric IDs. The existing
AssetDatabase imports `.pftilemap` through full TilemapSerializer validation, stable IDs,
cache/persistence and search. AssetBrowserPanel includes Tilemap (and Prefab) filters.
No second asset manager was created. The generic compiled cache still wraps the source;
this is not yet runtime asset resolution. Tilemap previews are still generic text artifacts.

## Input boundary implemented

WorkbenchInput.h now accepts InputEvent and PipeFrame vectors, with no native includes.
Selection/transform math and runtime/camera dispatch use neutral types. Workbench converts
native events at its application boundary. E, Z, F6/F7/F8 and Backspace were appended to
InputKey and mapped both ways; existing enum values/layout and plugin ABI 6 are retained.
The real header compiles in WorkbenchPresenterBoundaryCompile without native include paths.

WorkbenchInput.cpp still bridges to native WorkbenchView and uses legacy Input modifier
state. Native view/layout/widget isolation remains open; a neutral header is not a claim
that all native implementation dependencies are removed.

## Verification

Debug SimulationWorkbench/WorkbenchUITests/AssetDatabaseTests/TilemapTests builds passed.
Native-free presenter/input-header compile and dependency lint passed. Focused suite:
TilemapRegression, DockResizeRegression, AssetDatabaseRegression, RenderContextRegression,
WorkbenchUIAcceptance all passed. Evidence: `evidence/r6-tilemap-foundation/tests.txt`.
Tests cover custom checker brush on an independent robotics-named map, clipping, no-op
fill, non-mutating previews, stale commits, undo/redo, locks, serialization, real tilemap
asset import and malformed rejection. Geometry and ray checks are headless data checks;
no screenshot or full editor painting acceptance is claimed.

## Remaining before users can paint in the editor

- Playground ground component/recipe and standard Inspector integration are now implemented
  for generated projects (R6_PLAYGROUND_AUTHORING.md). Tilemap component assignment and
  resolution are implemented in R6_TILEMAP_COMPONENT.md; painting remains open.
- Create/load/save tilemap assets from the editor and resolve references into the scene.
- Declarative palette/layer/tool UI; pointer gesture capture, previews, Escape cancellation,
  one scene undo transaction per gesture, reload/project-switch teardown.
- Executable tool registration/lifecycle using the existing extension registry. The brush
  effect base implemented here is only one part of that contract.
- Materials/atlas regions, actual asset preview display, asynchronous import publication.
- Object-transform-aware rendering/picking/collision, resize/crop controls, ruler/eyedropper.
- Ant migration and blank-project end-to-end acceptance; performance gates remain open.

R6 is not complete. These are implemented building blocks and initial asset integration,
not a finished playground or tilemap editor.

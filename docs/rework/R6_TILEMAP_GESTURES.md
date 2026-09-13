# R6 tilemap pointer gestures

Date: 2026-09-12
Status: engine gesture controller verified; editor painting workflow remains open.

## Implemented

`engine/include/PipeFrame/Editor/TilemapPaintTool.h` derives from the existing
`EditorTool`. It consumes neutral `InputEvent` values and mapped cell coordinates.
It uses `TilemapEdit` for pencil, line, filled rectangle, outline and flood fill.
Erase uses `TileBrush(0)` through the same path. No native window, renderer or UI
types are exposed. The public header is included in the dependency lint guard.

A permitted left press inside the map captures a stroke. Captured moves and releases
are handled even when the host's begin hit-test is false. Pencil interpolates fast
moves, clips to the map and evaluates custom effects once per cell per stroke.
Rectangle/line previews replace previous endpoints. Flood fill computes once from
the initial seed, rather than repeating the flood on every move/release.

Preview never writes the document. Left release commits one patch and returns it
for history. No-op strokes produce no history entry. Escape, lost focus, pointer
leaving the native window, disabling or reconfiguring the tool cancel the preview.
A missing release coordinate cancels. A changed document revision invalidates the
whole gesture; locked layers and invalid custom-brush output fail without partial
writes. `LastError()` exposes failures to the host.

## Host contract and example

The mutable document must outlive the tool. Destroy it before replacing the map or
unloading the project/plugin. `Configure` owns a shared brush reference, so plugin
brush objects must also be released before unloading their implementation library.
The host must not mutate the asset module's const render cache to provide a document.

```cpp
TilemapPaintTool tool(editableMap);
tool.Configure(layerIndex, std::make_shared<TileBrush>(wallTile),
               TilemapPaintShape::Rectangle);
tool.SetEnabled(true);

// Host supplies inverse camera + entity transform coordinates. During capture,
// retain outside-map integer coordinates so shape clipping has correct endpoints.
auto result = tool.HandleEvent(event, mappedCell, viewportCanBeginStroke);
if (result.committed) {
    // Already applied: record one undo entry, mark dirty and refresh derived data.
    // Undo: patch.Apply(editableMap, false); redo: patch.Apply(editableMap).
}
// Draw tool.Preview() as an overlay; do not save preview cells.
```

Route captured events before other viewport tools. The controller's capture is a
routing contract, not an operating-system pointer capture call. `mayBegin` must be
false over widgets and outside the active Playground. It does not constrain later
shape cells to a smaller Playground: host/document bounds integration is still
required. Never retain this tool across replacing its referenced map.

## Verification

Debug `TilemapTests` built successfully. `TilemapRegression` and
`PipeFrameDependencyLint` passed **2/2** in 0.33 seconds.
Evidence: [test output](evidence/r6-tilemap-gestures/tests.txt).

Tests exercise denied/outside presses, interpolation, captured outside release,
one-patch undo/redo, duplicate release, no-op clicks, shrinking previews, cancellation,
external revision conflicts, locked layers, invalid effects, non-idempotent custom
brush retracing, line/outline/fill behavior. Existing map serialization, geometry,
clipping and collision-query tests run in the same regression executable.

## Still open

Workbench does not instantiate this tool yet. The Flutter-style palette, preview
overlay, camera/Playground mapping, history/document ownership, asset save/reimport,
shared versus make-unique editing and project-switch teardown must be wired together.
Custom brush settings/registry discovery, Ant live editing and circular pencil
footprints also remain open. No native interaction screenshot, performance result,
blank-project acceptance or R6 completion is claimed by these unit-level tests.

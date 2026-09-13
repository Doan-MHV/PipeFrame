# R6 package 3 — Built-in environment editing workflow

September 12, 2026. This package implements the built-in map editing workflow on the
public engine tool/document APIs and the existing declarative Workbench UI. It does
not close package 4's external tool generation/registration or custom Ant brushes.

## Using the editor

Select a Playground with a Tilemap asset, open **Assets → Maps**, and choose **Edit Map**.
The target name and editing/stopped status identify the active object. Editing an asset
without a selected instance uses its local coordinates. Changing the selected Playground
stops the previous instance's tool; choose Edit Map again for the new target. Dirty maps
must be saved before opening another map. Make Unique remains available for shared maps.

The built-in tools are:

- **Pencil**: continuous stroke with a circular radius of 0–64 cells. Radius zero paints
  one cell. Fast pointer movement interpolates the stroke instead of leaving gaps.
- **Erase**: switches the chosen painting shape to tile zero. It works with pencil radius,
  line, rectangle, circle and fill. Choosing a palette tile switches back to painting.
- **Line**, **Rectangle**, **Outline** and **Circle**: drag endpoints define a live preview;
  moving an endpoint replaces the preview. Circle's dragged distance defines its radius.
- **Fill**: floods connected matching cells from the pressed cell, bounded by the selected
  Playground. It does not spread outside its bounds or through different tile values.
- **Eyedropper**: selects the active layer's tile without creating an edit. It can inspect
  a locked or hidden layer.
- **Select**: drag a rectangular cell selection. **Paint Selection** and **Erase Selection**
  each create one undo command. Choosing another palette tile preserves the selection.
  Escape clears it. This is cell selection, not an object transform or clipboard tool.
- **Ruler**: measures between cell centers without changing the map. The measurement stays
  after release and reports world distance, including object scale, and a grid cell count.
  Escape clears it. Rotation does not change distance; nonuniform scale is accounted for.
- **Paint Boundary**: explicitly paints the perimeter of the Playground/map intersection
  with the selected tile. Bounds do not create implicit collision walls.

Cells always snap through the camera and object's inverse transform. **L** toggles
horizontal/vertical axis constraint (the first movement chooses the axis for that stroke); the same setting has an **Axis Lock** control.
**Escape** cancels a captured gesture without writing. Middle-button drag pans and the
wheel zooms. Focus loss cancels capture. Editor UI input takes precedence over new strokes;
release of a captured stroke still reaches its owner over a panel. Selecting another target,
changing layer settings or stopping editing cancels a pending gesture.

The viewport shows the brush footprint, committed selection and pending painting. The
map panel reports gesture endpoints and dimensions. Preview geometry recomposes all
visible layers at changed cells: erasing reveals lower layers, and painting a lower layer
does not incorrectly cover an unchanged upper layer. Geometry is batched for these cells.

## Layers and palette

Layers support **Add**, **Remove**, **Rename**, **Move Up/Down**, **Show/Hide**,
**Collision On/Off**, and **Lock/Unlock**. Later layers draw on top. Removing a layer can
be undone with all its data. At least one layer remains. Names must contain 1–80 characters
when renamed. Layer changes participate in the same map document history as painting.

Interactive painting rejects hidden and locked layers. Visibility and collision remain
independent: a hidden layer may still collide when its collision flag and tile solidity
are enabled. Low-level transactions may still edit hidden layers for custom tools;
visibility is not a substitute for an edit lock.

The palette shows color/atlas swatches and tile IDs, with wall labels for solid tiles.
It displays 32 tiles per page to avoid mounting an unbounded number of controls. Plain-color
maps can create additional color tiles. Tile tint and solidity are editable and undoable;
atlas tile IDs come from the assigned tileset. Material tint multiplies tile tint.

**Save Map**, **Undo Map** and **Redo Map** are directly available in Maps. Scene undo/redo
shortcuts route to the active map document while painting. Saved layer names/order/flags,
tile definitions and painted cells reopen exactly. Resize/resample and Make Unique continue
to use the package-2 implementation.

## Engine and editor structure

- `TilemapPaintTool` owns gesture state, pointer capture, brush radius, axis constraint,
  selection, ruler and hover state. It derives from public `EditorTool`.
- `TilemapEdit` and `EnvironmentBrush` own bounded cell effects and transactional patches.
- `TilemapAssetEditor` owns map history, selection commands, layer changes, tile metadata,
  savepoints and persistence. There is no separate Workbench undo implementation.
- `BuildTilemapPreviewGeometry` recomposes changed cells using the current layer stack.
- `ProjectSession` maps world input through Playground transforms, manages active target
  safety and submits backend-neutral viewport geometry.
- `AssetBrowserPanel` describes all controls with `View`, `ViewPanel` and shared controls.
  Native window/input/render details stay behind the existing backend boundary.

## Verification

- Full Debug suite: **73/73 passed**, 103.78 seconds.
- After the final axis-lock edge-case fix: **4/4 affected Debug checks passed**.
- Debug and Release all-target builds passed, including public header isolation checks.
- Native 1440×900 tool and 800×700 scrolled palette captures were inspected.

`EnvironmentEditingAcceptance` covers quick radius strokes, cancel/undo, bounded selection
fill and erase, explicit boundary painting, constrained lines, persistent ruler state,
layer create/rename/reorder/remove/flags, locked/hidden-layer protection, metadata undo,
save/reopen and layered preview composition.

`WorkbenchUIAcceptance` additionally exercises rotated, nonuniformly scaled Playgrounds at
two zoom levels, target selection changes, real declarative layer/history callbacks, and
UI regions preventing new map strokes. Existing map gesture, shared map, crop/resample,
asset, runtime and Ant regression tests are retained.

Normal and narrow Maps captures and final build/test logs are recorded in
[evidence/r6-environment-editing](evidence/r6-environment-editing).

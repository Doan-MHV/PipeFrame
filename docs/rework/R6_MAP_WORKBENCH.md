# R6 map authoring in Workbench

2026-09-12. Workbench integration implemented; R6 remains open.

## Available workflow

Open Assets → Maps. New Map writes/imports a valid .pftilemap, using a selected
Playground's dimensions when present (otherwise 32×24 cells of 10 local units).
Assignment attempts the selected object's compatible asset fields. Existing maps
can be selected and opened with Edit Map. The Maps page scrolls in short docks.

Pencil, line, rectangle, outline, circle, fill and erase use TilemapAssetEditor and
TilemapPaintTool. Palette buttons select tile IDs; layer buttons select an unlocked
layer. Eyedropper samples a tile without changing history; ruler shows local distance
while dragging and draws a viewport line. Circle is a drag-radius disk, not a
radius-setting freehand stamp brush. Generic custom brush effects still use the
existing EnvironmentBrush base.

Viewport mapping uses inverse camera/object transform. An assigned selected object
supplies the map transform; unassigned asset editing uses local origin. Preview
renders the private authoring map and pending stroke, with committed geometry cached
by map revision. It does not mutate the runtime's const asset cache. The source is
not written during drag. Save publishes/reimports the same asset ID. Toolbar Undo/Redo
route to map history while painting is enabled; Stop Paint restores scene history.
The toolbar includes map dirtiness. Project switching rejects dirty maps/active strokes.

Starting play or stepping disables painting. External target deletion, visibility/lock,
transform or reference changes stop painting and retain the document's unsaved data.
Build/reload disables painting. These actions do not implicitly save a map: save then
reset the simulation to use its changed source. New Map rejects outstanding dirty work.

AssetBrowser now also displays a selected texture's decoded preview via the shared
neutral View Mesh and GraphicsResourceService. The preview is cached by path/revision,
uses the generated bounded preview PNG, preserves aspect ratio and reports preview
(not source) dimensions. Unsupported material/tileset image previews remain open.

## Validation

- Full configured Debug build passed before the final tool additions.
- Full engine/editor/Ant suite passed 71/71 in 98.70 seconds at that checkpoint.
- Later focused UI/tilemap/asset/lint runs cover final tool/preview changes; see evidence.
- Native workflow test creates a map, paints through camera mapping, undoes/redoes,
  saves/reimports and reopens exact tile contents. Existing assignment/drag checks pass.
- Tool tests cover circle preview, read-only eyedropper/ruler and existing gesture cases.
- A 1100×800 Maps snapshot was inspected: panels stay within the dock and scroll.
  Initial snapshot runs left the Maps/layout state active and broke later picker
  assertions in the same harness. The snapshot branch now restores the workspace
  before continuing integration checks; failed capture runs are not passing evidence.

Evidence directory: [r6-map-workbench](evidence/r6-map-workbench).

The exploratory Release scale run reported 1/3/5-colony p95 of 5.15/11.13/17.09 ms.
It overlapped with the regression suite and is not an isolated performance gate.
The previous 16.67 ms target remains open; no population/timestep changes were made.

## Remaining R6 requirements — not completed by this change

- Complete native Widget/window/public authoring-header isolation.
- Validated materials/tilesets, atlas/UV/textured ground authoring, asynchronous imports.
- Explicit compatible assignment UI; new standalone map creation does not itself create
  a scene entity. Users still need a Playground for saved runtime placement.
- Resize/crop/resolution history, make-unique, bounds clipping for a differently sized
  existing Playground, selection tools and a circular freehand footprint.
- Typed custom tool settings, templates/plugin registration, custom scalar data brushes.
- Shared transformed tile/box/segment movement collision, ray/overlap/clearance queries.
- Ant typed environment components and food-density tools with authoring acceptance.
- Isolated repeated performance checks and physical interaction verification.
- Full blank-project→author→build→run→save/reopen→reload acceptance/documentation.

This integration is not the completion of all eight R6 packages.

Final focused suite: 4/4 passed in 5.24 seconds (UI, tilemap, asset database, lint).
The corrected narrow snapshot invocation also passed all Workbench UI checks.
[Inspected 800×700 Maps page](evidence/r6-map-workbench/maps-800.png).
At this width the existing responsive workspace moves tools below the viewport;
Maps remains scrollable, with painting controls further down the page.


## Follow-up: bounded painting

The editor now passes selected Playground bounds into the shared map transaction. Fill,
line, rectangle, circle and pencil effects cannot change cells completely outside those
bounds. Partial cells are paintable and their preview/committed overlay is clipped exactly
to the ground. Starting outside the ground does not capture input. Bounds changes cancel
an active gesture. This preserves the full underlying asset; it is not destructive cropping
or make-unique. Ruler endpoints remain unrestricted while dragging.

Ant now exposes Environment Map as an `asset:Tilemap` field in
SimulationSettingsComponent::Schema. Select Simulation Settings and assign an existing
compatible map from Assets / Maps; saving a painted map reimports it, and Reset loads the
new revision. Ant still requires cell size 1 and origin (0,0). The general New Map defaults
are not an Ant-specific template. Custom food brushes and the full Ant authoring gate remain open.


## Shared maps: Make Unique

Select one Playground whose Tilemap component references the open map. Save pending
map edits, then use **Assets → Maps → Make Unique**. The engine writes an independently
indexed copy in Assets/Tilemaps; the editor assigns it only to that selected object and
opens the copy for painting. Other objects retain their original reference and map data.
The button is unavailable for multiple selections, locked/hidden objects, unfinished
strokes, unsaved maps, or a selection that does not reference the open map.

Use **Stop Paint**, then scene Undo/Redo to undo/redo the reference assignment. In paint
mode Undo/Redo continues to operate on brush transactions. Undoing the assignment keeps
the copied asset in the browser, so subsequent redo and any other references stay valid.
If assignment fails, a successfully imported copy remains available as an unassigned asset.

This operation currently targets the standard Tilemap component. Ant's Simulation Settings
map field can use a copied asset through normal assignment, but does not yet participate
in this Playground-specific Make Unique action. Resize/crop and resolution changes remain open.

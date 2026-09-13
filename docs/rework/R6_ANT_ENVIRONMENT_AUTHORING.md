# R6 package 6 — Ant editor-authored environments

Status: **Complete** — September 12, 2026.

Ant now uses the shared Playground and Tilemap authoring components.
Validation results are recorded below. Package 7 performance acceptance and package 8
learning/closeout remain separate.

## Create or edit an Ant environment

1. Open Ant and select **PLAYGROUND** in the hierarchy. The shipped scene assigns
   `Assets/Tilemaps/Main.pftilemap` through its **Tilemap Asset** property.
2. Open that tilemap in the editor's Maps tools. Use the standard pencil, line,
   rectangle, fill and other gestures. Terrain solidity controls walls.
3. In **Assets → Maps → EDIT MAP**, choose **Ant / Ant Food Density**.
   Set **Food per cell**, leave **Erase food** off, and paint walkable cells with
   Pencil, Line or Rectangle. **Pencil / eraser radius (cells)** controls pencil width.
   To remove food, enable the brush's **Erase food** setting. Its schema owns quantity
   validation and erase behavior; shared transactions provide cancel and undo/redo.
   The pencil/eraser footprint stays visible while holding and dragging left-click
   (fixed September 13), including the pending-stroke preview.
4. Save the map. The next render/update or Play resolves the saved asset revision
   and rebuilds Ant's derived environment. It restores authored colony reserves and
   food and restarts simulation time, preserving the current play/pause mode.
5. Save the scene to preserve Playground assignment, colony and Food Source objects.
   Reopening or resetting restores that authored starting state.

For a new map, create a Playground from the hierarchy's type picker. Ant defaults
its cell size to **1**. Create/assign a tilemap using the shared asset tools with the
same rows, columns and cell size. An unassigned Playground is a valid empty bounded
Ant world, so it does not require a PNG. Add a colony and optionally Food Source
objects; the custom brush provides per-cell food independently of those objects.

Ant supports **one Playground**, at origin, with unit scale and zero rotation.
Dimensions must match the assigned tilemap and exceed four cells in each direction.
The two-cell physics border remains reserved. Invalid assignments/transforms report
an error and retain the previous simulation; they are not silently rescaled.
Other PipeFrame projects retain the generic transformed Playground capabilities.

## Engine reuse and domain boundaries

- Ant registers the engine's `PlaygroundComponent` and `TilemapComponent` with its
  ordinary entity/component registries. The inspector and asset assignment use the
  same schemas as generated projects. The hierarchy offers Playground alongside
  Colony, Food Source and Beacon.
- Ground color and material use shared `DrawPlayground` and `VisualAssetModule`.
  The ground grid setting controls Ant’s existing cached grid presentation.
  The Tilemap visibility setting hides Ant's terrain/food presentation without changing
  collision or food state. Colony/pheromone simulation and Pezzza-style wall/ant
  presentation remain Ant's domain rendering.
- Playground picking uses the shared transformed-ground helper; its selection outline
  is rectangular. Ant's accepted AntWorld/Physics/Rendering/Runtime organization stays.
- Maps load through `TilemapAssetModule` and the existing `WorldMapLoader`. The latter
  derives mutable Ant cells, food entities and marker sampling coefficients from
  authored data. Ant does not serialize its runtime grid as another environment file.
- Wall contact uses package 5's shared grid sweep/slide. Colony lifecycle, foraging,
  food consumption and pheromones stay in Ant. One tile cell remains one Ant world unit.
- **Live food:** open Ant's left **FOOD / TOOLS** tab, select **Add Food**, set
  the radius and **Live food per cell**, then **right-drag** in the world. This works
  while playing or paused, including scenes with an authored Playground. Food is
  added immediately without saving, rebuilding the world, or resetting time.
  **SELECT** restores right-click ant selection. Reset discards these live edits.
- Use **Assets / Maps** and the Ant Food Density brush for saved starting food.
  Starting a live runtime stroke stops the map painting overlay so it cannot hide
  live changes; the unsaved map document is retained. Scenes without a Playground may use a typed Environment Map asset; no PNG or filename fallback remains.
- The map Pencil/eraser footprint draws a solid yellow circular outline above the
  paint preview, including while the left button is held. During a captured stroke
  its position comes from the stroke endpoint rather than transient hover state.

Map saves apply at a frame/update boundary, not on every mouse move. Unsaved strokes
remain an editor preview. A valid reimport rebuilds once per revision. An invalid
reimport leaves the previous world intact and reports through the project log; a later
valid revision can recover. This avoids resetting the world repeatedly during a drag.

## Shipped scene and portable assets

`Scenes/Main.pfscene` now contains four objects: Colony, Food Source, Simulation
Settings and Playground. Its map reference is a typed `AssetReference`, not a filename
on Simulation Settings. The optional Environment Map setting is a typed tilemap asset; a Playground takes precedence.

Source-controlled starter scenes may refer to an imported asset as
`source:Assets/Tilemaps/Main.pftilemap`. The engine resolves this through the project's
asset database, without opening arbitrary files. The Workbench normalizes it to that
project's stable asset ID when migrating the scene to components. Subsequent saves
use the ID, so normal asset moves retain the link. The source alias is bootstrap data,
not a replacement for persistent project IDs.

The shipped map stores terrain tiles and `ant.food-density` quantities only. The
legacy visual Food layer and `Assets/Maps/Main.png` have been removed. The brush
starts missing density data at zero; there is no image conversion or implicit food
quantity. Runtime loading tests compare every cell against authored collision and
food density, and verify that erased food stays absent after save/reload.

## Acceptance

`AntEnvironmentAuthoringTests` creates a fresh asset database and a 32×24 map, uses
`TilemapAssetEditor` for a wall rectangle and `AntFoodBrush` for a food line, then
runs Ant. It verifies undo/redo, editor scene serialization/reopen, runtime reload,
reset, rejected-transform rollback and recovery from invalid saved revisions. Ten initial ants spawn over ten ticks, retaining
Ant's existing colony lifecycle behavior.

`WorkbenchUITests` opens the actual Ant project, verifies that its source reference
becomes a canonical asset ID, selects Playground and opens the assigned map in the
shared viewport editor. Frame Selection uses the full ground bounds, and invalid
Ant transforms are contained and logged by the editor host. Native evidence includes
the authored Playground.

No new SFML dependency is introduced into Ant Source. Build/test evidence is stored
under `evidence/r6-ant-environment-authoring/`. These checks do not claim a new
five-colony frame-time result or full Pezzza visual parity.

Verified on local macOS arm64, `PIPEFRAME_ANT_REWORK_ONLY=ON`:

- Debug and Release builds passed, including the backend-isolated consumer targets.
- Full Debug suite: **76/76**, 107.85 seconds.
- After native framing/error-containment fixes: **8/8** affected Debug checks,
  2.87 seconds, plus the native Workbench evidence run.
- Release: **9/9**, 9.78 seconds, including Workbench UI, authoring, asset database,
  source conformance, PNG parity, rendering, behavior parity and Ant stress.
- All **105 Ant Source** C++ files scanned: no `SFML` or `sf::` matches.

[Native authored Playground capture](evidence/r6-ant-environment-authoring/ant-authored-playground.png)
was inspected; Frame Selection shows the entire map within its rectangular bounds.

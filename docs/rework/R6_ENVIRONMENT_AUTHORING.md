# R6 environment authoring — required before robotics

Current status (September 13): **all eight R6 packages complete and verified**.
The collision workspace improvement closes the final five-colony gate in three matched
runs. See [final tests and performance evidence](R6_FINAL_PERFORMANCE.md) and
[blank-project learning acceptance](R6_BLANK_PROJECT_ACCEPTANCE.md).
The requirements below record the original September 12 gap and the accepted scope,
not the current implementation status of each subsystem.

Execution order and overall R6 status: [R6_EXECUTION_PLAN.md](R6_EXECUTION_PLAN.md).
This document supplies the detailed environment contract for that checklist.

## Original observed gap and existing foundations (September 12)

A scene/object editor is insufficient if a new project cannot create its own world.
Ant now loads a converted tilemap by default (R6_ANT_TILEMAP_MIGRATION.md), but still
uses its project-owned AntEnvironment, map initialization and wall/food
brushes. These are not a general-purpose tilemap/environment authoring workflow.

Reuse existing engine facilities rather than starting another framework:

- Resources/GraphicsResourceService.h: neutral texture/resource handles.
- Project/AssetDatabase.h: asset identity and categories, including Texture and Material.
  A material category alone does not constitute a working material editor/runtime.
- Spatial/UniformSpatialIndex.h and Spatial/GridRaycast.h: grid indexing and grid rays.
  These do not by themselves provide a complete scene collider query service.
- Environment/ScalarField2D.h: scalar fields, useful for overlays rather than a
  replacement for an authored tilemap document.
- Current scene serialization, component-owned schemas, undo/redo, prefabs and
  declarative View/StatefulView UI.

## User workflow and scope

New project → New Scene → Create Ground or Tilemap → import texture/atlas or use
plain colors → create material/tileset → paint floors/walls → place obstacles,
spawn points and goals → Play → save/reopen.

The first delivery is 2D. A Ground object is a resizable textured rectangle; a
Tilemap has cells and layers. Neither requires an external PNG map or custom engine
code. True 3D planes, heightfields, ramps and slope physics require a separately
implemented 3D/elevation contract; do not label a flat tilemap as 3D terrain support.

Support plain-color environments with no texture assets. Missing textures should
show a diagnostic/fallback and must not silently remove physical walls.

## Bounded playground workflow

Required refinement, September 12: Create Playground supplies a scene object with
Transform, editable rectangular bounds and a Tilemap. Users can also select an existing
rectangle and add the environment/tilemap components. A rectangle renderer alone does
not become an environment through an undocumented flag. The creation recipe assembles
the same registered components exposed to developers and the Inspector.

Set world-space width/height, origin and cell size; display the resulting row/column
counts. Validate positive finite dimensions and define cell alignment. Painting is
clipped to the selected playground and unlocked active layer. Multiple playgrounds
are supported; selection determines the target. Bounds do not implicitly create walls:
provide an explicit boundary-wall command or boundary policy.

Keep object resize and map-resolution changes distinct. Resizing bounds preserves
existing cell coordinates, previews any cropped content and supports undo. Changing
cell size changes scale unless an explicit resampling operation is chosen; never
silently discard or reinterpret painted data. Tile painting works in local coordinates
through the object's transform, with consistent picking, collision and sensor queries.

Provide pencil/circular brush, eraser, straight-line tool, outlined/filled rectangle,
bounded flood fill, selection and eyedropper. Click-drag displays a live preview and
commits one command on release; Escape cancels. Straight lines interpolate cells so
fast dragging leaves no gaps. A ruler measures world distance and cell count without
modifying the map; line painting is a separate action. Show brush footprint, endpoints,
rectangle dimensions and active target/layer. Support constrained directions and
snapping through documented shortcuts; UI focus always wins over world painting.

## Tile editor UI uses the current declarative API

Required: tile palette, layers, brush/material settings, dialogs and toolbars use the
same View/StatefulView and neutral ViewPanel API as the migrated Workbench presenters.
Custom brush settings use schema-generated controls or these same public views. Engine
viewport drawing handles previews; engine tool services handle gestures and transactions.
Do not build a separate tile-editor widget library or require native fonts/widgets in
project tools. See [the actual presenter migration](R6_PANEL_PRESENTERS.md).

## Developer brush and tool extension contract

Package 4 is implemented: [custom brush API, Ant integration and evidence](R6_CUSTOM_BRUSHES.md).

Provide public, backend-neutral extension bases for executable editor tools, with
engine-managed activation, pointer capture, preview, begin/update/end/cancel lifecycle
and teardown on project switch/reload. The implemented roles are `TilemapPaintTool`,
`EnvironmentBrush`, `BrushTool` and `SchemaBrush<T>`. Factories use the existing
plugin extension registry, with `RegisterBrush<T>` providing typed registration.

Separate stroke geometry from the operation performed on it. Standard freehand, line,
rectangle and fill tools produce cell/world samples; a brush supplies the effect.
For example, an AntFoodBrush can use either a circular stroke or a dragged rectangle
without rewriting picking, interpolation, clipping or undo logic.

Tool registration declares stable ID, label/icon, category, compatible targets/layers,
edit/play policy and typed settings. Tool-owned settings use the existing schema and
validation convention; the editor builds ordinary controls, and optional custom UI uses
the same public declarative View API. Provide Create Tool/Brush templates in the
project's Source/Editor area and automatic registration through build/reload.

The engine supplies an edit context with the selected target, coordinate conversion,
layer/bounds constraints, seeded randomness when needed, previews and validated edit
commands. A brush returns operations through this context; it must not mutate a hidden
second map, write backend vertices directly or implement its own undo stack. Domain
operations such as food density require a registered editable data layer/command and
serialization support, not a forced conversion into visual tiles. Engine transaction
handling captures before/after values, validates and groups the whole gesture, applies
runtime changes at safe boundaries, and invalidates only affected caches.

Use this API for built-in brushes and Ant's custom wall/food brushes. Ant-specific
pheromone injection, if retained as a simulation tool, declares its runtime-only policy
and must not silently become a persisted authored edit. Tool errors, target deletion,
focus loss, Escape and plugin reload must cancel or safely finish the active gesture
without partial changes or dangling callbacks.

Acceptance: a developer generates and registers a custom Ant food brush, exposes a
validated amount setting, uses it with freehand and rectangle gestures, undoes/redoes,
saves/reopens, and reloads the plugin without modifying Workbench source. A non-Ant
custom brush uses the same API to prove it is generic. A type descriptor or unused base
class alone does not satisfy this requirement.

## One authored source of truth

A scene stores environment object/component references and stable asset IDs. A
versioned tilemap asset stores origin, cell dimensions, bounds/chunks, layer order,
tile IDs and overrides. A tileset maps tile IDs to atlas regions and defaults.

Separate the following properties:

- Visual material: texture/atlas region, tint, UV scale/repeat/filtering and ordering.
- Collision/surface data: occupied shape, collision layer/mask, friction where supported,
  and typed surface properties for project-defined sensor interpretation.
- Domain state: Ant food and pheromones or robot sensor noise and scan timing.

A painted wall is authored data. Rendering meshes, collision geometry and spatial
indexes are derived caches, with stable source identity for picking/debugging. They
must update together when an edit commits. Sensors query that same geometry; do not
maintain an unrelated robot obstacle map or infer collision from displayed RGB values.
A visible surface may deliberately be non-colliding, and this must be explicit.

A sensor's generated occupancy map is an observation, not privileged access to the
authored truth. Exit-search controllers should consume their configured sensors and
knowledge; do not silently give them hidden wall coordinates.

Proposed reusable roles (names are design targets, not existing APIs): TilemapComponent,
GroundRendererComponent, TilemapRendererComponent, TilemapColliderComponent and a
scene environment-query service. Use the current schema/registry and World module
contracts; no second scene owner, Ant-only editor branch or parallel metadata system.
Do not add bases for inert values; module/system implementations use the established
engine lifecycle. Rendering, physics and runtime modules consume the same asset data.

## Delivery slices

### R6-E1 — Authored assets and runtime binding

Implemented: [package-2 workflow and verification](R6_VISUAL_ASSETS.md).
The following scope is retained for traceability.

Implement versioned tileset/tilemap/visual-material data, schemas and validators;
empty and colored defaults; stable references; save/load and missing-asset handling.
Register Ground/Tilemap creation in a generated blank project with no manual registry
or CMake edits. Expose transform, dimensions, layers and assets in the Inspector.
Define units/origin and conversions explicitly: preserve Ant's existing scale; robot
projects declare their physical units rather than assuming one pixel equals one meter.

Gate: create an empty map and textured ground, configure them, save/reopen and render
through the public engine API without Ant or SFML authoring dependencies.

### R6-E2 — Shared environment editing tools and developer extensions

The **built-in workflow portion is complete**: [package 3 and evidence](R6_ENVIRONMENT_EDITING.md).
External brush authoring and custom Ant food-density integration are complete in
[package 4](R6_CUSTOM_BRUSHES.md). Remaining object-role authoring and full Ant workflow
acceptance belong to packages 5–6; this combined section is not wholly complete.

Implement the bounded playground and developer extension contracts above.
Add a tileset/material palette, paint/erase, line drawing, ruler, rectangle fill, bounded flood fill,
eyedropper, brush-size preview, tile selection and layer visibility/locking/order.
Support tile snapping and transform-aware world-to-cell mapping. Commit a complete
brush stroke as one undo transaction; cancellation rolls it back. Dirty assets prompt
for save. Editing an asset shared by multiple instances must be explicit; provide a
make-unique path rather than surprising edits to every instance.

Place/transform/duplicate ground, static obstacles, spawn markers and goal/trigger
regions. Use the same declarative UI, input capture and command system as the editor.
Document edit/play policy: authored edits persist; runtime changes do not overwrite
assets on reset. If live editing is supported, apply changes at a fixed-step boundary.

Gate: mouse and keyboard interactions work through zoom/pan, short docks and scrolling;
no brush paints through panels; undo/redo and save/reopen restore exact authored data.

### R6-E3 — Geometry, collision and queries

Implemented in package 5: [shared collision API, supported motion and evidence](R6_ENVIRONMENT_COLLISION.md).

Build changed-chunk rendering and collision from authored data. Start with supported
2D cell/box/segment shapes; extend actual solver/query support for selected shapes,
not just shape metadata. Include raycast, overlap and clearance/shape query semantics,
layer masks, world-unit distances, hit normals and source object/cell identity.
Handle static maps and independently moving obstacle objects consistently. Test
inside-start rays, boundaries, corners, zero direction and max-range misses.

Ground appearance must not imply unsupported friction or elevation physics. Surface
properties become effective only when the corresponding physics/sensor consumer is
implemented and verified.

Gate: a non-Ant moving-body/query fixture collides with and detects an authored maze,
including after wall painting, obstacle movement, undo and reload. Visual, contact and
query boundaries agree within documented tolerances. The fixture proves reuse; it is
not the Milestone 19 ultrasonic/LiDAR model or autonomy implementation.

### R6-E4 — Ant migration and optional PNG import

Implemented in package 6: [Ant authored-environment workflow and evidence](R6_ANT_ENVIRONMENT_AUTHORING.md).

Make Ant consume the shared authored walls/ground and shared painting workflow while
keeping colony, food, pheromone diffusion and ant collision policy in its domain code.
Register Ant custom brushes through the public tool API; reuse gesture geometry, preview,
settings UI and transactions instead of maintaining another project-local painting loop.
Convert existing PNG maps through an explicit importer with a documented color-to-tile
mapping and world-scale conversion. Preserve source-map import as a convenience and
parity baseline. Author/edit/save a new Ant environment without generating a PNG.
Do not change wall sampling or foraging behavior accidentally during migration.

Gate: editor-painted Ant walls affect real ants; save/reopen/reset retain authored
walls; existing map import matches wall occupancy and relevant deterministic/parity
fixtures. Remove superseded project-local generic painting/storage after verification.

### R6-E5 — Blank-project acceptance, documentation and performance

Using only the editor and public library: create a project, author a floor and maze,
place an obstacle, spawn and goal; attach the shared test mover/query behaviour;
run/pause/reset, undo/redo, save/reopen, build/reload. No external map generation,
hardcoded environment coordinates or engine source changes. Save screenshots and
interaction evidence plus a tutorial covering the actual implemented APIs.

Measure paused painting, dragging and scrolling, large-map loading and dirty-chunk
updates. Rebuild only affected geometry; reuse allocations and batch rendering;
profile before parallelizing. No per-cell entity is required for a dense tilemap.
Declare fixture sizes, hardware/build, memory and median/p95 times before acceptance.
Repeat the existing matched 1/3/5-colony fixture to protect Ant performance.

## Ordering and completion

Keep three explicit open tracks: R6 backend/UI isolation; R6 performance (latest
five-colony p95 17.00 ms versus 16.67 ms target); and R6-E environment authoring.
E1 precedes E2/E3 integration, then E4 and E5. Implement neutral environment contracts
from the start while finishing the existing UI boundary. R7 audits all three tracks.
Do not declare R6 complete because source generation/build/reload already passes.

Milestone 19 consumes these tools to author robot arenas and adds sensor models,
controllers, parts and autonomy. Its environment stage provides scenarios/templates
and domain features; it must not invent the basic tilemap editor for the first time.

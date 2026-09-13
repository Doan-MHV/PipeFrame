# R6 execution plan — engine, editor and environment authoring

Status: **complete / verified September 13, 2026**. All eight packages are complete;
the final performance run passes all three repetitions with five-colony p95
16.356 / 16.185 / 16.491 ms. [Final evidence](R6_FINAL_PERFORMANCE.md). This is the authoritative remaining-work
checklist for R6 under ENGINE_REWORK_ANT_PLAN.md. It incorporates R6-E without renumbering
R7 or starting Milestone 19. Plan completion is not implementation completion.

## Outcome

From an empty project, a developer can create a bounded 2D playground, import or create
visual assets, paint an environment, place objects, attach logic, build/run, and save/reopen.
The same engine services and public tool API support Ant and a small independent fixture.
Ant uses custom brushes without its own picking, painting or undo framework.

Package 1 is **complete**: [public UI/editor isolation and evidence](R6_PUBLIC_UI_ISOLATION.md).
Historical package-1 slices: [six neutral panel presenters](R6_PANEL_PRESENTERS.md),
following [dashboard/controller isolation](R6_UI_BOUNDARY.md), plus
[reusable dock interaction and native host separation](R6_DOCK_ISOLATION.md) and
[floating-window interaction](R6_FLOATING_WINDOW_ISOLATION.md).
Current environment/input slice: [tilemap foundation and input boundary](R6_TILEMAP_FOUNDATION.md).
Current scene-authoring slice: [built-in Playground ground object](R6_PLAYGROUND_AUTHORING.md).
Current Ant migration: [editor-authored Playground workflow and acceptance](R6_ANT_ENVIRONMENT_AUTHORING.md).
Historical source-format checkpoint: [default tilemap asset and parity](R6_ANT_TILEMAP_MIGRATION.md).
Current asset/component slice: [Tilemap assignment and rendering](R6_TILEMAP_COMPONENT.md).
Current bounds slice: [Playground render clipping](R6_PLAYGROUND_BOUNDS.md).
Current gesture slice: [neutral tilemap painting controller](R6_TILEMAP_GESTURES.md).
Current asset discovery fix: [automatic project indexing](R6_ASSET_DISCOVERY.md).
Current document slice: [tilemap asset editing and persistence](R6_TILEMAP_DOCUMENT.md).
Current Workbench authoring: [map tools and texture previews](R6_MAP_WORKBENCH.md).
Current query slice: [transformed tilemap rays and overlaps](R6_TILEMAP_QUERIES.md).
Current scene integration: [automatic environment queries](R6_SCENE_ENVIRONMENT_QUERIES.md).
Current movement workflow: [generated-project kinematic collision](R6_KINEMATIC_MOVEMENT.md).
Latest integration checkpoint (September 12): Ant Simulation Settings exposes a typed
**Environment Map** asset reference through its component schema. The runtime resolves
it through `TilemapAssetModule`; saved/reimported maps are loaded on Reset. The legacy
filename remains a fallback only when no asset is assigned. Existing Ant cell-size/origin
validation is retained. Custom Ant brushes are now implemented in package 4; the
full Ant new-map workflow remains part of package 6.

Playground painting now clips transactions and previews to the selected Playground's
local bounds, including maps larger than their Playground. Partial edge cells follow the
same intersection semantics as rendering/queries. Changing bounds cancels an active stroke;
underlying cells outside the Playground are retained for shared-map use. Source restoration
also recovers the runtime tilemap cache without requiring a revision change.

Verification for this checkpoint: [focused test evidence](evidence/r6-ant-map-bounds/tests.txt).

Shared-map follow-up: **Make Unique** copies the open saved map and assigns it to one
selected standard Tilemap object. Other instances keep their map; scene undo/redo restores
the binding (after Stop Paint). Asset copying and native shared-reference tests cover this
workflow. Resize/crop/resolution changes are now implemented with preview and undo; see
[R6_VISUAL_ASSETS.md](R6_VISUAL_ASSETS.md).

Current plugin ABI: **12**; earlier ABI/test reports are historical checkpoints.

## Verified work to retain

- Entity/component/behaviour generation and registration, exposed component-owned schemas,
  attachment, validation, scene/prefab persistence, and configure/build/cancel/reload.
- Neutral Camera2D/RenderContext through RenderSurface; plugin ABI 4 with stale-ABI rejection.
- Ant Source has no direct SFML/backend references; the plugin and render public headers
  compile without native include paths.
- Last recorded Debug/Release builds passed; 69/69 tests passed. This is a checkpoint,
  not evidence for the environment features below.

Evidence: R6_BUILD_WORKFLOW.md, R6_RENDER_BOUNDARY.md and their saved test artifacts.
Do not rebuild these systems from scratch. Extend them and retain their failure recovery.

## Eight remaining work packages

| Order | Work package | Existing scope | Status |
| --- | --- | --- | --- |
| 1 | Finish public UI and editor backend isolation | R6 boundary | Complete: all public UI/editor headers isolated, native hosts opt in, 71/71 tests and Debug/Release builds pass |
| 2 | Playground, tileset, tilemap and material assets | R6-E1 | Complete: validated material/tileset editors, textured runtime binding, asynchronous imports and resize/resample undo; see R6_VISUAL_ASSETS.md |
| 3 | Built-in environment editing workflow | R6-E2 | Complete: selection, radius/axis tools, ruler, layers, visual palette, boundary painting and workflow acceptance; see R6_ENVIRONMENT_EDITING.md |
| 4 | Developer tools and custom Ant brushes | R6-E2/E4 | Complete: plugin brush factories, schema settings, source generation/build/reload, numeric data layers and Ant food-density consumption; see R6_CUSTOM_BRUSHES.md |
| 5 | Shared environment collision and queries | R6-E3 | **Complete** — shared tile/box/segment queries, translating obstacles, Ant wall sweep/slide and render chunks; [API and evidence](R6_ENVIRONMENT_COLLISION.md) |
| 6 | Migrate Ant to editor-authored environments | R6-E4 | **Complete** — shared Playground/Tilemap authoring, ground materials, paint/undo/save/reopen/reset, native framing and parity; [workflow and evidence](R6_ANT_ENVIRONMENT_AUTHORING.md) |
| 7 | Meet measured editor/simulation performance gates | R6 performance/E5 | **Complete** — reusable collision workspace, three matched runs pass, 77/77 Debug/Release; [final evidence](R6_FINAL_PERFORMANCE.md) |
| 8 | Blank-project acceptance and learning documentation | R6-E5 | **Complete** — compiled tutorial, retained blank project, integrated acceptance, native captures and 77/77 Debug/Release checks; [report](R6_BLANK_PROJECT_ACCEPTANCE.md) |

These are implementation packages, not time estimates. Detailed environment semantics
are specified in [R6_ENVIRONMENT_AUTHORING.md](R6_ENVIRONMENT_AUTHORING.md).

### 1. Finish public UI and editor backend isolation

**Complete September 12, 2026.** [Implementation, migration and verification](R6_PUBLIC_UI_ISOLATION.md).
The following is the satisfied scope, retained for traceability.

Audit both direct and transitive dependencies in SimulationDashboard, Widget/ViewPanel,
CameraController2D and the recorded Workbench Editor headers. Replace project-facing
native font, event, vector, target and widget-host contracts with existing PipeFrame
resource handles, input, math, render context and declarative UI interfaces. Keep native
window/input/render implementations behind engine adapters. Move implementation behind
private boundaries as needed; typedef aliases and forwarding headers that still expose
SFML are not isolation.

Preserve input capture, popup ordering, focus, Inspector scrolling and screen/world
coordinate mapping. Migrate consumers with each API change; no broken intermediate
consumer is accepted. Protect/revise plugin ABI if a binary contract changes again.

Gate: affected public authoring headers compile without backend include paths. Engine,
Workbench and Ant interactions pass; saved normal/narrow-layout images are inspected.
The existing 11-header inventory is a starting list, not the definition of all leaks.

### 2. Playground and reusable assets

**Complete September 12, 2026.** [Implementation, workflow and evidence](R6_VISUAL_ASSETS.md).
Material/tileset schemas and editors, atlas previews, typed property assignment,
full texture caches and runtime resolution, asynchronous import/reimport with cancellation,
and resize/resample preview/undo are implemented. The scope below is retained for traceability.

Create Playground assembles registered Transform, bounds and tilemap/render components;
an existing rectangle can acquire these components. Expose width/height, origin, cell
size and layers through component-owned schemas. Bounds are not implicit border walls.

Implement versioned tilemap/tileset/material assets with stable references. Support color
fills and textured ground, atlas regions and UV scale. Define resize versus resolution
changes, cropping preview/undo and make-unique for shared assets. Start with 2D only.

Gate: a generated blank project creates, edits, renders, saves and reopens a playground
without an imported PNG, Ant code or manual registration/CMake changes.

### 3. Built-in environment editing

**Complete September 12, 2026.** [Implementation, workflow and evidence](R6_ENVIRONMENT_EDITING.md).
The built-in tools, selection/ruler, layer management, visual palette, bounded previews,
undo/save/reopen and native transformed-viewport acceptance are implemented. Package 4's
external brush generation and custom Ant integration are also complete; see
[R6_CUSTOM_BRUSHES.md](R6_CUSTOM_BRUSHES.md). Scope retained below.

Implement palette, pencil/circular brush, erase, line, outlined/filled rectangle, bounded
flood fill, eyedropper, selection and ruler. Use transform-aware mapping, cell snapping,
active-layer selection/locking and clipping to the selected playground. Show previews
and dimensions. A drag is one undo transaction; Escape cancels. Ruler does not paint.

Build these tools on the public tool lifecycle from the start (package 4 completes its
generation and external-project validation). No temporary Workbench-only brush system.

Gate: painting through pan/zoom, quick drags, layer changes, short docks and multiple
playgrounds behaves correctly; panels capture input; undo/redo/save restore exact data.

### 4. Developer tool/brush API

**Complete September 12, 2026.** [API, workflow, limitations and evidence](R6_CUSTOM_BRUSHES.md).
The generated Brush template supplies the effect/settings part of an environment tool;
engine-owned gesture tools supply its executable input lifecycle. Original scope retained below.

Provide executable tool bases with activation, preview, begin/update/end/cancel and
teardown hooks. Reuse plugin registration. Separate gesture geometry from brush effects:
freehand and rectangle gestures can both apply an Ant food-density operation.

Expose typed tool settings through the existing schema convention and declarative UI.
Add Create Tool/Brush templates in Source/Editor and automatic registration/build/reload.
The engine edit context owns bounds, validation, change commands, undo, safe application
and cache invalidation; custom tools supply domain operations. Never require another
undo stack or expose SFML. Support custom data layers rather than forcing food into tiles.

Gate: create a custom brush, edit its validated setting, use two gesture shapes, cancel,
undo/redo, save/reopen and reload while a gesture is active. Test target deletion and
project switching. Ant and an independent custom brush consume the same API.

### 5. Shared collision and queries

Derive render chunks, collision and spatial indexes from the authored environment, with
hit identity back to the object/cell. Separate visual materials from collision/surface
properties. Implement supported tile/box/segment geometry consistently for movement,
raycasts and overlap/clearance queries, including moving obstacles and layer masks.

Start from existing grid and physics services. Do not introduce a robot-only obstacle
map or equate texture RGB with collision. Units and Ant's conversion are explicit.
Actual sensor timing/noise and autonomy remain Milestone 19.

Gate: an independent moving-body/query fixture detects and collides with authored walls,
including after painting, transforms, undo and reload. Test boundary/corner and miss
semantics. Rendering and query/collision geometry agree within recorded tolerances.

### 6. Ant environment migration

Use shared authored ground/walls and tool transactions. Keep ant food/pheromone/lifecycle
rules in Ant; register custom brushes for domain data. Import existing PNG maps with an
explicit mapping for parity, while supporting a fully editor-authored map.

Retain AntWorld's accepted Physics/Rendering/Runtime composition and current simulation
ordering. Remove superseded generic project painting/storage only after replacement
passes. Do not reinterpret cell scale or change wall sampling to simplify migration.

Gate: paint a new Ant map, add colony/food, run, edit supported data, undo/redo, save/reopen
and reset. Verify imported baseline occupancy and relevant deterministic/parity checks.

### 7. Performance, measured throughout implementation

Begin with the saved fixture and profiler during package 1; measure affected paths after
each slice. This final package closes unresolved regressions, not postpones profiling.

Historical September 12 Release median/p95 ms: Inspector scroll 1.64/2.02; 1 colony 4.83/5.75;
3 colonies 10.44/11.48; 5 colonies 15.62/17.00. That historical five-colony p95 exceeded 16.67 ms.
September 13: package 7 passes all three matched repetitions; five-colony p95 is
16.407 / 16.456 / 16.560 ms. Map budgets and native interaction checks also pass.
See [complete measurements and limits](R6_PERFORMANCE_GATES.md).
Use matched map, seed, populations, 1440x900 viewport, timestep and warmup. Record actual
live counts, hardware/build and competing workload. Use repeated matched runs to assess
variance, not the best isolated run. Keep full logs and stage breakdowns.

Retain a 16.67 ms p95 software-frame target for the agreed editor/1/3/5-colony fixtures
on the recorded reference machine. Establish and record small/large map dimensions and
painting/load/memory workloads before implementing performance acceptance for new tools.
Measure paint/drag/scroll input handling and visible response; offscreen timings alone
are not physical input-to-display latency. Inspect cache rebuild and allocation costs;
prefer changed-chunk updates and batched data over per-cell scene entities. No simulation
quality/population/timestep reduction to make the same fixture appear faster.

Gate: agreed fixtures meet their targets with recorded results and no unexplained
regression. Missing targets or physical interaction checks remain explicitly open.

### 8. Integrated acceptance and documentation

**Complete September 13.** [Learning guide and coverage evidence](R6_BLANK_PROJECT_ACCEPTANCE.md).
The intervening performance miss was resolved; [final package-7 verification passes](R6_FINAL_PERFORMANCE.md).
The satisfied workflow scope follows.

Run one coherent blank-project workflow through the editor: create playground/assets,
paint a maze, place obstacle/spawn/goal, generate and attach settings/behaviour, build,
run/pause/reset, prefab/save/reopen, and rebuild/reload. No hardcoded map coordinates,
external map generation or engine source edits. Repeat environment authoring in Ant.

Document the actual APIs with source links: asset/component roles, custom brush creation,
UI composition, module integration, edit/play policies, PNG import and sensor-query use.
Capture interactions and inspect screenshots; run relevant engine/editor/Ant and reuse
checks on the same revision. Preserve failed build/reload and validation recovery tests.

Gate: all eight packages have evidence and no outstanding required feature. Only then
mark R6 verified and proceed to R7's removal/documentation/completion audit.

## Scope and reporting rules

SailBoat code and tests remain excluded. Milestone 19 consumes the finished environment
foundation; robot parts/sensors/controllers and true 3D terrain are not silently added
here. Plain data uses schemas; systems/tools use meaningful execution contracts. Public
engine APIs stay generic; no Ant type switches or special defaults in Workbench.

For each package report implemented behavior, tests/evidence, measured limitations and
remaining work. Update this checklist rather than treating historical milestone claims
as current acceptance. No percentage completion based merely on file or checkbox counts.

# R6 Playground object authoring

September 12, 2026. Status: initial ground-object path verified; painting remains open.

## Available now

Generated projects derive from SceneProjectRuntime. After building/reloading against the
current SDK, their existing creation picker includes **Playground** automatically.
No project-specific registration, Inspector panel or render loop is required.

The engine recipe is ComponentEntity<Transform2DComponent, PlaygroundComponent>.
Its IDs are `pipeframe.playground2d` (entity) and `pipeframe.playground` (component).
The component lives in `engine/include/PipeFrame/Components/PlaygroundComponent.h`;
its Schema() is the single definition of editable fields, validation and read-only size.

Select Playground and edit Columns, Rows, Cell Size, Ground Color or Show Cell Grid.
Local Size is derived and read-only. The lower-left/top-left interpretation follows the
existing 2D coordinate system: local bounds begin at (0,0), with positive width/height.
Transform position moves that origin; rotation and scale affect drawing and picking.
Use ordinary scene transform and component edits, undo/redo, save/load and prefab paths.
The standard Inspector discovers the attached components; there is no Playground-specific UI.

Rendering uses neutral Canvas: two ground triangles and optional grid lines. The bounds
are selectable across their entire area. Other small scene objects take hit-test priority
over a containing Playground. Multiple Playground instances have separate components.
Bounds do not imply physical boundary walls.

SceneProjectRuntime performs the built-in rendering. Projects that override Render must
call the base implementation if they want this built-in ground rendering. Specialized
Ant runtime does not inherit this implementation and is not migrated by this change.
Previously built generated plugins must be rebuilt to acquire the new registration.

## Verification

Debug SceneProjectRuntimeTests and ProjectBuildTests build passed. Dependency lint passed.
SceneProjectRuntimeAcceptance + ProjectBuildAcceptance: **2/2 passed**, 9.52 seconds.
Evidence: `evidence/r6-playground/tests.txt`.

Tests verify real ECS component presence, standard inspection, bounded picking, transformed
picking and Canvas geometry submissions. The temporary generated-project build test creates
Playground through ProjectSession, edits dimensions, rejects zero cell size, undoes/redoes,
rebuilds/reloads, saves and reopens with the authored columns retained. This is API/editor
workflow acceptance, not a new native screenshot or pointer painting test.

## Remaining

Playground supplies colored ground and a grid. The subsequent R6_TILEMAP_COMPONENT.md
slice adds imported Tilemap assignment/rendering. Rows/columns therefore resize empty ground only; no
painted-cell crop or resolution conversion is claimed. Tilemap assignment/reference resolution is now implemented; asset editing ownership,
painting palette and pointer gestures are still needed. The existing Tilemap2D/TilemapEdit services remain ready for that integration.

Ant environment migration, full native UI isolation, material/texture authoring and
performance acceptance remain open. R6 is not complete.

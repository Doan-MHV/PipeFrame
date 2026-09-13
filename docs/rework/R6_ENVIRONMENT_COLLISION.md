# R6 package 5 — Shared environment collision and queries

Status: **Complete** (September 12, 2026).

Scope: shared 2D environment geometry, queries, continuous circular-body movement,
render chunk reuse, and Ant wall-contact integration. Validation recorded below.

## Authoring and runtime contract

Generated projects using `SceneProjectRuntime` offer **Environment Obstacle** as a
built-in entity, alongside Playground. Its Transform and Environment Collider
components use the same component-owned schemas and inspector as other entities.
Choose Box or Segment, edit local dimensions/endpoints, and set collision layers.
`visible` controls drawing independently of `enabled`, which controls collision.
Positive box dimensions and distinct segment endpoints are validated on edits.

The runtime reads assigned tilemap assets and object components directly. Tile
occupancy/solid flags and layer masks control collision; texture RGB, material tint,
and visibility do not. Colors/materials do not imply friction, elevation, or sensor
reflectivity. Those properties need an implemented consumer before becoming physics.

All positions, radii, lengths and returned distances use scene world units. Velocity
uses world units/second; FixedUpdate receives seconds. Transform rotation uses radians
internally (the inspector handles its display units). Box size and segment endpoints
are local values; translation, rotation and signed nonuniform scale produce the same
world geometry for drawing and queries. Circular body radius is a world-space radius.

## API for project developers

Include `PipeFrame/Project/SceneProjectRuntime.h` for the standard project runtime.
It provides:

- `RaycastEnvironment(origin, direction, maximumDistance, mask)` — nearest hit.
- `OverlapEnvironment(circle, mask)` — all touching/intersecting environment shapes.
- `HasEnvironmentClearance(circle, mask)` — true only when a valid circle is clear.
- `SweepEnvironment(circle, displacement, mask, ignoredObject)` — first continuous
  contact along a circular body's displacement.

A hit includes object ID, world contact point/normal/distance and geometry kind.
Tile hits retain layer, source cell and tile ID even after the object's transform
changes. Box/segment hits use cell `{-1,-1}` and tile ID zero. The existing
`TilemapQueryHit` name remains compatible at source level; its added geometry field
requires **plugin ABI 10**. Rebuild project plugins with this engine.

Attach `KinematicBody2DComponent` to an object with a Transform for shared swept
movement and sliding. Project code supplies velocity; the runtime handles collision.
An obstacle can use a Behaviour's FixedUpdate to translate its Transform. Relative
motion checks cover an obstacle sweeping into a stationary body, including a moving
tilemap. This is prescribed kinematic motion, not a rigid-body impulse solver.

`ShapeQueries2D.h` exposes the same primitive solver to specialized runtimes.
`GridCollision2D.h` borrows an existing grid through a blocked-cell predicate and
provides `MoveCircle` sweep/slide. Ant's ContactSolver now uses these functions with
its canonical environment cells. One Ant cell remains one world unit, with no PNG
color lookup or copied obstacle grid in the collision path. Ant's circle-circle
contacts and colony/food/pheromone rules remain domain code. Full authored-environment
migration is package 6.

## Geometry and update behavior

Tile rays use the existing grid traversal; overlaps/sweeps bound their candidate
cells in the authored tile grid. Primitive obstacles are currently visited in stable
object-ID order; there is no additional BVH or robot-specific obstacle map.

`TilemapChunkCache` keeps 32×32-cell render chunks. After an edit or asset reimport,
only chunks with changed visible tile/tint data regenerate geometry. Changes to
numeric brush data do not regenerate visual chunks. An atlas/layout change invalidates
all affected geometry. Asset revisions refresh the canonical map used by queries,
so collision has no separately stale chunk copy. Geometry is flattened for the
existing renderer; per-chunk GPU submission and large-map performance acceptance
are not claimed here and remain package 7 work.

Ray directions are normalized; invalid/zero directions and invalid ranges miss.
Inside-box rays return distance zero with no entry normal. A ray starting on an
outgoing box or tile face misses that shape. Segment rays are two-sided; collinear rays hit
the nearest endpoint or their starting position if already on the segment.
Overlap/clearance includes touching. Sweeps allow movement away from a touching
surface and tangential sliding, while rounded-corner contact stops inward motion.
Initial penetration returns a zero normal and stops motion rather than teleporting
the body. Up to four contacts are resolved per movement step.

Continuous obstacle motion currently supports translation. Rotation/scale changes
are sampled at the new pose, without angular continuous collision. Multiple moving
obstacles resolve in deterministic order; crushing/depenetration and dynamic
body-to-body impulses are outside this kinematic environment solver.

## Acceptance evidence

`engine/tests/EnvironmentCollisionTests.cpp` is an independent project fixture. It
covers box/segment edges, corners, misses, inside starts, masks, hidden/disabled
collision, transformed geometry, moving boxes and tilemaps, stationary-body pushes,
paint/save/reimport, undo, runtime reload and source-cell identity. A recording render
surface checks the same box corner used by ray and body contact. Numeric comparisons
use an absolute tolerance of **0.0001 world units**.

Chunk checks use a 96×64 map: six initial chunks, one rebuild for a local tile edit,
zero for a numeric-layer edit, and two for edits in two chunks after reimport.
The fixture also compares shared Ant-style borrowed-grid and authored-tile sweeps.

Build/test logs are in `evidence/r6-environment-collision/`. Package 5 does not claim
Milestone 19 sensor noise/timing/autonomy or closure of the R6 performance target.

Validation on the local macOS arm64 build, with `PIPEFRAME_ANT_REWORK_ONLY=ON`:

- Debug and Release engine/Workbench/Ant builds passed, including public-consumer
  and UI boundary compile targets.
- Full Debug suite: **75/75**, 107.12 seconds.
- After the final outgoing-tile-face consistency fix: **7/7** affected Debug
  checks, 2.14 seconds.
- Release tilemap, collision, scene runtime, visual assets, Ant physics/contact,
  parity and stress: **8/8**, 6.97 seconds.
- New public collision/chunk headers and all Ant Source C++ files: no `SFML` or
  `sf::` matches (109 files scanned).

These are regression results, not a new five-colony frame-time acceptance claim.

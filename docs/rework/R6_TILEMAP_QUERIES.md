# R6 transformed tilemap queries

2026-09-12. Query primitives verified; runtime movement integration remains open.

`Environment/TilemapQueries.h` adds `RaycastTilemap` and `OverlapCircleTilemap` over
actual Tilemap2D data. Queries use collision-enabled layers and solid tile definitions,
independent of layer visibility. A uint32 mask selects layer indices. When multiple
solid layers occupy a cell, the first matching layer provides the hit identity.
Hits carry caller-supplied object ID, layer, cell, tile ID, world point/normal and distance.
Ray distance is measured along a normalized world ray. Overlap distance is distance
from the circle center to the closest tile point (zero for a center inside a tile).
A center-inside overlap reports a zero normal; it is not a penetration solver.

Transforms support position, rotation and signed independent axis scales. Ray normals
use inverse-transpose scale and rotation. Circle queries test against rotated scaled
rectangles, avoiding the incorrect assumption that inverse nonuniform scaling keeps
a circle circular. Tangencies count as overlaps. Zero/invalid transforms and nonfinite
query inputs are rejected. Query radius/range use world units.

GridRaycast now clips incoming rays to grid bounds before DDA traversal, supporting
sensor origins outside the map without traversing out-of-bounds cells. Existing inside
ray behavior remains covered. Maximum range must be finite; outer maximum boundaries
are excluded for parallel/outgoing rays and included when pointing into the grid.

No duplicate obstacle grid is cached. Painting, undo, collision-layer changes and map
replacement are observed by querying the current map reference. There is no SFML API.

## Example

```cpp
auto hit = pipeframe::RaycastTilemap(map, transform, sensorPosition, direction,
                                   100.0f, 1u << wallLayer, objectId);
auto occupied = pipeframe::OverlapCircleTilemap(map, transform,
                                               {bodyPosition, bodyRadius}, 1u << wallLayer);
```

## Verification

TilemapRegression, SpatialServicesRegression, PhysicsEnvironmentRegression and
PipeFrameDependencyLint pass 4/4. Tests cover translated/nonuniform/rotated/reflected
maps, outside origins, maximum-face entry, ranges, masks, face tangency and clearance,
identity, hidden collision layers, collision toggles and immediate undo/redo effects.
Evidence: [test output](evidence/r6-tilemap-queries/tests.txt).

## Still open

These functions are not automatically dispatched by SceneProjectRuntime or AntWorld.
They operate on the full tilemap bounds, not a separately cropped Playground rectangle.
Shared scene-object query collection, box/segment obstacles, moving-body collision and
sweeps, moving obstacles, transformed Playground clipping and performance acceptance
remain required. Overlap currently enumerates the circle's local bounding cell range;
large-radius/chunk-index optimization has not been benchmarked. Ant simulation behavior
was not changed, and no R6 completion claim follows from this query slice.

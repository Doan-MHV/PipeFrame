# R6 scene environment query integration

SceneProjectRuntime now provides RaycastEnvironment and OverlapEnvironment. Both
collect objects with Transform2DComponent and TilemapComponent, resolve their assigned
asset through the existing TilemapAssetModule, and return hits with actual scene IDs.
Projects inheriting the generated runtime do not need to maintain an obstacle list.

RaycastEnvironment returns the closest hit, with deterministic object-ID ordering
for ties. OverlapEnvironment collects cell hits across objects. Masks select tilemap
layer indices. Visual visibility does not disable collision-enabled layers.

Tilemap queries now accept optional local bounds. Runtime supplies Playground bounds
when attached, matching rendering's clipped rectangles. Rays begin/end at the clipped
region; overlaps intersect each tile rectangle with that region. This handles partial
cells, outgoing maximum-face rays and existing maps larger than their Playground.
Underlying shared tilemap data is not cropped or copied by these queries.

Changes to the scene transform, assigned asset, asset revision, Playground dimensions
and object deletion are observed on the next query. No persistent pointer collection
or second collision map is created. Existing plugin ABI remains 7: these are nonvirtual
convenience methods and helpers, with no added runtime layout or vtable entries.

Example within a SceneProjectRuntime subclass:

```cpp
auto obstacle = RaycastEnvironment(sensorPosition, forward, 100.0f);
auto clearance = OverlapEnvironment({bodyPosition, bodyRadius + safetyMargin});
```

Validation covers actual assigned assets, identity, clipped cells and partial faces,
movement of the owning scene object, reimport, deletion, and the existing tilemap
transform/range/mask tests. The generated-plugin build acceptance also runs.
Evidence: [test output](evidence/r6-scene-queries/tests.txt).

Still open: moving-body collision/sweep/response, box and segment colliders, external
systems' service access, Ant's specialized runtime integration, large-world indexing
and performance gates. These query methods do not automatically stop a moving body
or implement robot sensing/timing. R6 remains open.

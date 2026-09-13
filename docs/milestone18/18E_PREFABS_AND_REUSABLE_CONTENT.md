# Milestone 18E — Prefabs and reusable content

Status: complete (2026-09-11).

Milestone 18E adds a persistent prefab workflow to the real component scene
model. A prefab stores one rooted object hierarchy, internal connections,
components, properties, nested prefab links, and a stable source-object identity.
It can represent a visual object, a simulation setup, or a multi-part machine.

## Data and persistence

- `PrefabDefinition` stores a stable ID, source revision, optional base-prefab
  ID, and a complete `SceneDocument` source.
- `.pfprefab` is a small versioned manifest. Its companion `.pfprefab.scene`
  uses the normal scene serializer, so component and scene migrations have one
  implementation.
- Scene format version 7 persists an ordered list of `PrefabInstanceLink`
  records on every object. The order preserves nested instances from the
  innermost prefab to the outermost assembly.
- Every link records the prefab ID, source revision, stable source-object ID,
  concrete scene-instance root, and variant/base identity.
- Object references and mechanical, power, and signal connection endpoints are
  remapped when a prefab is created or instantiated.
- The asset database recognizes `.pfprefab` as `AssetType::Prefab`, imports it
  into the deterministic cache, and exposes it in the Asset Browser.

## Authoring operations

`PrefabLibrary` implements:

- creation from a selected rooted hierarchy;
- adoption of the original hierarchy as the first instance;
- instantiation under a scene parent and at a requested transform;
- nesting without erasing inner prefab identity;
- variants with an explicit base-prefab ID;
- override discovery for names, transforms, object properties, components,
  added objects, and missing source objects;
- revision-conflict reporting;
- apply, which writes an instance back to its source and advances the revision;
- revert/update, which restores source data while preserving existing scene IDs;
- unpack, which removes one prefab layer while preserving nested prefab links;
  and
- internal connection restoration with new collision-free scene connection IDs.

`ProjectSession` wraps create, instantiate, apply, revert, and unpack in the
normal `SceneHistory` and `DocumentChanged` path. Prefab edits therefore update
dirty state, undo/redo, runtime synchronization, and selection consistently
with ordinary scene edits.

The viewport context menu exposes **Create Prefab**, **Apply Prefab**,
**Revert Prefab**, and **Unpack Prefab**. Dragging a prefab asset from the Asset
Browser to the viewport creates an instance at the pointer instead of assigning
the asset to an Inspector field.

## Stable assembly behavior

A nested assembly retains two identities where applicable. For example, a
wheel object inside a robot assembly retains its `wheel` prefab link and gains a
second `robot` link. Unpacking the robot removes only the `robot` layer. The
wheel remains an editable wheel-prefab instance. Revert updates objects in
place, so scene references to existing instance objects do not silently change.

Root transforms are placement data. Creating a prefab normalizes its source
root, instantiation applies the requested placement, and apply/revert do not
bake an individual scene placement into the reusable source.

## Verification

`PrefabLibraryRegression` covers:

1. a wheel hierarchy with a stable internal signal connection;
2. creation and adoption of the authored hierarchy;
3. a robot assembly containing the wheel prefab;
4. prefab manifest and source persistence;
5. multiple instances and nested-link preservation;
6. override detection and scene-history undo/redo;
7. scene version-7 round trips;
8. in-place revert with stable scene object IDs;
9. apply and source revision advancement;
10. stale-revision conflict reporting;
11. propagation to another instance;
12. variant base identity; and
13. outer unpack while retaining the nested wheel instance.

`WorkbenchUIAcceptance` adds the dynamic-library boundary: it authors nested
wheel/robot prefab links and a typed component override, reloads the external
runtime from a staged library copy, and verifies both the scene document and
replacement runtime receive the same links and complete component data. The
same test reloads Basic, Ant, and SailBoat and verifies their component-backed
reference scenes remain unchanged.

Asset-database, scene-serializer, component-scene, and Workbench UI regressions
run alongside the prefab test. The complete Debug build and all 70 registered
tests pass at this checkpoint.

# Milestone 18B — Component scene model and hierarchy

Status: complete (2026-09-11).

## Delivered model

- Scene objects have stable IDs, parent IDs, sibling order, local transforms,
  inherited world transforms, layers, tags, visibility, and locking.
- Every object has a required `pipeframe.transform2d` component synchronized
  with its canonical transform. PipeFrame also publishes Identity, Physics Body
  2D, Collider 2D, and Renderer 2D schemas. Physics, collision, and rendering
  remain optional per archetype.
- Plugins publish component descriptors and archetypes through
  `ProjectRuntime`. Descriptors carry stable field keys, defaults, units,
  constraints, schema versions, editability, enum choices, and editor hints.
  The typed authoring builder avoids compiler-specific reflection and field
  offsets.
- Component values support booleans, integers, numbers, strings, vectors,
  colors, assets, and stable object references without SFML types.
- Scene settings preserve length units, angle units, and coordinate convention.
- Mechanical, power, and signal connections use stable object/attachment
  endpoint IDs. The scene rejects missing endpoints, duplicate IDs, duplicate
  endpoint pairs, and self-connections; deleting an object removes its links.
- The document supports rename, reparent/reorder, recursive duplicate/delete,
  group, layer/tag changes, visibility/locking, component add/remove/edit,
  validation, and combined name/type/layer/tag filtering.
- `ProjectSession` wraps authoring changes in undoable transactions and
  synchronizes the runtime. Multi-object metadata changes and grouping produce
  one history entry.
- `SceneWorkspace` supplies named scene creation from a template document,
  duplication, active-scene selection, removal, and additive loading.
- The hierarchy view traverses parent/child order and identifies hidden, locked,
  or structurally invalid entries while keeping selection IDs aligned with the
  viewport.

## Persistence and migration

Scene format 6 stores scale-bearing transforms, hierarchy metadata, component
schemas, all property variants, scene units, coordinate convention, and stable
connections. The loader accepts formats 3 through 6. Older files receive
default scale, Transform components, default units, and no connections during
load, then save as format 6 on their next write.

## Verification

- `ComponentSceneModelAcceptance` covers transform inheritance, cycle
  rejection, search/filter, layers, tags, visibility, locking, component
  mutation, connection validation and undo/redo, recursive duplication,
  grouping, templates, multiple scenes, and additive loading.
- `SceneSerializerRoundTrip` covers format-6 reload of hierarchy, components,
  object references, colors, units, coordinates, and endpoint connections.
- `WorkbenchUIAcceptance` proves a plugin-defined component can be registered
  and instantiated by an archetype without Workbench source changes, and keeps
  hierarchy/viewport selection behavior covered. It replaces the external test
  plugin and every Basic, Ant, and SailBoat runtime in place, then verifies that
  component registries, component instances, stable references, authored
  values, and prefab links remain intact.
- Complete Debug build: passed.
- Complete Debug test suite: 70/70 passed.

Metadata-generated component controls and custom property drawers begin in
18C. This keeps 18B focused on the durable model, persistence, hierarchy, and
transaction contracts those controls consume.

The standard project layout, optional decorator-style convenience API, and
registered system lifecycle are required by later Milestone 18 phases and are
tracked in
[PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md](PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md).
They build on 18B without reopening its completed scene-model scope.

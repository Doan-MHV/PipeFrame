# Ant authoring implementation proof

> Historical Milestone 18 checkpoint, superseded by the engine rework. Do not use
> its aggregate Agent2D/AgentGroup2D/EntityStore or AntAuthoring conventions for new code.
> Use the [current Ant source guide](../../examples/AntSimulation/README.md),
> [component schemas](../rework/COMPONENT_SCHEMA_CONVENTION.md),
> [lifecycle API](../rework/BEHAVIOUR_LIFECYCLE.md) and
> [R7 migration audit](../rework/R7_AUDIT.md).

Updated 2026-09-11. This is an implemented Ant improvement, not a declaration
that Milestone 18, SailBoat migration or backend isolation is complete.

## Engine responsibilities now used by Ant

- `Agent2D` owns position, identity, velocity and active state and supplies
  integration and target steering. Ant inherits spatial storage and its movement
  system calls shared steering. Ant still specializes velocity/activity queries
  to account for its direction, speed and energy rules.
- `AgentGroup2D` owns group identity, position and membership count. Colony uses
  that storage; its lifecycle system updates the shared count.
- `EntityStore` allocates identities, constructs entities, finds and destroys
  entities, and repairs dense storage lookups. AntStore uses its `Spawn` path.
- Movement, behavior and cleanup are real fixed-update systems used both by the
  headless pipeline and the plugin scheduler.
- `ComponentSchema<T>` binds member pointers to editor metadata and implements
  validated atomic assignment and serialization. `ColonySettings.h` is the
  working example. The runtime consumes its bound values when rebuilding Ant.
- `ui::Controls` creates themed buttons, equal-width action rows and range
  inputs. Ant uses it for brush radius, marker intensity and selected-ant
  controls. Existing dashboard infrastructure handles input and drawer layout.

## Where a new developer starts

1. Put a runtime entity in `Source/Components`, deriving from `Agent2D` when it
   needs common agent motion. Keep domain state on the entity.
2. Put its authorable settings alongside it. Declare each member once in a
   `ComponentSchema<T>`; expose `Schema().Describe()` through the runtime registry.
   `Serialize` and `Apply` use the same bindings.
3. Put fixed-step rules in `Source/Systems`, implementing `FixedUpdateSystem`.
   Register the phase and dependencies in the plugin composition root.
4. Use `EntityStore<T, AgentId>::Spawn(...)` rather than managing IDs and indexes.
5. Build UI with shared controls. For example, Ant now uses:

```cpp
const pipeframe::ui::Controls controls(*dashboard);
controls.ActionRow(selected, {
    {"FOLLOW", [this] { ToggleFollow(); }},
    {"TARGET", [this] { ToggleTarget(); }}
});
```

The callback names above illustrate project actions; Ant supplies its inspector
callbacks directly. The project supplies no button geometry, colors or flex
allocation for this row.

## Remaining work that must not be called complete

- SailBoat has not yet migrated to the same entity/group/store layout.
- Rendering, dashboard integration and public legacy UI types still expose
  SFML. This controls facade is not a complete backend-neutral Flutter-style UI
  library, and must not be presented as one.
- A new arbitrary C++ entity is not automatically registered by adding a file.
  Code generation, registration and the editor build/reload loop still need a
  complete workflow.
- Component schemas do not automatically attach physical bodies to every Ant.
  Ant's existing physics bridge remains responsible for its simulation bodies.
- Other Ant settings still use descriptor metadata; ColonySettings is the first
  member-bound implementation. Live individual-ant editing is not implemented.

These are acceptance gaps, not optional polish. Do not close 18G/18H on the
strength of an ID base class, directory existence or a passing test count.

## Verification

The complete Debug build and 70/70 CTest tests passed after these changes,
including Ant behavior parity, stress, typed binding, entity storage and
simulation dashboard interaction checks. Graphics tests require desktop access;
the sandbox-only attempt crashed in unrelated graphics suites. This is automated
behavior evidence, not a new visual-design approval or Milestone 18 completion.

## ECS, attached behaviours and declarative views (follow-up)

The earlier utility-only proof was insufficient for the requested authoring
model. The engine now also provides:

- `PipeFrame/ECS/World.h`: entity lifetime, independent typed component pools,
  add/get/remove, and multi-component queries. AntStore delegates to this world.
- `PipeFrame/ECS/Scene.h`: attached `Behaviour` scripts, registered factories,
  component access, enable/disable, Start-once, variable/fixed updates and
  deferred destruction with cleanup callbacks. Colony components live in this
  scene, and a registered `ColonyBehaviour` drives each colony's simulation.
- `PipeFrame/UI/View.h`: backend-neutral Row, Column, Text and Button trees.
  AntSelectionView describes the current inspector state. The engine's private
  renderer reconciles stable keys and keeps widget identity during input.
- Ant composition now lives in `Source/Runtime`; dashboard and view construction
  live in `Source/Editor`; public type IDs live in `Source/Components`.

Reference designs reviewed: [Unity MonoBehaviour](https://docs.unity3d.com/2020.1/Documentation/ScriptReference/MonoBehaviour.html)
for attached lifecycle hooks, and [Flutter declarative UI](https://docs.flutter.dev/flutter-for/declarative)
for state-derived descriptions with persistent rendering objects. This code
implements those limited patterns; it does not implement either framework.

Still required: editor-driven arbitrary Behaviour registration/build/reload,
collision/visibility events and coroutine integration, full Ant dashboard
migration, and removal of SFML from remaining legacy public/editor boundaries.
The new View declaration and ECS headers are included in dependency lint.

Follow-up verification: full Debug build and all 70 CTest tests passed after the
ECS/behaviour/view migration (94.96 seconds). The focused deferred self-destruction
test also passed. Passing these tests does not close the remaining gates above.

# R6 — authoring workflow and backend boundary

Authoritative remaining-work checklist: [R6 execution plan](R6_EXECUTION_PLAN.md).
Eight packages cover UI isolation, playground/assets, editing, custom tools, geometry,
Ant migration, performance and integrated acceptance.

## Current boundary status

Public UI/editor backend isolation is **complete**. All 31 public UI/editor headers
compile separately without native includes, and a consumer linked to Engine verifies
that SFML does not propagate through its CMake interface. Full Debug suite: **71/71**
passed in 100.87 seconds. Debug/Release builds and native normal/narrow captures passed.
See [the completion report](R6_PUBLIC_UI_ISOLATION.md). Plugin ABI remains **7**.
Other R6 packages remain open in the execution plan.

## Historical presenter migration

[Six Workbench presenters and ViewPanel](R6_PANEL_PRESENTERS.md) now have neutral
public headers and backend-free source compile checks. Plugin ABI is **6**. Native
window/dock/widget interfaces remain open; tile editing is not yet implemented.
This continues the same View/StatefulView UI architecture required for R6-E.
Verification: 69/69 passed (97.49 seconds), Debug/Release builds and native UI checks pass.
Direct SFML references in Editor headers are down from 11 to 5; this count excludes
remaining engine Widget/Controls and other native hosts, so it is not full isolation.

## Latest package-1 implementation

[Dashboard/controller boundary](R6_UI_BOUNDARY.md) implemented; public dashboard and
actual Ant UI sources compile without native include paths. Plugin ABI is now **5**.
Widget/ViewPanel and Workbench header migration remain open. Checkpoints below predate
this slice and their ABI numbers/test evidence describe those revisions.

## Environment authoring requirement — September 12

**New required scope, not implemented:** [R6-E blank-project environment authoring](R6_ENVIRONMENT_AUTHORING.md).
A generic scene editor, Ant-specific brushes and texture loading do not satisfy this gate.
R6 now has three open tracks: native UI/header isolation, measured performance, and the
shared environment workflow (assets, tile painting, collision/queries, Ant migration).

## Current status — render boundary approved and implemented

**R6 remains open for native UI/header isolation, environment authoring and performance.** The integrated
configure/build/cancel/reload workflow and generated-project authoring acceptance are
implemented. Camera2D and RenderContext now use neutral PipeFrame types through
RenderSurface. The user approved this migration; it is no longer waiting for approval.
See [render boundary and remaining scope](R6_RENDER_BOUNDARY.md) and
[build workflow](R6_BUILD_WORKFLOW.md).

Final verification for the render migration: **69/69 tests passed** in 96.17 seconds;
Debug and Release builds pass, including the standalone neutral header target.

Plugin ABI is now 4. Existing runtime libraries must be rebuilt; an incompatible reload
is rejected before registration and leaves the current runtime active. Ant Plugin.cpp
and the render public headers compile without SFML include directories.

Latest post-migration Release measurements: Inspector scrolling median/p95 1.64/2.02 ms;
1 colony 4.83/5.75 ms; 3 colonies 10.44/11.48 ms; 5 colonies 15.62/17.00 ms.
The five-colony p95 is slightly above the 16.67 ms target. Do not claim the performance
gate universally closed; the earlier lower result is a historical run. Evidence and
fixture details are in [the boundary report](R6_RENDER_BOUNDARY.md).

The sections below are historical checkpoints. In particular, statements that the editor
has no build runner are superseded by this current status.


## Latest world-composition checkpoint — September 12, 2026

Ant now uses `World/AntWorld`, `World/Physics/AntPhysicsWorld`,
`World/Rendering/AntRenderingWorld` and `World/Runtime/AntRuntimeWorld` with executable
PipeFrame module bases. All Ant Source direct backend references have been removed and
are guarded by dependency lint. See [the current architecture and boundaries](ANT_WORLD_COMPOSITION.md).
R6 remains open for the integrated build/reload workflow, broader editor public-header
isolation and the previously recorded performance acceptance. Earlier counts and paths below
are historical snapshots, not the current Ant folder layout.


Status: **in progress**, September 12, 2026. This entry is an implemented runtime/generation slice, not R6 acceptance.

## Current checkpoint — September 12, 2026

**R6 is still open.** The Ant entity/runtime migration and source/attachment controls below are implemented. The earlier entries are historical; the build runner and full backend-isolation gates are not complete.

- `AntEntityTypes()` in `Source/Runtime/AntRegistration.h` now registers **Ant, Colony, Food Source, Simulation Settings and Signal Beacon**. Ant is runtime-only; the other four populate the editor picker. AntQuery and ColonyLifecycleSystem use the engine registry's typed `Spawn` path with their original domain constructor arguments, avoiding property-map construction on each ant spawn.
- `BindAntFactories()` in `AntRegistration.cpp` supplies Colony's world-dependent factory and post-restoration settings hook. AntSimulationRuntime has no Colony/Beacon creation branch. Its generic registered batch includes all authored entity types; colony lifecycle still controls ant spawning.
- The engine now owns authored scene-diff checks, component-edit transactions, compatibility property reading, registered entity lookup/restoration and cleanup. AntSimulationRuntime.cpp is approximately **1,090 lines versus 1,330 before this migration**. No additional Ant implementation file was introduced. Domain simulation, bespoke rendering and dashboard host integration remain there.
- Ant input consumes neutral InputEvent directly, instead of translating it back into an SFML event. RenderContext supplies neutral pixel-to-world mapping. This is not full backend isolation.
- Hierarchy **+ → NEW ENTITY / COMPONENT / BEHAVIOUR** opens a declarative source-generation form. It creates a standard-folder class and updates generated registration. Entity templates include Transform. Existing class files are not overwritten. Ant consumes GeneratedRegistration.h when present, as do generated projects.
- Inspector **ADD COMPONENT / BEHAVIOUR** attaches/removes registered optional components through scene commands. Required components are protected. Actual Motion attachment/removal is tested against Ant's live ECS.
- Ant runtime acceptance registers another test archetype through its public extension API and instantiates it without modifying the runtime implementation. Source-dialog acceptance types a class name and creates the actual file. A generated plugin with component, behaviour and entity registration was compiled and loaded without SFML include directories.

Final verification for this checkpoint: **65/65 tests passed** in 60.08 seconds; the native source-form interaction and screenshot capture passed, and the generated plugin compiled/loaded successfully. Evidence: `R6_AUTHORING_ACCEPTANCE.txt` and `evidence/r6-authoring/source-generation.png`. CMake also tracks first-time creation of Ant's optional GeneratedRegistration.h.

### Still required before marking R6 complete

1. Integrated configure/build execution, output, cancellation, runtime manifest setup and build/reload failure recovery. The new source form currently instructs the developer to build externally and then use Reload; it does **not** pretend this step is automated.
2. Finish migrating remaining Ant/editor public backend dependencies into private engine adapters. There are still 39 source/header files with SFML references across Ant Source and the editor directory; the neutral input conversion does not close this gate.
3. Complete the end-to-end generated project/editor acceptance after those changes, including authored prefab/reopen and appropriate validation. Existing API, native plugin and interaction checks cover portions of the workflow.
4. Close the previously required measured editor/1–3–5-colony performance gate. Stress-test success does not establish 60 FPS.

For the current Ant project, after creating source use the existing configured build's `AntSimulationRuntime` target, then the editor's **Reload** action. New generated projects still require configuring their native build and runtime path manually.

## Implemented

- `SceneProjectRuntime` supplies shared authored-ID-to-ECS mapping, typed component registry inspection, scene synchronization, checked handles, behaviour attachment and fixed lifecycle, setting edits, reset and teardown. Unchanged authored objects keep their live state. Invalid authored component values are validated in an isolated candidate scene before applying the scene update.
- New project runtimes derive from this engine implementation instead of generating empty scene/lifecycle overrides.
- Adding a component or behaviour regenerates `Source/Runtime/GeneratedRegistration.h` from `Config/Modules.pfconfig`. Components register their owned `Schema()`; behaviours register a typed attachment descriptor/factory. The generated file has an ownership marker and refuses to overwrite an unmarked user file.
- `IdentityComponent` owns the schema needed by the standard generated Object type.
- `.pftype` definitions load into the shared runtime and instantiate attached typed data and behaviour markers. Behaviour attachment/removal is based on authored component presence/enabled flags.
- Existing common component schema validation and editor read-only rules are reused; setting edits are retained for reset.

## Verified slice

`SceneProjectRuntimeAcceptance` uses a SoldierSettings component and SoldierBehaviour to verify actual motion from an exposed setting, live setting edits, preservation across unchanged sync, rejection of invalid scene data, behaviour detachment, reset and invalidation on unload. This is an extension-mechanics fixture; its movement is not Pezzza soldier/combat behaviour.

`ProjectManagerTests --emit-r6-fixture <empty-directory>` uses the real project/module generator to create SoldierSettings, SoldierBehaviour and SoldierAnt definitions. The generated Plugin.cpp was syntax-checked and compiled into a native library using only the engine and generated Source include directories, without SFML include paths. The library was loaded through ProjectRuntimeLibrary, initialized from its generated object definitions, instantiated, stepped, reset and unloaded successfully. This is command-line integration evidence, not proof of editor button interaction or the generated CMake workflow.

## Remaining R6 gates

1. Editor generation/type/attachment UI, including component/behaviour selection and error feedback. Current AddModule is still a scaffolder API, not an end-to-end editor flow.
2. Build/configure execution, build output, cancellation, runtime manifest path setup, successful reload and failed-build/reload recovery. The new project manifest still has no configured runtime library path.
3. Integrate generated extensions with the existing Ant runtime. The reusable scene runtime is currently the new-project path; Ant does not yet consume GeneratedRegistration.
4. End-to-end SoldierAnt creation, run, property edits, prefab/save/reopen, and appropriate scene validation in the editor on the same revision.
5. Migrate remaining Ant/editor/public backend dependencies into engine-private adapters. The generated source's neutral compile does not establish isolation of existing Ant or the editor.
6. Resolve the pre-R6 frame-time gate: current performance results do not establish 60 FPS or restored 4–5-colony capacity.

The shared runtime currently provides no default object rendering. It recreates an entity when its component topology changes; this restarts that entity's attached behaviours. Full transform hierarchy, generic per-component activation semantics, and runtime-state preservation across arbitrary structural edits are not supplied by this slice. Scene candidate validation guarantees invalid property data is rejected before mutation; it is not transactional rollback of arbitrary user behaviour constructor exceptions.

## Relevant sources

- [Shared scene project runtime](../../engine/include/PipeFrame/Project/SceneProjectRuntime.h)
- [Project/module generation](../../apps/SimulationWorkbench/Editor/ProjectScaffolder.cpp)
- [Runtime acceptance fixture](../../engine/tests/SceneProjectRuntimeTests.cpp)
- [Generator integration checks](../../apps/SimulationWorkbench/tests/ProjectManagerTests.cpp)

Final verification for this slice: **65/65 tests passed** in 58.42 seconds. [Acceptance results](R6_RUNTIME_ACCEPTANCE.txt). SailBoat remained excluded.

## Additional objective: real Ant project entity and type picker

Implemented September 12, 2026. `SoldierAnt` above remains a temporary generator fixture, not an Ant project entity or a combat implementation. The permanent example is **Signal Beacon** (`ant.signal-beacon`).

- Hierarchy **+** opens a shared declarative Create Object popup listing the active project's registered object types. Select **SIGNAL BEACON** to create it at the camera centre. Context-menu **PLACE OBJECT HERE** uses the same picker with the clicked position. The search field filters on commit (Enter).
- `Source/Entities/SignalBeaconEntity.h` composes Transform and SignalBeaconComponent and attaches SignalBeaconBehaviour. The behaviour runs in Ant's existing shared BehaviourScene; it rotates the Transform on fixed ticks.
- `Source/Components/SignalBeaconComponent.h` owns editable radius, colour and rotation speed, including range validation. The Inspector discovers these through the existing component registry. No separate beacon-specific Inspector was added.
- `Source/Rendering/SignalBeaconGeometry.h` produces backend-neutral triangle vertices. Existing Ant runtime rendering submits these through its current backend host. This does not complete backend isolation.
- `ProjectSession::CreateObjectOfType` uses the existing scene document/history path. The picker reads descriptors; it has no hard-coded beacon or colony type list.

Verified through real Ant runtime acceptance: explicit type creation, unknown type rejection, undo/redo, component editing, invalid radius rejection, fixed-tick rotation, pause, reset and save/reopen. Editor acceptance clicks the hierarchy plus and beacon entry and checks cancellation. Visual evidence is in `evidence/r6-beacon/create-object-picker.png` and `signal-beacon-inspector.png`.

This completes the requested additional entity/type-picker objective, not the remaining full R6 gates above. Signal Beacon is a small authoring/lifecycle example with no ant interaction. Adding it to this existing Ant runtime still requires registration and runtime factory/render integration; automatic generation/build/attachment remains outstanding. Structural scene edits retain Ant's existing whole-world rebuild behavior.

Validation: the full 65-test suite passed 64 tests; the remaining Ant regression contained two obsolete three-type/count expectations. Both now assert the four-type contract, and its focused rerun passed. All 65 tests are covered as passing across these runs. See `R6_BEACON_ACCEPTANCE.txt`.

## Entity registry follow-up

The manual SignalBeacon creation branch has been removed. `engine/include/PipeFrame/Project/EntityRegistry.h` now supplies archetype factories paired with editor descriptors, schema restoration, registered-subset creation with rollback, authored-ID resolution and explicit cleanup. Ant uses `RegisteredEntityObjects` in its existing BehaviourScene, so it retains one simulation scheduler. Generated-project `SceneProjectRuntime` uses the same registry and restoration path through `RegisterEntity<T>()`.

The project registration is now in `RegisterAntEntities()` in `Source/Runtime/AntRegistration.cpp`:

```cpp
registry.Register<SignalBeaconEntity>({
    SignalBeaconTypeId, "SIGNAL BEACON",
    SignalBeaconComponent::Schema().Describe().properties,
    {SignalBeaconTypeId}
});
```

Component registration remains separate: it declares the typed fields used by all entities. Entity registration pairs an editor type with the recipe that composes those components. No additional runtime branch is required for instantiation, restoration, lookup or reset of another registered archetype. Components and behaviours still belong in their standard folders; no new Ant project source file was introduced for this refactor.

Ant remains a specialized runtime composed with these shared services, rather than inheriting another scene-owning runtime and accidentally creating a second world. Colony/environment initialization, simulation-specific reactions to edits, custom rendering and hit shapes still belong to its domain implementation. This change does not automatically render arbitrary archetypes: those need a renderer, and existing Ant renderer/backend extraction remains open. Editor source generation and build/reload automation also remain open.

Acceptance adds two distinct registered archetypes to a plain SceneProjectRuntime, verifies descriptor discovery, component composition, attached behaviour, reset and duplicate-type rejection, and exercises failed-batch rollback and cleanup using the same mapping service Ant uses. Existing real Ant beacon and picker tests cover the migrated path.

Entity registry verification: 65/65 tests passed (60.17 seconds); after the final ownership/type-change guards, all three focused runtime/Ant acceptance tests passed. Results: `R6_ENTITY_REGISTRY_ACCEPTANCE.txt`.

## Build/reload implementation — September 12, 2026

The previously missing native configure/build/reload path is now implemented. See
[R6_BUILD_WORKFLOW.md](R6_BUILD_WORKFLOW.md) for UI instructions, actual acceptance coverage
and platform/development-SDK limits. Old statements above saying the editor has no build
runner describe earlier checkpoints.

Additional boundary work: ProjectRuntimeHost's public input/position API uses neutral
PipeFrame types, and AntSimulationRuntime's public header forward-declares its UI host.
Ant Plugin.cpp can now be syntax-checked with only engine and Ant include directories.
The native dashboard implementation remains in the engine UI host.

**R6 is not yet complete:** automatic approval review rejected the unapplied broad
Camera2D/RenderContext API migration, citing its effect on core APIs and dependent callers.
The concrete proposal in R6_BACKEND_MIGRATION_REVIEW.md is awaiting explicit approval.
Legacy native UI/public contracts also still require audit; no claim of zero transitive
native exposure across the entire editor/engine is made here.

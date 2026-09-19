# PipeFrame engine rework — Ant reference implementation

Status: R7 complete / verified September 13, 2026. Engine/editor/Ant rework acceptance gates pass. See [R7 audit](rework/R7_AUDIT.md) for same-source evidence and explicit limits.
Scope decision: 2026-09-11. This is the governing rework plan for the engine, editor, and Ant. SailBoat implementation, migration, and tests are excluded from this work. Existing files are preserved; exclusion is not permission to delete that project. Milestone 19 remains deferred.

## 1. Outcome and non-negotiable rules

### General-purpose engine, Ant-first validation

PipeFrame serves multiple independent projects, including future robotics, vehicle, physics and other simulation projects. Ant is the first reference implementation and migration target; it does not define the limits of the engine. The current exclusion of SailBoat restricts implementation and test scope, not engine capabilities or architectural reuse.

Engine modules, scene/component registration, lifecycle, physics, rendering, resources, editor tooling and declarative UI must work without importing Ant headers, knowing Ant type IDs, or assuming colonies, pheromones, insect locomotion or a particular scene. Domain policies are supplied through registered components, systems, behaviours, assets and extensions. Generic services must not embed Ant-specific defaults or rules.

Validate reusable APIs with small independent engine fixtures as well as Ant: for example, an object with transform/body components and a sensor-like behaviour, and a stateful settings panel unrelated to Ant. These are focused reuse tests, not a new full example project or the start of Milestone 19. Future robots should compose bodies, wheels, sensors and lights using the same authoring model; this rework must support that extension path without prematurely implementing all robot features.

An extraction is complete only when its public contract is domain-neutral, its behavior is exercised, and Ant consumes it through the same API available to another project. Do not create speculative abstractions with no concrete consumer merely to appear general-purpose.

A developer creates a project and scene, creates objects from registered recipes, attaches components and behaviours, edits real component properties in the Inspector, and presses Play. PipeFrame owns lifecycle, storage, scheduling, physics, rendering, resources, input, and UI layout. Ant supplies ant-specific data, rules, assets, recipes, and views.

Use a Unity-inspired object/component/lifecycle authoring model backed by ECS storage and batch systems. Use Flutter-inspired declarative widget composition, constraints, state, and keyed reconciliation for UI. These are C++ PipeFrame APIs, not an attempt to embed either framework or reproduce their entire feature sets.

- One authoritative scene and object lifetime, one component registry, one scheduler, one public UI composition model.
- No milestone credit for moving files, introducing unused base classes, or adding empty contracts.
- Every new reusable abstraction must be exercised by Ant or a focused engine integration test before being called implemented.
- Components store state; behaviours and systems execute logic. Entity recipes assemble objects. UI views describe presentation.
- Public engine/editor authoring APIs and all Ant source contain no SFML types, includes, or backend-specific adapters. Backend code lives inside the engine implementation.
- Preserve existing Ant behavior and Pezzza reference expectations. Record and investigate existing divergences instead of treating current output as unquestionable truth.
- Migrate one complete vertical workflow at a time; remove superseded paths after the replacement passes its gate.

## 2. Original problems and current resolution

The original duplicate Ant/Colony owners and simulation dispatcher have been removed.
AntWorld composes runtime, physics and rendering worlds over one BehaviourScene. Schemas
live with components and drive the Inspector, validation and serialization. The editor
and Ant use the shared declarative UI API. Native window/render/widget implementations
remain inside Backend/SFML and are excluded from public authoring include paths. R6's
build/reload, source generation and environment workflow is verified. R7 audits removals,
current learning material, independent reuse and remaining lifecycle promises; see the
[audit evidence map](rework/R7_AUDIT.md).

## 3. Target runtime architecture

### Scene and object ownership

The engine Scene owns the ECS world, object hierarchy, component/behaviour instances, scene services, and lifecycle scheduling. Consolidate the existing scene APIs into this model; internal adapters may exist temporarily during migration, but projects get one public entry point.

Entity is an opaque, validated handle. Use generation-aware runtime identity so destroyed/reused objects cannot be accessed through stale handles. Persist a separate stable scene identifier for serialization and prefab references. SceneObject is a lightweight checked facade over the scene and entity, offering transform, component access, attachment, activation and destruction; it is not a second owner of state.

`Source/Entities` contains recipes such as AntEntity and ColonyEntity. A recipe derives from the engine EntityArchetype and composes registered components. It does not hold a parallel Ant object or run an update loop. Scene creation and recipe instantiation use the same structural command/rollback path.

### Components, behaviours, systems

Components use one registration/schema mechanism for storage, defaults, serialization, Inspector metadata and validation. Plain data components need no virtual base class; this is deliberate ECS design. Their mandatory contract is registration and schema, not an arbitrary class name. Components with resource ownership must use engine-managed handles and defined lifecycle operations.

Behaviour is the base for attached object scripts. It supplies owner/component access, activation, Start/Update/FixedUpdate, destruction, scheduled tasks/coroutines, and collision/trigger/visibility callbacks connected to actual engine services. Scripts are created from a template and registered automatically. Define callback ordering, disable/destroy semantics and task cancellation in tests.

System is the base for registered batch processing. Systems declare component access, phase and dependencies. Ant populations use batch systems rather than one virtual script update per ant. A colony or controller may use an attached Behaviour. A rule executes through exactly one of those paths.

Engine-owned shared components/services include Transform2D and hierarchy, identity/activation, motion/body/collider data, rendering/material/resource handles, camera, spatial queries, debug drawing, input actions, time, random streams, profiling and history buffers. Reuse existing working services before creating replacements. Ant-specific collision and locomotion rules remain configurable project logic; do not replace them with generic physics that changes behavior.

Fixed-step order is declared and deterministic: queued scene changes; lifecycle initialization; sensing/decisions; movement/physics; contact consequences and cleanup; metrics. Match the reference simulation's required phase ordering where it differs and document that ordering. Rendering and UI consume snapshots after simulation. Pause, single-step, speed, reset and stop belong to the engine.

### Properties and editor

One typed member-bound schema supplies field types, labels, ranges, units, enums, defaults, serialization versions and runtime setters. Build on ComponentSchema; do not add another metadata-only decorator system. Convenience macros may reduce syntax but must bind actual members.

Inspector edits use transactions with validation, undo/redo and explicit edit/play policies. Live-editable fields update the selected instance. Restart-required settings are labeled and applied through reset. Transform gizmos and numeric Inspector fields edit the same data. Telemetry is read-only. Adding a component supplies dependencies/defaults and rejects invalid combinations.

Create Behaviour generates the class, includes/registration and build integration; compile failures are shown without destroying the running editor. Successful rebuild/reload preserves scene data, recreates scripts, and cancels old callbacks safely. Add Component/Behaviour lists registered types. Creating a scene instance serializes an instance; creating a new script/type generates source. These are separate explicit editor actions.

## 4. Target UI architecture

Projects author StatelessWidget or StatefulWidget-style views through one backend-neutral API. Names may be adapted to avoid clashes, but there is one documented model. Build returns a widget tree; SetState marks the mounted subtree dirty and the engine schedules rebuilding. State ownership and subscriptions end on unmount.

The engine owns reconciliation, mount/update/unmount, stable sibling keys, focus, pointer capture, keyboard navigation, clipping, hit testing, text measurement, theming, layout and rendering. Invalid keys/layouts produce useful diagnostics. Changing a keyed widget's type replaces its retained element safely. Rebuilds must preserve focus and active edits when identity is retained.

Layout uses constraints flowing down and measured sizes flowing up. Provide Row, Column, Flex/Expanded, Padding, SizedBox, Stack, scrolling, text, buttons, toggles, sliders, numeric/text inputs, dropdowns, tabs, dialogs, tooltips, lists, charts and world-view integration. Reuse existing rendering implementations behind private adapters. A project must not position every label, manage sf::Font, refresh every button, or manually repair overlap.

Ant UI becomes reusable view classes: AntHud, ColonyPanel, AntInspectorView, EnvironmentToolsPanel, SimulationControls and ProfilerPanel. Engine editor UI uses the same controls. Docking/viewport/popup layers have clear ownership; collapsed handles and expanded panels cannot simultaneously intercept the same region. UI input blocks world tools only inside the appropriate visible interactive surface.

## 5. Ant migration map

| Current area | Required destination and responsibility |
|---|---|
| Ant.h/.cpp | Split authoritative state into AntIdentity/colony membership, Energy, Foraging, Steering, Encounter and procedural pose components; shared Transform/Motion/Body state belongs to engine components. Remove the monolithic Ant owner after migration. |
| Colony.h/.cpp | ColonyState and ColonySettings components, ColonyEntity recipe, and colony spawn/accounting behavior/system with no second world. |
| AntLeg, TrackingDirection | Pose data and pose/steering algorithms in explicit components/systems; extract shared math/interpolation only when reusable. |
| AntRole, AntState | Domain enums colocated with their owning component headers unless shared enough to justify a Types header. |
| AntAuthoring, simulation type IDs | One generated/registered module entry; schemas live with component declarations. Remove duplicate descriptor lists. |
| AntStore, AntSimulationWorld | Replace independent ownership with engine Scene queries/services; retain only domain-level scene setup if needed. |
| AntSimulationPipeline | Engine system registration/dependencies; remove the parallel update dispatcher. |
| Physics/World | Engine spatial grid, raycasts, body/contact services and field storage; Ant-specific markers, food rules and wall interaction remain domain logic. |
| Rendering | Engine render/resource submissions; Ant geometry/pose extraction systems generate data without backend calls. |
| AntDashboard and inspectors | Declarative views and schema-driven Inspector, no parallel manually synchronized model of editable state. |

Current Ant source contract (the World composition requested during R6 supersedes the
initial flat Systems/UI proposal):

```text
Source/
  Entities/       recipes
  Components/     registered state and component-owned schemas
  Behaviours/     attached colony/controller scripts
  Configuration/ project defaults and validated configuration
  Statistics/    project metric snapshots
  Editor/        declarative panels and domain brushes
  Runtime/       project host, AntRegistration and asset/UI integration
  World/
    AntWorld     composes the three engine world contracts
    Physics/     AntPhysicsWorld and its systems/solver
    Rendering/   AntRenderingWorld and its render layers
    Runtime/     AntRuntimeWorld, typed views and domain Systems/
  Plugin.cpp     project plugin entry
```

Scenes, assets and configuration remain outside Source. World modules own their ordered
steps; placing a file in a folder does not schedule it. Entity registration and component
schema registration have distinct responsibilities. See the [Ant source map](../examples/AntSimulation/README.md).

### Required naming and folder completion gate

Names must communicate the file's actual architectural role, consistently across engine APIs, official examples, and generated projects. This is required rework scope, not optional cleanup.

- Entities: `AntEntity`, `ColonyEntity` — composition recipes, not state containers.
- Components: `AntIdentityComponent`, `ForagingComponent`, `AntPoseComponent`, `ColonyStateComponent`, `ColonySettingsComponent` — small, registered data/schema types. Apply the same role suffix to shared engine components when normalizing the public authoring API (for example `EnergyComponent`).
- Behaviours: `ColonySpawnerBehaviour` — attached lifecycle logic.
- Systems: `AntForagingSystem`, `AntMovementSystem` — batch logic.
- UI: `ColonyPanel`, `AntInspectorView` — declarative view composition.
- Registration: a clearly named registration module under Runtime or the Plugin entry point, not miscellaneous authoring code in Components.
- Domain enums and small helper records belong with their owning component or an explicitly named shared types file. Generic algorithm helpers belong in engine services when reusable; they do not become components just because they are in that folder.

The R3 migration removed `Components/Ant.h/.cpp` and `Components/Colony.h/.cpp`; the replacement views borrow typed engine scene storage rather than owning those aggregates under new names. Registration/type IDs moved to Runtime, domain role/state/leg records are colocated with components, and direction tracking moved to the engine. The same conventions remain required for generated projects in R6.

Update class names, filenames, includes, CMake targets/sources, registration, templates, tests and documentation together for each migrated slice. Final acceptance requires no unexplained legacy aggregate or miscellaneous helper under Components, and generated names must follow the same convention. Generated-project conformance and the remaining UI migration are not implied by the Ant runtime cleanup.

## 6. Ordered execution and acceptance gates

### R0 — Baseline and scope

Inventory Ant responsibilities, reusable engine services, duplicate owners and backend leaks. Save deterministic Ant checkpoints, representative screenshots and interaction cases for colony creation, foraging, markers, walls, food, selection and reset. Record known reference divergences separately. Establish explicit engine/editor/Ant build targets and a test selection that excludes SailBoat, including its currently shared dashboard test location. Move Ant-specific dashboard tests to Ant or engine ownership before relying on them.

Gate: reproducible baseline and an Ant-only verification command. No simulation changes yet.

### R1 — One scene and lifecycle

Implement safe identities, SceneObject facade, hierarchy/activation, structural mutation rules, shared scheduler and lifecycle order. Consolidate existing scene APIs. Deliver create object → attach behaviour → fixed update → destroy in engine and in a minimal Ant scene.

Gate: stale handles, reentrant destruction, enable/disable, reset, pending commands and scene teardown tests pass; no second authoritative Ant world.

### R2 — Real components and properties

Documented colony-editing gate verified: [R2 implementation and acceptance](rework/R2_COMPONENT_EDITING.md).

Register common components and schemas; unify typed component storage with Inspector/serialization. Deliver ColonyEntity with editable Transform and ColonySettings, undo/redo, save/reopen and play/reset semantics.

Gate: editing actual colony data works end to end. Adding a new exposed field does not require a hand-written Inspector or a runtime property-name switch.

### R3 — Ant ECS migration and shared services

Migrate state/system slices in order: identity/transform; colony spawning and energy; steering/physics/contact; foraging/food/markers; pose/render extraction; history/metrics. Run relevant parity checks after each slice. Move reusable infrastructure to engine services. Remove obsolete stores, state copies and dispatchers as each slice is replaced.

Gate: entire Ant simulation runs from the engine Scene and registered systems; no monolithic Ant/Colony owner remains; performance and reference behavior are measured, with no unexplained regression. Set tolerances from R0 measurements rather than inventing passing thresholds later.

### R4 — Declarative UI runtime

Implementation and acceptance evidence: [R4 UI runtime](rework/R4_UI_RUNTIME.md). Full Ant/editor conversion remains R5.

Consolidate existing UI APIs; implement mounted widget lifecycle, automatic dirty rebuilds, constrained layout, reconciliation, scrolling and input ownership. Extend existing controls behind neutral interfaces. Verify nested stateful views and a schema-generated Inspector before full Ant migration.

Gate: resizing, text measurement, focus retention, key replacement, disabled controls, scrolling, popups and callback disposal pass interaction tests.

### R5 — Full Ant/editor UI migration

Implementation, reusable API examples and acceptance evidence: [R5 UI migration](rework/R5_UI_MIGRATION.md). Native host consolidation remains R6.

Rebuild the PipeFrame Editor and all Ant surfaces using the same public Flutter-inspired declarative API (`View`, `StatefulView`, `MountedView`), including charts and environment tools. The editor must serve as a documented reference application for project developers. Cover editor toolbars, hierarchy, schema-driven Inspector, asset browser, console, dialogs and dock content as well as simulation overlays. Add missing reusable controls to the engine first; do not introduce a separate editor-only UI framework or require projects to use private renderer/widget synchronization APIs. Document representative editor and Ant view composition, state updates, events and schema binding with source links. Preserve simulation visibility and reference controls. Remove the old imperative dashboard. Validate normal and narrow layouts, collapsed/expanded panels and visible hit regions at representative desktop sizes.

Gate: both editor and simulation UI use the same public declarative composition, state, layout and input contracts; editor source and learning documentation demonstrate this authoring path. Long Inspectors must retain scrolling through live refresh, clip content, expose overflow and keep lower fields reachable at short dock heights. Saved screenshots plus interaction evidence for every panel and tool; no accidental overlap or inaccessible controls; project UI contains no backend types or manual widget synchronization loops.

### Performance gate before R6

Resolve the editor/Ant frame-time regression with measured paused scrolling, dragging and matched 1/3/5-colony workloads. Functional UI acceptance alone is insufficient. [Historical corrections](rework/UI_PERFORMANCE.md). Package 7 now passes its final three matched repetitions after contact-workspace reuse; see [measured results and limits](rework/R6_PERFORMANCE_GATES.md).

### R6 — Authoring workflow and backend boundary

Follow the [eight-package R6 execution checklist](rework/R6_EXECUTION_PLAN.md), including
the environment extension below. Previously verified build/render work is retained.

Component authoring follows the [component-owned schema convention](rework/COMPONENT_SCHEMA_CONVENTION.md): declarations and validation live in `Component::Schema()` beside the data, and registration only consumes schemas. Keep generated files and future examples consistent with this contract.

Finish Create Scene/Object/Component/Behaviour, template generation, registration, compile/reload, component attachment, prefab/save/load and scene validation. Port remaining public/editor backend dependencies into engine-private adapters. Maintain dependency checks throughout R1–R5; do not defer all isolation to this stage.

Gate: create a SoldierAnt type in the editor, attach behavior and exposed settings, instantiate it, run it, modify/save/reopen it without editing engine source or manually locating registration files. This proves extension mechanics; new soldier simulation behavior must be explicitly specified rather than assumed to match Pezzza.

### R6-E — Environment authoring from an empty project

Required scope added September 12, 2026; authoring implementation complete September 13.
**R6 is complete / verified September 13**: [final evidence](rework/R6_FINAL_PERFORMANCE.md).
Package 2 is complete: [Playground/material/tileset/tilemap assets](rework/R6_VISUAL_ASSETS.md).
Package 3 is complete: [built-in environment editing workflow](rework/R6_ENVIRONMENT_EDITING.md).
Package 4 is complete: [developer tools and custom Ant brushes](rework/R6_CUSTOM_BRUSHES.md).
Package 5 is complete: [shared environment collision and queries](rework/R6_ENVIRONMENT_COLLISION.md).
Package 6 is complete: [Ant editor-authored environments](rework/R6_ANT_ENVIRONMENT_AUTHORING.md).
Package 7 is complete: [final measured performance gates](rework/R6_FINAL_PERFORMANCE.md).
Package 8 is complete: [blank-project acceptance and learning documentation](rework/R6_BLANK_PROJECT_ACCEPTANCE.md).
[Detailed environment contract and acceptance](rework/R6_ENVIRONMENT_AUTHORING.md).
Deliver bounded playground creation from a rectangle, extensible public tool/brush
lifecycle with custom Ant brushes, line/ruler/drag-rectangle gestures, editable
Ground/Tilemap objects, texture/material/tileset assets, painting and
layers, collision/query integration, obstacle/spawn/goal placement, persistence and
Ant consumption. PNG maps are an optional import path. Validate a blank non-Ant scene
and a newly authored Ant map. This is a 2D environment foundation; true 3D terrain is
not implied. Milestone 19 must consume this foundation rather than build a bespoke map editor.

Public UI/editor header isolation is complete; see [package-1 evidence](rework/R6_PUBLIC_UI_ISOLATION.md).
The final ABI 12 contact-workspace optimization passes all three performance runs
(five-colony p95 16.356 / 16.185 / 16.491 ms). Final Debug and Release pass 77/77
tests each. This is the historical R6 checkpoint; R7 final verification below supersedes its ABI and aggregate counts.

### R7 — Removal, documentation and completion audit

**Verified September 13:** [final audit, migration guide and evidence](rework/R7_AUDIT.md).
Final ABI 13 builds pass 79/79 Debug and 79/79 Release checks. All three matched
performance repetitions pass; five-colony p95 is 16.420 / 16.125 / 16.569 ms.

Delete superseded APIs/adapters within scope, update generated templates and Ant documentation, publish a small developer walkthrough, and run engine/editor/Ant verification only. Audit remaining direct and transitive backend exposure. Review each new base class/service for a real consumer.

Gate: all below completion scenarios pass on the same revision. Prior milestone claims and aggregate test counts cannot substitute for these gates.

R1 → R2 → R3 establishes simulation ownership. R4 can be developed once R1 ownership is settled; R5 depends on R2–R4. R6 completes the authoring path; R7 closes the rework. Re-estimate effort after R0; do not promise completion dates without measuring scope.

## 7. Final completion demonstration

1. New project → new scene → author ground/tilemap, materials, walls and obstacles inside the editor → save/reopen → colony and food → Play, pause, step, speed and reset. No external PNG or hardcoded map is required; the same environment workflow also passes in a non-Ant fixture.
2. Select colony and an individual ant; edit supported real properties; undo/redo; serialize and reopen correctly.
3. Generate and attach a new behavior with an exposed field through the editor; build/reload safely, including failure recovery.
4. Compose a new nested UI panel using state and standard controls without coordinates, backend objects or engine edits.
5. Resize, scroll, open/close panels, use keyboard and pointer; UI and world interactions never steal each other's input incorrectly.
6. Spawn/destroy large populations; verify lifecycle safety, deterministic/reference checks, and agreed performance limits.
7. Build Ant without including backend headers in its authoring source. Backend dependency checks have no Ant allowlist exceptions.
8. Follow documented Ant structure to find data, rules, recipes, UI and registration without knowing legacy file names.
9. Build and run a minimal independent engine fixture with registered components, an attached behaviour, and a stateful UI panel without linking Ant. Verify that generated project templates expose the same APIs and contain no Ant-specific assumptions.

## 8. Progress reporting

R0–R7 acceptance is verified within this rework's stated scope. R0 baseline/reference
provenance is retained in the R3 audit and parity reference inventory; the final audit
records known differences rather than claiming perfect Pezzza equivalence. R1 scene
ownership/lifecycle, R2 properties, R3 migration, R4/R5 declarative UI, R6 authoring/backend
isolation and R7 cleanup/reuse are mapped to concrete evidence in the
[R7 report](rework/R7_AUDIT.md). Historical checkpoints remain in the
[progress log](rework/ANT_REWORK_PROGRESS.md). Update this plan when scope changes.
Do not resume SailBoat work/tests unless the user asks. Milestone 19 remains separate.


R3 implementation and evidence: [runtime audit](rework/R3_RUNTIME_AUDIT.md). Full reference/visual equivalence is not claimed by the runtime migration.

## Default sprite and physics follow-up (2026-09-13)

See [shared sprite and kinematic authoring](rework/SPRITE_AND_KINEMATIC_AUTHORING.md)
for the generated-project defaults, custom rendering extension path, editor workflow
and explicit limits before Milestone 19. This provides textured entities and box
kinematic collision; force-driven vehicle dynamics are not implied.

### Generated world structure

New Project now generates explicit World/Runtime, World/Physics and World/Rendering modules derived from PipeFrame bases, with the shared editor scene and default simulation/render paths wired through them. See [Generated project worlds](rework/GENERATED_PROJECT_WORLDS.md) for scheduling, custom solvers, texture rendering and migration scope.

### Shared world visualization

PHYSICS/MESH viewport controls now request project-supplied debug geometry through
`ProjectRuntime::CollectWorldDebug`. Ant's physics/rendering worlds supply their
actual bodies, wall boundaries and triangle geometry. New world templates expose
the same hooks alongside default scene geometry. Source/Runtime remains explicitly
documented as the editor integration boundary. See GENERATED_PROJECT_WORLDS.md.

### Ant outer runtime responsibility cleanup

AntWorld now constructs authored simulation worlds; AntRenderingWorld owns renderer
assets/options, geometry and draw ordering; AntEnvironment owns food-radius queries.
Dashboard hosting moved to Editor. Runtime retains editor/host integration and
registration, with existing public interfaces and project paths preserved.

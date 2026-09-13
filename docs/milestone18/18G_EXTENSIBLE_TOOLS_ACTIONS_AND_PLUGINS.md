# 18G — Extensible tools, actions, and plugins

Status: **reopened (2026-09-11)**.

The first completion claim was incorrect. Registration callbacks and folder
names were implemented, but the reference projects were not structurally
migrated. The current remediation begins with public simulation and editor-tool
contracts: `AntSimulationPipeline`, `ColonyLifecycleSystem`, and `AntAvoidanceSystem` implement
`FixedUpdateSystem`; `ContactSolver` implements `PhysicsSolver`; `BoatMovementSystem`
implements `EntityUpdateSystem` with an explicit `BoatUpdateCommand`; and
`AntEditorTool` implements `EditorTool`. Ant and SailBoat domain state, world,
physics, behavior, colony, training, race, and updater code now uses PipeFrame
math/color types; SailBoat's backend-bearing source fell from 16 files to 5.
18G remains open until the remaining rendering/dashboard/runtime boundaries use
PipeFrame submissions and reference-project `Source/` contains no SFML or
PipeFrame backend adapter references.

## Public plugin contract

`PipeFrame/Project/PluginAPI.h` is the backend-neutral extension and system API.
Every runtime reports a stable plugin ID, display name, ABI version, plugin
version, reload-state schema, and whether PipeFrame's scheduler owns its update
and render phases. Workbench rejects incomplete registrations, ABI mismatches,
state-schema mismatches, duplicate IDs, invalid dependency graphs, action
conflicts, and lifecycle violations before activating a replacement runtime.

`PluginRegistrar` registers extensions, actions, and systems under the plugin's
owner ID. The extension registry supports panels, drawers, tools, gizmos, menus,
commands, overlays, importers, settings, part categories, attachment
compatibility, connection types, environment brushes, validation rules,
telemetry streams, and simulation debug overlays. Workbench discovers these
records from `ProjectRuntimeHost`; no project-specific branch is required.
Every extension record provides an executable callback or, for an Inspector
drawer, an executable presentation callback. `ExtensionRegistry::Invoke`
isolates exceptions and reports the failing stable ID. Project actions use the
same attributed failure boundary.

The standard tools dock now includes **Commands** and **Extensions** tabs.
Commands can be searched by stable ID, display name, or category, filtered by
context, and invoked from generated rows. Shortcut rebinding rejects conflicts
only when actions can be active in the same context. The Extensions tab lists
the loaded project's registered extensions and systems. The toolbar exposes a
single **Reload** action for the active project runtime.

## Registered system lifecycle

Systems declare a stable ID, owner, lifecycle phase, dependencies, component
reads and writes, parallel-safety status, editor-only status, structural-change
use, and an execution callback. The phases are load, start, fixed pre-physics,
fixed physics, fixed post-physics, fixed behavior, fixed cleanup, variable
update, render preparation, render, and editor update.

The scheduler topologically orders dependencies and rejects:

- duplicate or missing system IDs;
- cyclic or later-phase dependencies;
- runtime dependencies on editor-only systems;
- editor-only systems placed in runtime phases;
- unordered systems that write the same component data; and
- structural scene mutations during render phases.

Each `SystemContext` provides component queries, a deferred structural command
buffer, a typed service registry, fixed/variable time and tick number, events,
backend-neutral render submissions, deterministic random streams, profiling,
deterministically ordered jobs, and project logging. Spatial, physics, resource,
and renderer services are supplied through the typed service registry, keeping
the contract independent of a concrete backend. Exceptions from project actions
or systems are caught at the plugin boundary and attributed to their stable ID.

Systems declare their named PipeFrame service requirements. Registration is
rejected before activation when the host cannot provide one; execution also
checks the corresponding direct context pointer. SimulationWorkbench owns and
provides the shared authored-scene spatial index, PhysicsWorld2D, graphics
resources, render surfaces, events, deterministic random/job queues, profiler,
and project log. Scene synchronization rebuilds the shared spatial index from
authored transforms.

Deferred create, remove, replace, and component-enabled commands are converted
to Workbench scene edits after a system phase. They therefore pass through the
normal document transaction, selection, serialization, and undo path instead
of modifying the hierarchy while it is being queried.

## Safe reload

Runtime reload stages a uniquely named copy of the rebuilt library, validates
its descriptor and complete registration graph, initializes it, restores the
authored scene, selection, view mode, simulation state, and versioned plugin
state, and only then unloads the previous runtime. A missing, incompatible, or
partially registered replacement leaves the active runtime untouched.

Action and system callbacks are destroyed while their owning library remains
loaded. This ordering is essential because `std::function` managers may contain
code from the plugin. Regression coverage includes the partial-registration
failure that originally exposed this lifetime boundary.

## Reference-project migration

BasicSimulation, AntSimulation, and SailBoatSimulation now use the standard
`Source/Plugin.cpp` entry point, stable plugin descriptors, module manifests,
standard `Config`, `Assets`, `Scenes`, `Source/Components`, `Source/Systems`,
`Source/Runtime`, `Source/Editor`, and `Tests` roots, registered extensions, and
PipeFrame-scheduled runtime/render phases.

Each project contains versioned ProjectSettings, Input, Physics, and Modules
configuration plus compiled `ComponentContract`, `SystemContract`,
`RuntimeContract`, and `ExtensionContract` modules in the standard source
roots. Its manifest names those modules, and every standard asset category
exists.

- Basic registers simulation and render systems.
- Ant registers simulation, live-state, and render systems plus its dashboard,
  world brush, selection tool, debug overlay, telemetry, settings, and commands.
- SailBoat registers sensing/preview, training and network evaluation, race
  presentation, and render systems plus its dashboard, race editor, network,
  telemetry, validation, and commands. Simulation time remains advanced by the
  sensing phase and visible in the existing dashboard.

Domain algorithms remain inside their projects. Project sources are checked to
ensure they do not include or depend on SimulationWorkbench internals.

The scheduled examples query PipeFrame components and the shared authored-scene
spatial index, dispatch deterministic jobs, use deterministic random streams,
publish events, write project logs, receive automatic profiling, and resolve
resource/render services through the host. Ant retains its specialized dense
pixel-wall/contact solver because marker fields, wall masks, and colony contacts
need domain data absent from a generic rigid-body world; Ant physics, contact,
behavior-parity, and stress regressions guard that choice. SailBoat retains its
polar-table integrator because apparent wind and sail response are the domain
algorithm being simulated; movement, race-task, training, and performance
regressions guard it.

## Remaining closure gates

- Move all Ant and SailBoat math, input, resource, render, and UI interaction to
  backend-neutral PipeFrame APIs.
- Replace raw SFML render-target drawing with PipeFrame render submissions and
  move any necessary conversion into `PipeFrame/Backend/SFML`.
- Convert remaining lifecycle-bearing updater, behavior, physics, renderer, and
  editor classes to the matching PipeFrame contract or document why a class is
  a plain value/domain type.
- Split runtime-owned orchestration into real registered system objects rather
  than descriptor lambdas that call monolithic runtime methods.
- Add a conformance scan that fails on SFML/backend symbols and missing required
  PipeFrame base contracts in Basic, Ant, and SailBoat project source.

## Verification to retain

`PluginAPIRegression` executes every extension family and verifies attributed
extension/action/system failure isolation, command search, context-aware
shortcut rebinding, declared-service rejection, deterministic jobs, component
access, events, command buffers, profiling, dependency ordering, conflict
detection, and lifecycle validation.

`ReferenceProjectConformance` loads all three runtime libraries; rejects missing
configuration, asset roots, placeholder-only module folders, and uncompiled
contract modules; validates ABI, plugin entry point, executable extensions,
actions, declared system services, and Workbench independence; then executes
the headless Basic scheduled pipeline with host-equivalent services.
`WorkbenchUIAcceptance` runs Basic, Ant, and SailBoat through the real host and
scheduler, invokes every extension family from an external plugin, and confirms
successful, missing-file, and partial-registration reload behavior.

The 18G checkpoint passes the complete Debug build and all 70 registered tests,
including Ant/SailBoat behavior, dashboard, deterministic, performance, render,
and stress coverage.

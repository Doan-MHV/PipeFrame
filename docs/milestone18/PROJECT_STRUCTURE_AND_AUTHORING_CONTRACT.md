# Milestone 18 project structure and authoring contract

**Ant rework amendment (September 12):** Ant's simulation systems are now grouped beneath
`Source/World/{Runtime,Physics,Rendering}` and owned by typed world modules. The current
layout and inheritance/lifecycle contract are documented in
[Ant world composition](../rework/ANT_WORLD_COMPOSITION.md). The general scaffold layout
below remains the existing generator output; world-composed starter generation is not yet
implemented.

Status: required for Milestone 18 completion; implementation gaps tracked in [integration status](UNITY_FLUTTER_INTEGRATION_STATUS.md)

PipeFrame projects may contain additional folders, but generated projects and
official examples use this structure so components, systems, runtime code,
editor extensions, assets, scenes, configuration, and tests remain predictable.

```text
MyProject/
├── project.pipeframe
├── CMakeLists.txt
├── Config/
│   ├── ProjectSettings.pipeframe
│   ├── Input.pipeframe
│   └── Physics.pipeframe
├── Assets/
│   ├── Audio/
│   ├── Materials/
│   ├── Models/
│   ├── Prefabs/
│   ├── Shaders/
│   └── Textures/
├── Scenes/
├── Source/
│   ├── Entities/
│   ├── Behaviours/
│   ├── Components/
│   ├── Systems/
│   ├── Runtime/
│   ├── Editor/
│   └── Plugin.cpp
└── Tests/
```

`Entities` contains composition recipes derived from `EntityArchetype`; the ECS world owns identities. `Behaviours` contains attached `Behaviour` scripts with engine-dispatched lifecycle hooks. `Components` contains serializable data and metadata. `Systems` contains domain
behavior operating on component queries. `Runtime` composes systems and owns the
project lifecycle. `Editor` contains optional drawers, tools, gizmos, importers,
and overlays. `Plugin.cpp` is the single registration entry point. Build output
and imported caches stay outside authored source folders.

Dependency direction is enforced: Components contain backend-neutral data;
Systems may depend only on Components and public PipeFrame APIs; Runtime composes
systems; Editor may depend on Runtime metadata, but packaged Runtime code must
not depend on Editor or Workbench. Project code does not include SFML or another
backend directly. Backend adapters remain inside PipeFrame.

## PipeFrame-provided project SDK

PipeFrame provides the component registry and storage, archetype registration,
typed queries, structural command buffers, system registry, dependency-ordered
scheduler, lifecycle phases, fixed and variable time, deterministic random and
jobs, spatial queries, physics access, resource handles, render submissions,
events, logging, profiling, serialization/migrations, undo transactions, plugin
loading, and editor metadata bridge. Projects consume these public services;
they do not recreate the framework layer.

Projects provide domain components and rules. Ant owns foraging, pheromone,
avoidance policy, colony behavior, and its presentation. SailBoat owns boat/race
rules, controller inputs, neural-network evaluation and training policy, and its
presentation. PipeFrame owns their scheduling, data access, common physics and
spatial operations, resource/render plumbing, lifecycle, diagnostics, and
editor integration.

## Property exposure

The canonical API is a typed descriptor/builder registered by the project
plugin. Every component and field has a stable serialization ID, display name,
type, default, version, editability, units, constraints, enum choices, and
editor hint. The Inspector consumes this metadata without project-specific
branches.

Milestone 18 also supplies optional Unreal-style convenience macros such as
`PF_COMPONENT` and `PF_PROPERTY`. They generate or register the same typed
descriptors; they must not expose raw memory offsets, mutate arbitrary object
memory, require compiler-specific reflection, or become a second schema. A
project can use the builder directly when macros are undesirable.

Transform, Identity, Physics Body 2D, Collider 2D, and Renderer 2D use the same
registration and Inspector path as project components. Transform remains
required. Other common components are optional unless an archetype requires
them. Metadata changes preserve stable IDs and provide explicit migrations.

## System structure and lifecycle

PipeFrame owns scheduling and services. Projects register systems and declare
their phase, component access, dependencies, ordering constraints, and whether
parallel execution is safe. The standard lifecycle is initialize, fixed update,
variable update, late update, render preparation, and shutdown. Physics and
simulation behavior use fixed update; editor-only systems do not enter packaged
runtime builds unless explicitly requested.

Systems receive a restricted context for component queries, commands, time,
spatial queries, physics, rendering submissions, resources, events, profiling,
logging, deterministic random streams, and deterministic jobs. Structural
changes use command buffers so component iteration remains safe. Systems do not
reach into Workbench internals or own a
second main loop. Project code keeps domain policy: an Ant movement system owns
ant decisions, while PipeFrame supplies scheduling, spatial search, physics,
jobs, resources, rendering, and profiling.

The registry validates duplicate IDs, missing dependencies, dependency cycles,
invalid phase access, conflicting writers, unavailable services, and runtime/
editor boundary violations. Stable system IDs control ordering, saved settings,
profiling, enablement, and migration.

## Creation and enforcement

Workbench provides New Project templates for empty simulation, 2D physics,
learning simulation, and modular assembly projects. Creation generates the
standard folders, manifest, build target, plugin registration file, starter
scene, example component/system pair, and tests. An Add Component/System/Editor
Extension action places files in the standard folder and updates registration.

A project validator and CI conformance test check manifests, duplicate IDs,
schema migrations, forbidden backend dependencies in public/project APIs,
unregistered components and systems, dependency cycles, missing assets, and
editor/runtime separation. Custom layouts remain allowed, but the manifest must
declare source and asset roots so Workbench can discover them consistently.

### Object creation versus code generation

Scene objects are instances, so **Create Object** never creates a C++ class per
object. A reusable data-only type is created with **Create Object Type**, which
stores a registered archetype/prefab made from components, defaults, and assets.
This is the normal workflow for variants such as a Soldier Ant, a scout, a
different boat setup, or a robot assembled from existing parts.

New C++ is needed only for a new component schema, algorithm/system, runtime
module, or editor extension. Workbench owns generators for each case. A
generator places code in the standard folder, emits the typed descriptor and
registration skeleton, updates build and plugin registration metadata, runs
validation, builds, and hot reloads. The user is not expected to search the
documentation to discover a base class or edit CMake and `Plugin.cpp` by hand.
Generated project code uses public PipeFrame APIs and composition; inheritance
is introduced only when the selected public extension contract requires it.

## Required example migration

BasicSimulation, AntSimulation, and SailBoatSimulation are first-party
conformance projects. Milestone 18 migrates each to this layout and the same
component/system/plugin contracts generated for external users. Ant updater
orchestration becomes registered systems. SailBoat movement, sensor/network
evaluation, race logic, simulation timing, and training orchestration enter the
shared lifecycle while their domain algorithms stay project-owned.

The migration is structural and must not erase Milestone 17 work. Existing
behavior, UI reference parity, controls, SailBoat network and simulation-time
views, deterministic fixtures, saved scenes, and performance targets remain
acceptance gates. Temporary adapters are allowed only with an owner, removal
phase, and regression coverage; permanent duplicate engine services are not.

Public project APIs use explicit ownership and lifetime rules, stable IDs, and
versioned schemas. Hot reload first validates registrations and migrations,
captures editor/runtime state, unloads old code, loads the replacement, migrates
data, restores state, and reports failures without corrupting the open scene.

## Phase ownership

| Requirement | Milestone phase |
|---|---|
| Stable scene/component schemas and plugin registration foundation | 18B, complete |
| Generated Inspector and property exposure convenience macros | 18C, complete |
| Asset source/import/cache conventions | 18D, complete |
| Prefab and reusable assembly code/data conventions | 18E, complete |
| Standard panels and visible project workflows | 18F, complete |
| System registry, lifecycle, service context, extensions, and hot reload | 18G, complete |
| Basic, Ant, and SailBoat migration to the standard public contract | 18G, complete |
| Project generators, build/package flow, validation, CI, and documentation | 18H |

## Completion evidence

Milestone 18 is not complete until a newly generated external project can:

1. add an exposed component using the builder or macro convenience layer;
2. see and edit it beside Transform and Physics in the generated Inspector;
3. register a fixed-update system without implementing a custom application loop;
4. query components and use PipeFrame spatial, physics, resource, job, render,
   event, and profiling services through the system context;
5. build, reload, serialize, migrate, undo, run, package, and pass project
   validation without modifying Workbench or engine source.
6. demonstrate the same structure and APIs in Basic, Ant, and SailBoat while all
   Milestone 17 parity and performance evidence continues to pass.
7. create a data-only object type, instantiate it, and edit its components
   without generating C++ or choosing an inheritance base;
8. generate a new component and system, build and hot reload them, and expose
   them in Workbench without manual CMake or registration edits; and
9. start Milestone 19 work through the public editor and library contracts, then
   update PipeFrame or Workbench when dogfooding exposes a bug, missing feature,
   awkward API, broken interaction, or poor design. The fix must add regression
   coverage and replace any project workaround before acceptance.

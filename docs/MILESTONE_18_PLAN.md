> September 12 rework clarification: environment authoring must pass the explicit
> [R6-E blank-project workflow](rework/R6_ENVIRONMENT_AUTHORING.md) before robotics.
> Existing environment/asset mentions below are requirements, not proof of completion.
> The governing engine rework remains Ant-first; SailBoat migration/tests stay excluded.

> Scope update (2026-09-11): [Engine rework — Ant plan](ENGINE_REWORK_ANT_PLAN.md) governs remaining engine/editor work. Ant is the sole reference project; SailBoat implementation and tests are excluded. Earlier completion statements do not establish completion of this rework.

# Milestone 18 — Production editor foundation

Status: active; not complete. Milestone 18A is complete, 18H is in progress,
and several acceptance gates previously labeled complete still need additional
implementation. See
[milestone18/MILESTONE_18_COMPLETION_AUDIT.md](milestone18/MILESTONE_18_COMPLETION_AUDIT.md).
Milestone 17 extracts runtime systems and completes Ant and
SailBoat parity. Milestone 18 turns SimulationWorkbench into a reusable,
project-extensible editor. It must also provide the authoring contracts needed
by Milestone 19's modular robotics simulation without embedding robot-specific
logic in Workbench. Its source checklist is the
[reusable systems inventory](PIPEFRAME_REUSABLE_SYSTEMS_INVENTORY.md). The
required project layout, property exposure API, and system lifecycle are defined
in [milestone18/PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md](milestone18/PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md).

## Editor experience target

The Workbench must reach the interaction quality and information architecture
expected from editors such as Unity and Unreal: a central scene viewport,
discoverable hierarchy and asset navigation, a metadata-driven inspector,
dockable tools, predictable selection and transform controls, clear simulation
state, contextual actions, saved workspaces, and readable diagnostics. This is a
workflow and quality reference, not a requirement to copy either product's
branding or exact visual design.

Every Milestone 18 phase includes UI work where its capability becomes visible.
Completion requires a coherent shared design system for typography, spacing,
icons, colors, focus, hover, selection, disabled states, validation, tooltips,
menus, dialogs, scroll behavior, and keyboard navigation. Panels and overlays
must never cover essential controls accidentally or intercept input while hidden.

## Milestone 19 readiness contract

Milestone 18 owns the generic editor mechanisms needed to assemble a robot from
parts. Milestone 19 owns robot parts, vehicle physics, electrical behavior,
sensors, controllers, and autonomy examples. The editor foundation must support:

- explicit project units and coordinate conventions for distance, angle, mass,
  time, velocity, force, voltage, and current;
- typed attachment points and compatibility rules so chassis, wheels, motors,
  servos, sensors, lights, and controllers can be placed and connected safely;
- component and asset metadata for part categories, physical bounds, collision
  geometry, preview images, configuration limits, and project-defined icons;
- authored mechanical, power, and signal connections with stable endpoint IDs,
  validation, hierarchy visualization, and undo/redo;
- environment construction from walls, floors, obstacles, paths, line-tracking
  surfaces, lights, spawn points, and navigation targets;
- live, read-only telemetry and registered viewport overlays for rays, scans,
  contacts, wheel forces, paths, signal values, and sensor ranges; and
- reusable assemblies and templates so a reference kit is one configuration of
  the modular system rather than the editor's fixed object model.

Milestone 19 is the mandatory external-user dogfood test for Milestone 18. Each
robotics feature begins through the released Workbench workflows, generated
project structure, and public PipeFrame libraries. It may add robot domain
components and systems. When that work exposes an engine/editor bug, missing
capability, confusing API, broken control, inefficient workflow, or poor design,
Milestone 19 must update and retest PipeFrame or Workbench rather than hide the
problem in project-specific code. Repeated project-side code for scheduling,
serialization, physics/spatial access, asset handling, rendering submission,
telemetry, undo, registration, or build/reload is evidence of a missing shared
facility and must be promoted into PipeFrame before Milestone 19 is accepted.

These discoveries reopen the relevant Milestone 18 acceptance gate for
hardening. The fix includes engine/editor regression coverage, documentation,
and verification in the existing Basic, Ant, and SailBoat projects before the
robotics work continues. Milestone 18 remains the foundation specification;
Milestone 19 is allowed and expected to improve its implementation.

## Object, type, and code creation workflow

Workbench must make the common path data-driven. **Create Object** creates a
scene instance from a registered object type, prefab, or archetype; it does not
generate a C++ class for every instance. **Create Object Type** lets a user name
a reusable type, choose components, set defaults, assign assets, and save it as
a discoverable archetype/prefab without writing or compiling C++.

For behavior that cannot be composed from registered systems, the editor
provides **Add Component**, **Add System**, **Add Runtime Module**, and **Add
Editor Extension** wizards. These actions:

- ask for a stable ID, display name, namespace, fields/services, lifecycle
  phase, and destination module;
- generate files into the standard `Source/Components`, `Source/Systems`,
  `Source/Runtime`, or `Source/Editor` folders;
- generate typed property descriptors and serialization defaults;
- update the project registration entry point and build manifest safely;
- validate names, IDs, dependencies, schemas, and backend-neutral includes;
- build and hot reload the project with errors linked to the generated file;
- make the new component, system, drawer, or object type immediately searchable
  in Workbench; and
- support undo/removal of generated registration while refusing to delete
  user-modified source silently.

The generated code uses composition instead of requiring users to discover an
inheritance hierarchy. For example, a Soldier Ant should normally be an
archetype/prefab composed from `Ant`, `ColonyMember`, a role value such as
`Soldier`, and optional combat/sensing components. Existing registered systems
then process that data. If Soldier Ant needs a new algorithm, **Add System**
generates the correctly registered system and query skeleton; the user fills in
only the domain rule.

## 18A — Viewport grid, selection, snapping, and gizmos

Status: complete. The generic viewport now supplies the grid, rulers,
measurements, direct transform handles, snapping, pivots, selection modes,
placement workflow, registered previews and overlays, camera tools, undo/redo,
responsive layouts, and stored visual evidence required by this phase. The
implementation and acceptance evidence are recorded in
[milestone18/18A_BASELINE.md](milestone18/18A_BASELINE.md).

- Add adaptive major/minor grid lines, axes, rulers, and measurements.
- Add grid/position/rotation/scale snapping with visible controls.
- Add hover, box, single, additive, and subtractive selection.
- Add move/rotate/scale gizmos, local/world space, and pivots.
- Add frame-selection/scene, bookmarks, editor/game cameras, drag/drop creation,
  placement previews, context actions, and registered debug overlays.
- Show attachment points, connection previews, measurement tools, collision
  bounds, sensor range previews, and project-registered placement constraints.

Acceptance: a generic project can create, select, multi-select, transform, snap,
undo, and redo through the viewport. Grid and gizmos remain usable across zoom,
DPI, and supported resolutions.

## 18B — Component scene model and hierarchy

Status: complete. The version-6 component scene model, stable hierarchy and
connections, scene settings, transactional commands, filtering, scene
workspace, migration path, plugin registration, and hot-reload preservation
across Basic, Ant, SailBoat, and an external component plugin are verified. See
[milestone18/18B_COMPONENT_SCENE_MODEL.md](milestone18/18B_COMPONENT_SCENE_MODEL.md).

- Add parent-child objects, inherited transforms, stable references, components,
  runtime/editor-only components, layers, tags, locking, and visibility.
- Add typed attachment points and stable mechanical, power, and signal
  connections. Connections reference endpoints rather than array positions and
  report missing, incompatible, duplicate, and cyclic links where applicable.
- Replace the special-case `SceneObjectData::transform` plus flat project
  property map with a versioned component schema. PipeFrame supplies common
  `Transform2D`, `PhysicsBody2D`, collider, renderer, and identity components;
  projects register domain components without changing Workbench.
- Provide a typed C++ component-registration builder for fields, defaults,
  constraints, units, serialization IDs, and editor hints. Do not depend on
  compiler-specific decorators, unsafe field offsets, or SFML types.
- Define project-level units and coordinate conventions in the scene schema and
  preserve them through serialization and migration.
- Make component presence explicit per object archetype: Transform is common,
  while Physics and other components remain optional and addable/removable when
  the schema permits it.
- Add rename, reparent, reorder, duplicate, delete, group, search, and filtering.
- Add scene validation, templates, duplication, multiple scenes, and additive
  loading.

Acceptance: nested transforms, common and project-defined components, and
references serialize, migrate, undo, reload, and survive plugin reload;
viewport/hierarchy selection agrees. A new plugin can register a domain
component and attach it to an archetype without Workbench source changes.

## 18C — Inspector and transactional authoring

Status: complete. The Inspector is generated from typed object/component
metadata, shared and project components use the same path, and validated edits
support foldouts, live telemetry, mixed values, batch undo, defaults, and typed
copy/paste. Runtime component commands preserve physics authority across
stopped, paused, and playing states. Plugin drawer callbacks provide compact
presentations through stable editor hints, and Ant/SailBoat settings migrate to
registered domain components when their projects open. See
[milestone18/18C_INSPECTOR_AND_TRANSACTIONAL_AUTHORING.md](milestone18/18C_INSPECTOR_AND_TRANSACTIONAL_AUTHORING.md).

- Generate all standard property editors from metadata, including assets and
  object references; add units, validation, defaults, foldouts, and custom drawers.
- Generate editors for attachment selection and typed endpoint connections, and
  let plugins provide compact previews for part geometry, sensing range, and
  actuator limits.
- Generate a collapsible section for every registered component, including the
  shared Transform and Physics components. Support boolean, integer, number,
  string, vector, color, enum, range, asset, object-reference, and read-only
  telemetry fields instead of hardcoding Transform controls in `InspectorPanel`.
- Define transform/physics authority explicitly: the authored Transform drives
  body creation while stopped; the physics body drives the displayed runtime
  pose while playing; paused edits issue validated teleport/body-update commands
  rather than mutating physics storage directly.
- Route runtime physics snapshots back to read-only inspector values and keep
  authored values separate from transient simulation state. Committing a
  permitted runtime edit must update the scene, physics body, dirty state, and
  undo history as one transaction.
- Add multi-selection, mixed values, copy/paste, batch edits, and live inspection.
- Route every mutation through coalesced/cancellable undoable transactions.
- Add optional `PF_COMPONENT`/`PF_PROPERTY` convenience macros that generate the
  canonical typed descriptors. Keep the builder API authoritative and prohibit
  raw field offsets, compiler-specific reflection, and direct editor mutation of
  arbitrary C++ objects.

Acceptance: every property type round-trips and plugin-defined drawers require no
Workbench source changes. Transform and Physics appear through the same metadata
pipeline as project components; stop/play/pause transitions never leave scene
and physics poses inconsistent, and undo/redo restores both authored state and
the corresponding runtime body state.

## 18D — Asset database, browser, and import pipeline

Status: complete. Stable persistent asset IDs, versioned metadata, dependencies,
built-in importers, deterministic cache output, previews, generic part metadata,
search/filtering, visible/cancellable operations, reimport, assignment, and
missing-source/reference repair are implemented. See
[milestone18/18D_ASSET_DATABASE_AND_IMPORT_PIPELINE.md](milestone18/18D_ASSET_DATABASE_AND_IMPORT_PIPELINE.md).

- Add stable asset IDs, metadata, dependencies, importer versions, cached output,
  folders, search, thumbnails, previews, operations, and reimport.
- Add texture, audio, shader, material, model, and scene importers.
- Add generic part-asset metadata: category, attachment definitions, physical
  bounds, collision defaults, preview, tags, compatibility, and configuration
  limits. The schema must not contain brand-specific robot fields.
- Add drag/drop assignment and missing-reference repair.

Acceptance: moves and reimports preserve references; imports and failures are
visible, cancellable, reproducible, and migration-tested.

## 18E — Prefabs and reusable content

Status: complete. Persistent prefab manifests and scene sources, stable nested
instance identity, variants, overrides, conflicts, apply/revert/unpack,
transactional project-session operations, Asset Browser import/drag creation,
and nested assembly regression coverage are implemented. Nested links and typed
component overrides are verified across plugin replacement. See
[milestone18/18E_PREFABS_AND_REUSABLE_CONTENT.md](milestone18/18E_PREFABS_AND_REUSABLE_CONTENT.md).

- Add prefab creation, instances, nesting, variants, stable references,
  overrides, apply/revert, unpack, conflicts, and scene/object templates.
- Treat a multi-part machine as a reusable assembly: nested part instances keep
  attachment and connection identity through prefab edits and variants.

Acceptance: nested instances preserve overrides through source edits, save/load,
undo/redo, and plugin reload without silent data loss.

## 18F — Docking, layouts, and professional workspace

Status: complete. The responsive Workbench has resizable side and bottom docks,
interactive movable/resizable floating tools, compact modal presentation,
saved/reset and display-safe layouts, separate scene/game viewport hosts,
keyboard panel access, and standard Console, Profiler, Learning, Game,
Connections, Telemetry, Commands, and Extensions tabs. Implementation and
acceptance evidence are in
[milestone18/18F_DOCKING_AND_PROFESSIONAL_WORKSPACE.md](milestone18/18F_DOCKING_AND_PROFESSIONAL_WORKSPACE.md).

- Add dockable/resizable panels, tabs, floating windows, splitters, saved layouts,
  reset, multiple monitors, multiple scene/game viewports, and DPI resilience.
- Supply hierarchy, inspector, assets, console, profiler, learning, and game panels.
- Supply a generic connections view and telemetry view that projects can extend
  with mechanical, power, signal, sensor, and actuator data.
- Preserve project-specific Pezzza drawers inside simulation views.
- Add a consistent menu bar, toolbar, status bar, breadcrumbs, context menus,
  searchable creation menus, icons, tooltips, empty states, validation messages,
  modal dialogs, notifications, and progress feedback.
- Establish editor design tokens and reusable patterns for spacing, typography,
  colors, borders, selection, focus, hover, disabled states, and density. Apply
  them consistently rather than styling each panel independently.
- Support compact laptop, 1080p, 1440p, high-DPI, ultrawide, and multi-monitor
  layouts with enforced minimum sizes, scrolling, overflow behavior, and no
  overlapping or unreachable content.

Acceptance: layouts restore after missing plugins and across supported screens;
no essential panel becomes unreachable. Primary workflows follow the same
selection, editing, drag/drop, context action, keyboard, and feedback conventions
across hierarchy, viewport, inspector, assets, and project-defined panels.

## 18G — Extensible tools, actions, and plugins

Status: **reopened**. Plugin identity, executable extension points, actions,
scheduling, declared shared services, and staged reload are implemented. The
reference-project migration is incomplete: Ant and SailBoat still expose SFML
through project source and several lifecycle-bearing classes bypass PipeFrame's
system/tool contracts. Plugins register every editor/domain extension family,
searchable actions, and dependency-ordered systems through a backend-neutral
API. The host validates, schedules, isolates, and safely reloads them. Basic,
Ant, and SailBoat conform to the shared entry point, folder contract, and
scheduler while retaining their accepted behavior. Implementation and evidence
are in [milestone18/18G_EXTENSIBLE_TOOLS_ACTIONS_AND_PLUGINS.md](milestone18/18G_EXTENSIBLE_TOOLS_ACTIONS_AND_PLUGINS.md).

- Let plugins register panels, drawers, tools, gizmos, menus, commands, overlays,
  importers, and settings.
- Let plugins register part categories, attachment compatibility rules,
  connection types, environment brushes, validation rules, telemetry streams,
  and simulation debug overlays.
- Add action maps, shortcut rebinding/conflicts, contexts, and command search.
- Add safe hot reload, compatibility checks, state restoration, and isolation.
- Add a standard project module contract for Components, Systems, Runtime, and
  Editor code. Projects register systems with stable IDs, lifecycle phases,
  component access, dependencies, ordering, and parallel-safety declarations.
- Supply system contexts for component queries, command buffers, fixed and
  variable time, spatial queries, physics, resources, events, rendering
  submissions, logging, deterministic random streams, profiling, and
  deterministic jobs. Structural changes use command buffers. Project systems
  must not own a second application loop or depend on Workbench internals.
- Validate duplicate IDs, missing/cyclic dependencies, conflicting writers,
  invalid lifecycle access, and editor/runtime boundary violations.
- Migrate BasicSimulation, AntSimulation, and SailBoatSimulation to the standard
  project layout and registration entry point. Their Components, Systems,
  Runtime composition, Editor extensions, Assets, Scenes, Config, and Tests must
  be discoverable through the same conventions as a newly generated project.
- Replace monolithic or project-owned lifecycle orchestration with registered
  PipeFrame systems. In particular, split Ant updater orchestration into
  declared systems and adapt SailBoat movement, sensing/network evaluation,
  race logic, and training orchestration to the shared scheduler and contexts.
  Domain algorithms remain in their projects.
- Require lifecycle-bearing project classes to implement the matching public
  PipeFrame contract. Update orchestration implements `FixedUpdateSystem` or
  `EntityUpdateSystem`; physics passes implement `PhysicsSolver`; editor tools
  implement `EditorTool`; render features submit backend-neutral render
  commands. Plain components, entities, configuration, value types, and domain
  algorithms remain composition-oriented data and do not inherit framework
  classes.
- Prohibit `#include <SFML/...>`, `sf::` types, backend resource access, and raw
  render targets throughout reference-project `Source/`. SFML belongs solely
  in PipeFrame's backend adapter. This gate must pass before 18G can close.
- Require the migrated examples to use PipeFrame component queries, command
  buffers, fixed-step scheduling, spatial services, physics, resources,
  rendering submissions, events, deterministic jobs/randomness, logging, and
  profiling where those services apply. A project-local replacement requires a
  documented domain-specific reason and a regression proving it is necessary.

Acceptance: an external sample plugin adds every extension type without changing
Workbench; reload/failure workflows preserve scene and editor state. The sample
also replaces a monolithic project updater with registered systems using
PipeFrame scheduling and services. Basic, Ant, and SailBoat pass the project
conformance validator and retain their Milestone 17 behavior, visuals, controls,
network/telemetry, simulation time, deterministic results, and performance
gates. Reference-project source contains zero SFML includes, symbols, or backend
adapter access, and its lifecycle-bearing classes visibly implement PipeFrame
contracts.

## 18H — Scene workflow, build pipeline, and completion

Status: in progress. Standard scaffolding, module and object-type generators,
data-only discovery, validation, generated CMake, and the robotics-readiness
fixture are implemented. Build/package/run UI and the final acceptance matrix
remain open. See
[milestone18/18H_SCENE_WORKFLOW_BUILD_AND_COMPLETION.md](milestone18/18H_SCENE_WORKFLOW_BUILD_AND_COMPLETION.md).

- Complete scene management, additive loading, mode transitions, build, package,
  run, platform settings, logs, validation, profiling, capture/export, headless
  automation, replay, accessibility, keyboard navigation, and documentation.
- Run sanitizers, migrations, resource reload, DPI/resize, plugin, performance,
  memory, and full Debug/Release suites.
- Add a backend-neutral editor-readiness fixture that assembles a chassis,
  driven wheels, a distance-sensor placeholder, and a light placeholder; wires
  typed endpoints; builds an obstacle environment; saves, reloads, duplicates,
  undoes, and runs with mock telemetry. This fixture validates editor contracts
  only and does not implement the Milestone 19 robot simulation.
- Run an editor UX acceptance matrix for new/open/save, scene navigation,
  selection, transform, component editing, asset assignment, prefab authoring,
  play/pause/step, undo/redo, layout restoration, error recovery, and keyboard
  operation. Retain screenshots at all supported resolutions for visual review.
- Add New Project templates and Add Component/System/Editor Extension commands
  that generate the standard `Config`, `Assets`, `Scenes`, `Source/Components`,
  `Source/Systems`, `Source/Runtime`, `Source/Editor`, and `Tests` structure,
  CMake target, manifest, registration entry point, starter scene, and tests.
- Add the Create Object Type workflow and generated archetype format. A user can
  compose a reusable type from registered components and defaults without C++.
- Make all code-generation commands update registration/build metadata, compile,
  report navigable errors, hot reload, and expose the generated type immediately.
  Generated code must depend only on public PipeFrame APIs and follow the
  standard project structure automatically.
- Add project validation and CI conformance checks for manifests, source/asset
  roots, schemas and migrations, registrations, system dependency graphs,
  backend-neutral public APIs, assets, and editor/runtime separation. Permit
  custom layouts only when their roots are declared in the manifest.
- Run the same generated-project conformance suite against BasicSimulation,
  AntSimulation, and SailBoatSimulation. Retain the Milestone 17 parity tests,
  interaction recordings, screenshot comparisons, deterministic fixtures, and
  performance thresholds during their structural migration.

Acceptance: a new project can import assets, compose scenes/components, create
prefabs, extend the editor, simulate, debug, build, and run without project-specific
Workbench code. The readiness fixture proves modular part assembly, connections,
environment authoring, and telemetry extension without robot-specific Workbench
code. The UX matrix and reviewed screenshot board show a coherent professional
editor with no overlap, clipped content, hidden input surfaces, inconsistent
states, or unreachable actions. Required engine/editor code exposes no SFML. A
fresh generated project can expose a component, register a system, consume
PipeFrame services, build, reload, serialize, migrate, run, and package without
editing engine or Workbench source. Basic, Ant, and SailBoat use that same public
contract and standard layout without losing any accepted behavior or UI.
Creating a data-only type such as Soldier Ant requires no manual source lookup,
inheritance choice, CMake edit, or registration edit. Creating genuinely new
behavior requires only choosing **Add System** and implementing the generated
domain method. Milestone 19 starts its modular robotics work through these
released workflows. Bugs, missing features, awkward APIs, broken controls, and
poor editor design discovered during that work trigger engine/editor fixes and
reopen the relevant Milestone 18 acceptance gate. Framework-like project code
is a foundation defect rather than accepted Milestone 19 boilerplate.

## Ordering and completion rule

18A begins after relevant 17B-17E APIs stabilize. 18B/18C define authoring before
prefabs. 18D can proceed alongside them using stable references. 18F/18G build on
the component, connection, and asset contracts. Only 18H completes the editor
and unlocks Milestone 19. A class or panel
does not satisfy a phase until its running workflow, persistence, undo, visual,
compatibility, and performance checks pass.

The 18B core remains implemented: it owns the stable scene/component data contracts. The
generated Inspector and macro convenience layer extend those contracts in 18C;
system registration and lifecycle enforcement land in 18G; project scaffolding
and conformance gates land in 18H.

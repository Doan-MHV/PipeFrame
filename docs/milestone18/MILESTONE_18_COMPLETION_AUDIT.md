> Scope update (2026-09-11): [Engine rework — Ant plan](../ENGINE_REWORK_ANT_PLAN.md) governs remaining engine/editor work. Ant is the sole reference project; SailBoat implementation and tests are excluded. Earlier completion statements do not establish completion of this rework.

> Historical Milestone 18 checkpoint, superseded by the engine rework. Do not use
> its aggregate Agent2D/AgentGroup2D/EntityStore or AntAuthoring conventions for new code.
> Use the [current Ant source guide](../../examples/AntSimulation/README.md),
> [component schemas](../rework/COMPONENT_SCHEMA_CONVENTION.md),
> [lifecycle API](../rework/BEHAVIOUR_LIFECYCLE.md) and
> [R7 migration audit](../rework/R7_AUDIT.md).

# Milestone 18 completion audit

Audit date: 2026-09-11

Status: **not complete**

This audit compares the requirements in `docs/MILESTONE_18_PLAN.md` and every
document in `docs/milestone18/` with the current source tree and automated
tests. A passing regression suite is evidence for implemented behavior, but it
does not override an unmet acceptance condition in the milestone documents.

## Verification snapshot

- The Debug build succeeds.
- CTest passes 70/70 registered tests.
- Milestone 18 stores reviewed compact, 720p, 1080p, 1440p, ultrawide, context,
  and floating-workspace views. The broader error-state matrix required by 18H
  remains open.
- `ReferenceProjectConformance` verifies required configuration and asset roots,
  non-placeholder compiled module contracts, `Plugin.cpp`, Workbench
  independence, plugin identity, executable extensions, system graphs and
  declared services, and a headless scheduled Basic pipeline. Backend-removal,
  project generation now has a standard scaffolder, data-only object workflow,
  and robotics-readiness fixture; packaging and the broader 18H matrix remain open.

## Phase verdicts

| Phase | Document status | Audited status | Evidence and remaining work |
|---|---|---|---|
| 18A | Complete | **Complete** | Adaptive and fixed grids, origins, rulers, measurements, direct move/rotate/non-uniform-scale handles, pivots, placement, attachment/connection/collider/sensor previews, registered debug geometry, zoom-stable hit targets, three resolution layouts, and stored screenshots are implemented and covered by `WorkbenchUIAcceptance`. |
| 18B | Complete | **Complete** | The scene/component/hierarchy model, serializer, transactions, connections, migration, and workspace are covered. Basic, Ant, and SailBoat publish domain component descriptors; legacy Ant/SailBoat scene properties migrate into components on open. `WorkbenchUIAcceptance` hot reloads the external component plugin and all three reference runtimes, preserving component registries, instances, references, and authored values. |
| 18C | Complete | **Complete** | The generic Inspector, all typed values, validation, multi-edit, telemetry path, builder, macros, runtime authority, and transactions are covered. Drawer extensions now execute backend-neutral Inspector presentation callbacks through editor hints without Workbench project branches. Ant and SailBoat instantiate and consume registered domain components, and their legacy scenes migrate through `ProjectSession`. Member-binding source generation remains explicitly assigned to 18H. |
| 18D | Complete | **Complete** | Stable IDs, persistence, search, visible operations, repair, generic part metadata, browser assignment, and migration tests pass. Built-in importers validate supported source signatures and package deterministic versioned cache artifacts with distinct preview/thumbnail outputs. New projects include `Assets/Prefabs`; queued and running cooperative cancellation are covered. |
| 18E | Complete | **Complete** | Prefab persistence, nested identity, variants, override detection, apply/revert/unpack, scene transactions, asset recognition, viewport commands, and source propagation are covered. An external plugin replacement receives the same nested prefab links and complete typed component override data held by the scene. |
| 18F | Complete | **Complete** | Responsive side/bottom docks, splitters, interactive movable/resizable floating tools, saved bounds and docking, compact modal tools, eight standard/extension tabs, scene/game viewport hosts with separate cameras, F6/F7/F8 access, missing-plugin and multi-display reconciliation, and compact through ultrawide screenshots are implemented and covered. |
| 18G | Reopened | **Incomplete** | Extension execution, failure attribution, shared services, scheduling, and reload exist. The reference migration does not meet its contract: Ant and SailBoat still expose SFML/backend APIs. Ant now registers separate colony-lifecycle, movement, behavior, cleanup, live-state, and render phases; its entities, groups, store, history, solver, and editor tool use PipeFrame contracts. SailBoat system-object migration and complete backend isolation remain required. |
| 18H | No completion status | **Not implemented** | The full project templates, standard scaffolding, Add Component/System/Runtime/Editor Extension generators, Create Object Type workflow, build/package/run pipeline, navigable compiler errors, project validator, CI conformance, robotics-readiness fixture, accessibility/replay/capture workflow, Debug+Release+sanitizer matrix, and reviewed UX evidence are missing. The milestone plan explicitly says only 18H completes Milestone 18. |

## Required project layout audit

BasicSimulation, AntSimulation, and SailBoatSimulation now contain the required
configuration files and asset categories. Their Components, Systems, Runtime,
and Editor roots contain real implementations compiled by each runtime.
Conformance rejects a missing config or asset root, a placeholder-only module
directory, Workbench dependencies, inert extensions, invalid registration
graphs, and systems without declared service requirements. For Ant it also
checks Agent2D/AgentGroup2D inheritance, generic storage/history, the three
executable simulation phases, scheduler registration, and common plus domain
editor schemas.

## Property exposure audit

PipeFrame has a useful metadata-based foundation:

- `ComponentBuilder` and `FieldDescriptor` are the canonical descriptor API.
- `PF_COMPONENT` and `PF_PROPERTY` provide optional declaration syntax.
- `ProjectRuntime::GetAllSceneComponentTypes` supplies common Transform,
  Identity, Physics Body 2D, Collider 2D, and Renderer 2D descriptors.
- `InspectorPanel` creates standard controls from component/property metadata.

This is not yet a Unity/Unreal-equivalent authoring workflow:

- The macros intentionally describe canonical typed fields without unsafe
  offsets; generating member bindings and registration updates is assigned to
  18H's project authoring workflow.
- There is not yet an annotation parser, generated serializer, code-generation
  tool, or automatic registration update.
- Drawer extensions now provide executable Inspector presentation callbacks;
  richer editor widget factories remain part of the general 18G/18H extension
  work.
- Ant and SailBoat now expose domain component schemas and migrate their legacy
  flat scene properties into those components on project open.
- The project authoring service now creates components, systems, runtime modules,
  editor extensions, and data-only object types. The visible Workbench command
  surfaces and build/reload loop remain open.

## Backend isolation audit

The project contract says project code must not include SFML or another backend
directly. Current source violates this and therefore keeps 18G open:

- BasicSimulation has 9 source files containing SFML includes or `sf::` use.
- AntSimulation's domain migration and scheduler split are complete, but its rendering/dashboard/runtime
  boundary files still contain SFML or PipeFrame backend access.
- SailBoatSimulation's domain migration is complete, but 5
  rendering/dashboard/runtime boundary files still contain SFML or backend access.
The public `PipeFrame/Project` headers are backend-neutral, but other public
engine surfaces used by the editor and examples, including `Application`,
`Scene`, `Input`, `Camera2D`, `CameraController2D`, `RenderContext`, and the UI
widget headers, still expose SFML. This fails the 18H acceptance requirement
that required engine/editor code expose no SFML and prevents the examples from
demonstrating backend-neutral project code.

## Work required before completion

1. Finish the 18G reference migration: remove direct backend dependencies from
   project code, use PipeFrame lifecycle contracts, and replace monolithic
   descriptor callbacks with registered system objects.
2. Implement 18H project templates, authoring/code generators, Create Object
   Type, validation, build/hot-reload/package/run, and error navigation.
3. Implement the modular-assembly readiness fixture and full UX, accessibility,
   screenshot, migration, sanitizer, Debug, and Release acceptance matrices.

Milestone 19 should not be treated as unlocked by the current implementation.
Its dogfood work may begin only after the 18H public workflows exist; defects
found during Milestone 19 must reopen and harden the relevant Milestone 18 gate
as the plan already requires.

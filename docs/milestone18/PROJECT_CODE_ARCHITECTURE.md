# PipeFrame project code architecture

> Historical Milestone 18 checkpoint, superseded by the engine rework. Do not use
> its aggregate Agent2D/AgentGroup2D/EntityStore or AntAuthoring conventions for new code.
> Use the [current Ant source guide](../../examples/AntSimulation/README.md),
> [component schemas](../rework/COMPONENT_SCHEMA_CONVENTION.md),
> [lifecycle API](../rework/BEHAVIOUR_LIFECYCLE.md) and
> [R7 migration audit](../rework/R7_AUDIT.md).

Status: required for Milestone 18G completion

PipeFrame owns the application loop, fixed-step scheduling, input translation,
render backend, resource lifetime, scene transactions, editor selection, undo,
serialization, hot reload, diagnostics, and service discovery. A project owns
only domain data and domain rules.

## Class rules

| Project code | Required shape |
|---|---|
| Entity | Identity owned by `ecs::World`; attach independently typed components |
| Attached script | Derive from `Behaviour`; the scene owns dispatch and teardown |
| Legacy runtime agent | `Agent2D` remains a compatibility component during migration |
| Runtime group | Derive from `AgentGroup2D` for identity, position and membership |
| Authorable settings | Bind members with `ComponentSchema<T>` |
| Configuration, command, event, small value type | Plain data |
| Fixed-step orchestration | Derive from `pipeframe::FixedUpdateSystem<Result>` |
| Entity update rule | Derive from `pipeframe::EntityUpdateSystem<Entity, Environment, Command>` |
| Physics/contact pass | Derive from `pipeframe::PhysicsSolver<World, Result>` |
| Interactive editor tool | Derive from `pipeframe::EditorTool` and register it as an extension |
| Render feature | Produce PipeFrame geometry/effect/text commands; never accept an SFML target |
| Complete project runtime | Derive from `pipeframe::ProjectRuntime`; keep it as a thin composition root |

Inheritance identifies lifecycle and host integration. Domain objects use
composition. A Soldier Ant, for example, is an authored type composed from Ant,
ColonyMember, Role, sensing, and combat components. It is not a new framework
subclass.

## Backend boundary

Project `Source/` may include public `PipeFrame/...` headers. It may not include
`SFML/...`, name `sf::` types, include `PipeFrame/Backend/...`, request an SFML
render target, or translate PipeFrame input back into SFML events. The SFML
implementation is confined to `engine/src/Backend/SFML` and private platform
code. PipeFrame must expose a backend-neutral capability before a project uses
that capability.

## Reference mapping

- `AntMovementSystem`, `AntBehaviorSystem`, `AntCleanupSystem`,
  `ColonyLifecycleSystem`, and `AntAvoidanceSystem`: executable fixed-update
  systems. The plugin registers the principal phases with PipeFrame's scheduler.
- `AntSimulationPipeline`: compatibility composition used by direct/headless
  calls; it delegates to the same movement, behavior, and cleanup systems.
- `Ant`: `pipeframe::Agent2D`; `Colony`: `pipeframe::AgentGroup2D`.
- `AntStore`: `pipeframe::EntityStore<Ant, AntId>`; `ColonyHistory`:
  `pipeframe::SampleHistory<ColonyHistorySample>`.
- `AntAuthoring.cpp`: canonical `PF_COMPONENT` schemas that feed the Add Object
  menu and Inspector, including validation ranges and units.
- `ContactSolver`: physics solver. `AntEditorTool`: registered editor tool.
- `BoatMovementSystem`: entity-update system driven by `BoatUpdateCommand`.
- Configurations, commands, events, and small value types remain plain data.
- Ant and SailBoat renderers must migrate to PipeFrame command submission before
  the backend-isolation gate closes.

The conformance test will enforce both the backend boundary and the required
base contracts. A project should never need its own window loop, input adapter,
resource manager, raw render target, fixed-step scheduler, undo implementation,
or editor lifecycle.

Implementation details and remaining gaps: [Ant authoring proof](ANT_AUTHORING_PROOF.md).

# Ant world composition — September 12, 2026

This is the current Ant architecture. It supersedes the earlier top-level Physics,
Rendering, Systems and environment-only World layout. SailBoat is outside this change.

```text
Source/
  Entities/                       EntityArchetype recipes
  Components/                     data and component-local Schema()
  Behaviours/                     attached Behaviour scripts
  Configuration/
  World/
    AntWorld.h/.cpp                root: fixed-step phase order and world state
    Physics/
      AntPhysicsWorld.h           PhysicsWorld: owns and schedules physics work
      AntBodySystem.h/.cpp        body storage/synchronization/integration
      AntMovementSystem.h/.cpp
      AntContactSystem.h/.cpp
      AntAvoidanceSystem.h/.cpp
      ContactSolver.h/.cpp
      CollisionGrid.h/.cpp
      AntLegPose.h/.cpp
    Rendering/
      AntRenderingWorld.h         RenderingWorld: owns and registers render layers
      AntRenderer.h/.cpp
      EnvironmentRenderer.h/.cpp
      ShadowRenderer.h/.cpp
      AntDebugRenderer.h/.cpp
      MarkerRenderer.h/.cpp
      WallRenderer.h/.cpp
      AntGeometry.h/.cpp
    Runtime/
      AntRuntimeWorld.h           RuntimeWorld: scene, environment, colony/ant systems
      AntQuery.h/.cpp
      AntView.h/.cpp
      ColonyView.h/.cpp
      ColonyHistory.h/.cpp
      AntStepResult.h
      Environment/                map, cells, food, marker and world queries
      Systems/                    colony lifecycle, foraging, cleanup, worker rules
  Runtime/                        plugin/editor hosting and type registration
  Editor/                         declarative dashboards, inspectors and editor tools
  Plugin.cpp
```

## Engine responsibilities

The public contracts are in `engine/include/PipeFrame/World/World.h`:

- `World` validates fixed-step phase order and owns an optional rendering module.
- `RuntimeWorld` owns the ECS/behaviour scene. `Update(dt)` runs registered environment
  systems, scene behaviour lifecycle, then registered runtime systems.
- `PhysicsWorld` executes registered physics steps in order with timestep and reentry
  checks. Ant registers its movement pipeline, which uses the body system and solver.
- `RenderingWorld` executes registered `RenderLayer`s in insertion order within a stage.
  The layer base handles visibility; concrete layers implement `Draw(Canvas, RenderState)`.

These are executable contracts, not ID-only parents. All module owners are noncopyable:
registered systems/layers refer to their owner's members. Registration happens during
construction, not once per ant or once per frame. Entity/component data continues to use
existing ECS storage and bulk systems rather than a virtual object per particle.

AntWorld owns its AntRuntimeWorld and AntPhysicsWorld in its private state. Its inherited
rendering owner holds AntRenderingWorld when hosted graphically. The project runtime also
retains that rendering module across simulation rebuilds so textures and shaders are not
reloaded on Reset. Headless worlds need no graphics module or graphics initialization.
`GetRuntimeWorld()` and `GetPhysicsWorld()` expose the modules. `GetPhysicsBodies()` is the
explicit query for the body system, replacing the misleading old body-store getter name.

AntRenderingWorld registers environment in stage 0 and shadows, ants, debug and editor
geometry in stage 1. The runtime draws authored beacons/selection between these stages,
preserving the existing visual order. EnvironmentRenderer contains the smaller marker and
wall layers. Physics and runtime logic remains in small files underneath their owning world.

## Adding logic

For a new runtime rule, implement `FixedUpdateSystem<void>::Update(dt)`, own it as a member
of the project's RuntimeWorld, and register it with `AddSystem`. Environment work that must
precede behaviours uses `AddPreSceneSystem`. Physics steps use `AddStep`; result-returning
systems can retain a typed result in their module. A new render layer implements
`RenderLayer::Draw`, is owned by the project's RenderingWorld and registered with `AddLayer`.
The root world keeps the phase order; small systems do not drive their own application loop.

Entity recipes still derive EntityArchetype and compose registered components. Component
schemas remain inside component headers, including editable/read-only/hidden policy and
validation. Register the entity recipe for the editor's creation picker; a folder name is
not runtime reflection or automatic discovery.

Not every struct needs inheritance. Components are data; CollisionGrid delegates to the
engine UniformSpatialIndex; geometry builders and map/raycast utilities are algorithms.
Those are implementation details of a world module, not competing application lifecycles.
Adding an empty parent solely to label a data structure would not supply engine behavior.

## Backend boundary

All Ant Source headers and implementations now contain zero direct SFML references or
PipeFrame backend includes. A dependency-lint failure enforces this rule with no Ant allowlist.
Render layers use neutral vertices, Canvas, texture/shader handles and RenderState. Engine
adapters resolve handles and submit native geometry. Canvas uses a nonowning function-pointer
view and the adapter reuses its vertex conversion buffer. Selection outlines also use a
shared neutral Canvas primitive.

The existing engine RenderContext, Camera2D and native UI host still internally use SFML;
their headers are not all opaque/Pimpl APIs yet. This change removes backend code from Ant,
not the engine's SFML implementation or every transitive native-header dependency. Ant's
World/render-layer headers can be compiled without SFML include directories. The application
host remains linked against PipeFrame's current native backend.

## Scope and acceptance

This closes the Ant world organization and direct project-backend migration requested here.
It does not close R6's integrated source build/reload workflow, the broader editor public-header
isolation, or the measured editor/1–3–5-colony 60 FPS gate. Existing project scaffolding still
uses the earlier general Source/Systems layout; world-composed starter generation is a follow-up
for the authoring workflow, not claimed implemented by moving Ant's source files.

Verification results are recorded in WORLD_COMPOSITION_ACCEPTANCE.txt.

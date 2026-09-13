# Build a small environment from an empty PipeFrame project

This lesson uses the existing editor and public library. Its two source files are compiled
by [BlankProjectAcceptanceTests](../../../apps/SimulationWorkbench/tests/BlankProjectAcceptanceTests.cpp).
The Probe moves toward a painted wall, reports its nearest ray hit, and stops. It is a
query-driven movement example, not a path planner or a physical ultrasonic/LiDAR model.

## Start and author the world

1. Launch SimulationWorkbench, choose **New Project**, and select a new empty folder.
   Keep the generated `project.pipeframe`, `Config`, `Assets`, `Scenes`, `Source`, and
   `Tests` structure. Asset subfolders are also created on demand. Choose **Build & Reload**
   before expecting the project's entity types in the hierarchy's **+** picker.
2. Choose **+ → Playground**. Set its Transform position to `(0,0)` and size to
   `(320,240)`. This is the bounded authoring surface; it is not itself a collision wall.
3. Select the Playground, open **Assets → Maps → New Map**. The editor creates and assigns
   an empty tilemap. For this lesson use 32×24 cells of size 10. Choose the Wall tile and
   **Line**; drag vertical walls through columns 12 and 22, leaving passages at opposite
   ends. Add a short horizontal branch. These coordinates are lesson placement choices,
   not values in the Probe's code. Brush, rectangle, ruler, layers, selection, resize,
   and resolution tools are described in [environment editing](../../rework/R6_ENVIRONMENT_EDITING.md).
4. **Undo Map** and **Redo Map** operate on an entire stroke. **Save Map** saves the asset;
   **Save** saves the scene. Stop Paint before moving objects. Toolbar undo routes to the
   active asset editor while it is open. A shared map edit affects every referencing
   instance; choose **Make Unique** when only one instance should change.
5. In **Materials**, create a material, edit its tint, save it, then assign its asset ID
   to the Playground's Ground Material property. Textures, shaders, materials, tilesets,
   and maps have separate browser categories. Project assets are indexed automatically;
   a project does not need a handwritten file list. See [visual asset workflow](../../rework/R6_VISUAL_ASSETS.md).
6. Create three **Environment Obstacle** objects. Rename them Spawn, Goal, and Obstacle.
   Give each a visible 10×10 box. Place Spawn around `(30,105)`, Goal around `(280,105)`,
   and Obstacle around `(60,120)`. Disable **Collision Enabled** for Spawn and Goal,
   and give them distinct colors. These two are visual role markers; naming an object
   Goal does not automatically implement navigation or a win condition.

## Generate an entity, settings, and logic

Use the hierarchy's source-generation action to generate **Entity Probe**,
**Component ProbeSettings**, and **Behaviour ProbeBehaviour**. Generated source lives in
`Source/Entities`, `Source/Components`, and `Source/Behaviours`. Replace the generated
settings and behaviour with [ProbeSettings.h](ProbeSettings.h) and
[ProbeBehaviour.h](ProbeBehaviour.h). Then **Build & Reload**.

Generation records module entries and refreshes `Source/Runtime/GeneratedRegistration.h`.
Do not duplicate that registration in a handwritten Ant-style registration file. Merely
creating an arbitrary `.h` file outside the generator does not register a type. Creating
an instance with **+** writes scene data; it does not create a new C++ class for each ant.
[ProjectScaffolder](../../../apps/SimulationWorkbench/Editor/ProjectScaffolder.cpp) implements this contract.

Create a Probe instance and place it at Spawn. In its Inspector attach:

- **Probe Settings**: speed 20, range 80, stop distance 6, direction `(1,0)`.
- **Kinematic Body**: radius 2. This shared component enables swept circle movement.
- **ProbeBehaviour**: the project-specific decision about when to move or stop.
- **Environment Collider**: size `(6,6)`, Visible on, **Collision Enabled off**.
  Here the shared shape is a simple visible marker. Keeping its collision disabled avoids
  the Probe's own marker being hit by its ray. Its Kinematic Body still collides with walls.

A Transform is supplied by the entity. The current generic runtime renders environment
shapes; attaching a legacy Renderer2D descriptor alone is not a replacement for a working
sprite renderer. The lesson deliberately uses an implemented rendering path.

## Understand the code

[ProbeSettings](ProbeSettings.h) owns its complete schema. `.Editable(...)` creates an
editable, serialized field; `.ReadOnly(...)` exposes telemetry without an Inspector setter.
Fields omitted from the schema are hidden and are not serialized by this schema. The
component's `.Validate(...)` checks a complete proposed value, including the relationship
between stop distance and ray range. Try setting stop distance above range: the edit must
be rejected. Pausing or resetting does not turn read-only telemetry into editable settings.
These policies govern authoring, not arbitrary C++ writes by simulation code.
[ComponentSchema](../../../engine/include/PipeFrame/Project/ComponentSchema.h) is the implementation.

[ProbeBehaviour](ProbeBehaviour.h) derives from the engine's `Behaviour`, obtains the
attached components with `GetComponent<T>()`, and implements `FixedUpdate`. It asks
`GetService<EnvironmentQueries>()` for the current scene's environment service, casts a
ray, publishes telemetry, and sets the body's velocity. The generated
[SceneProjectRuntime](../../../engine/include/PipeFrame/Project/SceneProjectRuntime.h)
automatically installs that service and advances kinematic bodies. The author does not
create a physics loop or reach into editor internals.

[EnvironmentQueries](../../../engine/include/PipeFrame/Environment/EnvironmentQueries.h)
provides raycast, circle overlap, clearance, and swept-circle queries against authored
maps and environment shapes. Distances are world units; masks filter collision layers.
This is a geometric API. Sensor noise, scan timing, beam models, and navigation policy
remain project logic. `GetService` returns null without an attached scene/provider;
the sample handles missing dependencies safely. Services are scene-local borrowed
references, not global singletons; a provider must outlive its behaviours.
[BehaviourScene](../../../engine/include/PipeFrame/ECS/Scene.h) owns the registry and lifecycle.

## Verify and retain your work

1. Save, then **Play**. The visible Probe advances and stops before the first wall.
   Inspect the live Transform and Probe Settings: Nearest Hit becomes positive and
   Blocked becomes on. The authored selection gizmo can remain at the authored position;
   inspect the visible marker and live values when checking simulation movement.
2. **Pause** freezes movement. **Reset** restores the authored position and telemetry
   defaults. Runtime telemetry is not a new authored scene configuration.
3. With Probe selected, right-click the viewport and choose **CREATE PREFAB**. In
   **Assets → Prefabs**, select it and choose **INSTANTIATE PREFAB**. Place another near `(30,120)` to exercise the separate box obstacle. Save the scene and map.
4. Reopen the project. Check map assignment, painted walls, components, and prefab instance.
   Build & Reload again and repeat Play/Pause/Reset. Compiler or ABI errors must leave the
   last usable runtime available; inspect the Console and fix the source, then rebuild.

The integrated automated test checks this entire sequence using the same ProjectSession,
asset editors, gesture controller, build commands, and plugin loader used by the editor.
To retain a reproducible full project, run `BlankProjectAcceptanceTests /tmp/MyNewLesson`
from the built `apps/SimulationWorkbench` directory; the destination must not already exist. With no
argument it creates a temporary project and cleans it up on success. The native walkthrough
and automated coverage are separately recorded in [the acceptance report](../../rework/R6_BLANK_PROJECT_ACCEPTANCE.md).

## Extend the engine conventions without starting from scratch

**Systems and worlds.** Entity is identity/composition, Component is data, Behaviour is
per-object lifecycle logic, and systems process component queries in batches. Larger
projects compose RuntimeWorld, PhysicsWorld, and RenderingWorld under World.
[World.h](../../../engine/include/PipeFrame/World/World.h) supplies ordered runtime systems,
physics steps and rendering layers. Owners retain those modules; registration is explicit
and borrowed lifetimes must remain valid. The engine does not discover execution order
by scanning folders. Study [AntWorld.cpp](../../../examples/AntSimulation/Source/World/AntWorld.cpp)
and its World subfolders for a real simulation. Ant's specialized solver remains explicit;
the generic scene service does not silently replace its physics implementation.

**Custom brushes.** Generate a Brush module, implement the public
[BrushTool](../../../engine/include/PipeFrame/Editor/BrushTool.h) contract, expose settings
through its schema, and rebuild. Reuse the engine gesture transaction, clipping, preview,
and undo machinery. Ant's wall/food brushes demonstrate project-specific semantics over
shared tools. Follow [custom brush API and example](../../rework/R6_CUSTOM_BRUSHES.md).

**Flutter-style UI.** Describe a tree with [View](../../../engine/include/PipeFrame/UI/View.h):
Row/Column, Card, Text, controls and Scroll. Use stable keys and
[StatefulView](../../../engine/include/PipeFrame/UI/StatefulView.h) for changed state, and
[ViewPanel](../../../engine/include/PipeFrame/UI/ViewPanel.h) to mount a project panel.
Invalidate when state changes; do not rebuild unrelated trees for every simulation tick.
The Workbench's [InspectorPanel](../../../apps/SimulationWorkbench/Editor/InspectorPanel.cpp)
and Ant's [AntDashboard](../../../examples/AntSimulation/Source/Editor/AntDashboard.cpp)
use the same public composition model. Native window/font/event details belong in the
backend host, not project UI. This is a C++ declarative API inspired by Flutter, not Dart
or a promise of every Flutter widget.

**Assets and PNG migration.** A Texture supplies pixels, a Material supplies appearance,
a Tileset maps tiles to visuals, a Tilemap stores cell/layer data, and scene components
place/bind those assets. Collision is authored tile/shape data, not inferred automatically
from any displayed texture. Ant retains its explicit legacy PNG importer (red wall channel
takes precedence over green food); new environments use editor-authored tilemaps.
See [Ant migration and import limits](../../rework/R6_ANT_ENVIRONMENT_AUTHORING.md) and
[shared collision semantics](../../rework/R6_ENVIRONMENT_COLLISION.md).

The local build workflow currently supports macOS/Linux process execution. Rebuild plugins
for host ABI 13; old plugin binaries are rejected. This lesson does not claim a portable
installed SDK, Windows process support, full 3D terrain, or the Milestone 19 robot project.

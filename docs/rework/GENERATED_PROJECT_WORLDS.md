# Generated project worlds

New Project now generates Ant's world-module organization without copying Ant rules or assets:

```text
Source/
  Runtime/<ProjectName>Runtime.h   # editor lifecycle and registration adapter
  World/
    ProjectWorld.h                # owns the three world modules
    Runtime/ProjectRuntimeWorld.h # behaviours and ordered simulation systems
    Physics/ProjectPhysicsWorld.h # default or custom physics and ordered steps
    Rendering/ProjectRenderingWorld.h # default or custom rendering and layers
  Components/
  Entities/
  Behaviours/
  Systems/
  Editor/
```

Each world derives from its PipeFrame world base. Physics and rendering helpers belong beside their owning world, as in Ant. Components contain data and Schema(); entities compose components. The world classes do not replace entity data.

The runtime world shares the actual editor scene. It runs pre-scene systems, scene behaviours, then post-scene systems once per fixed tick. The scene adapter captures previous transforms before that phase, performs lifecycle bookkeeping, then dispatches physics. The default physics path preserves kinematic collision handling and contact callbacks. Register project steps with AddStep; these run after default physics. Replace defaultPhysics(delta) in ProjectPhysicsWorld::Simulate when supplying a custom solver. A replacement solver is responsible for its collision/contact behavior.

Rendering calls the default scene renderer first and registered RenderLayers afterward. Replace drawScene() in ProjectRenderingWorld::DrawScene for a specialized renderer. Own each custom RenderLayer in the rendering world and register it with AddLayer. Use neutral Canvas, Vertex2D and RenderState, and obtain texture resources through the runtime's GetVisualAssets().ResolveTexture(typedAssetReference). No SFML is required in project code. The default renderer still supports Inspector-authored sprites and terrain; a custom renderer must implement any visualization and visibility notifications it replaces.

New files are not automatically scheduled merely because they are in these directories. Their owner registers them explicitly, making order and dependencies visible. The engine continues to own save/load, component schemas, object creation, reset, and build/reload integration.

This changes newly generated projects. Existing projects and Ant are not rewritten. Ant keeps its specialized solver and batched renderer. Built-in physics/sprite components remain optional defaults, not a requirement for custom simulation models.

## Physics and mesh visualization

The main editor toolbar has independent **PHYSICS OFF/ON** and **MESH OFF/ON** buttons. These are session view settings, not scene edits; they do not enable/disable collisions, run a tick, or change saved data. Geometry is collected only while a channel is enabled and drawn after world rendering, before editor handles and UI.

`ProjectRuntime::CollectWorldDebug(WorldDebugDraw&, WorldDebugOptions)` is the public extension hook. The generated adapter forwards to the default scene provider and to `ProjectPhysicsWorld::CollectDebug` / `ProjectRenderingWorld::CollectDebug`. Add custom geometry in those world files, or delegate to their smaller systems. Remove the default provider call when replacing the default scene simulation/rendering entirely.

```cpp
// Inside the physics world: use actual solved positions and radii.
void CollectDebug(pipeframe::WorldDebugDraw& draw) const {
    for (const auto& body : bodies.GetBodies())
        draw.Circle(body.position, body.radius);
}
// Inside a rendering world: vertices must be in world space.
// draw.Mesh(renderer.GetTriangleVertices());
// draw.Line(rayStart, rayHit); // rays/contact normals are project-provided
```

The neutral API batches lines, circles, polygons and triangle edges through Canvas with no texture or shader. Custom projects are not required to use a particular collider component or solver. Mesh input is triangle-list geometry, not triangle strips or indices. Convert indexed geometry before submitting it. Point LOD has no triangle edges; zoom into Ant's quad rendering to inspect its mesh.

Default scene visualization includes enabled primitive colliders (even when their normal Visible field is false), fallback kinematic circles, solid tile cells, and visible sprite quad edges/diagonals. Ant supplies real body radii, exposed wall edges and its current body/leg/food mesh. Its existing local PHYSICS display remains independently available. Contacts/rays are supported as line submissions but are not automatically inferred or recorded for every solver.

`Source/Runtime/` remains the integration boundary for the editor: registration, loading, authored-scene synchronization, reset, and dispatch. `Source/World/Runtime/` owns simulation rules. Keep new gameplay/simulation systems inside World, not inside the adapter. Existing integrations depend on the Runtime folder convention, so this update documents that distinction rather than renaming existing projects.

Plugin ABI is now 14. Rebuild project libraries before loading them with the updated editor.

## Completion evidence

The generated-project acceptance compiles custom physics/rendering debug providers
and checks off, physics-only, mesh-only and combined output through the editor host.
Native UI acceptance verifies toggle interactions. Ant's running physics and mesh
views have been captured and inspected; see
[evidence](evidence/world-debug-controls/README.md).

## Ant outer runtime cleanup

Ant's outer adapter now delegates scene construction to `AntWorld::FromScene`
(`World/Runtime/AntWorldAuthoring.cpp`), renderer loading/options/geometry/drawing
to `AntRenderingWorld`, and food queries to `AntEnvironment`. Construction returns
a candidate world and entity handles together, retaining custom entity factories.
The adapter commits them after successful validation and construction.

`Editor/AntUIHost.cpp` now contains dashboard hosting next to the dashboard views.
`Runtime/` retains the host contract, registration metadata and scene-authoring
integration. It is intentionally not renamed: generated registration and existing
project integrations use that path. The adapter still implements the broad
ProjectRuntime interface; this is a responsibility refactor, not a claim that
Ant's adapter is as small as an empty generated project.

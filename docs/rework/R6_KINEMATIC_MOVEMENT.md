# R6 generated-project movement against authored walls

The generated SceneProjectRuntime now registers KinematicBody2DComponent and executes
it after behaviour FixedUpdate. Attach **Kinematic Body** through the existing component
picker, then edit Velocity, World Radius, Tile Collision Mask and Enabled. Schema and
validation live in the component header. Rebuild older generated runtimes to acquire
the component; no project-specific component registration or collision loop is needed.

The engine reads each entity's actual Transform and Kinematic Body components. Velocity
is in world units/second; radius is explicitly world-space and independent of the body's
own Transform scale. Tile collider transforms and Playground clipping still apply.
Directly moving a Transform in project behaviour is not retroactively collision-checked;
set the body's velocity when collision-controlled movement is desired.

## Continuous collision and response

SweepCircleTilemap uses continuous circle/rectangle face and rounded-corner intersections.
It supports the same rotation, signed/nonuniform scale, layer mask and clipped tile bounds
as scene queries. A conservative enclosing-circle query collects candidate tiles; there is
no second obstacle map. SweepEnvironment dispatches to assigned scene tilemaps and excludes
the moving object's own tilemap. No simulation substep-size reduction is used to avoid
crossing thin walls during a long movement.

The runtime advances to first contact, removes inward velocity, and sweeps remaining
movement along the tangent. Up to four contacts are processed per tick; unresolved
remaining movement is discarded. A starting penetration stops movement without attempting
automatic ejection. These policies are bounded kinematic motion, not rigid-body dynamics.

Pause prevents movement. Reset restores authored components. Runtime velocity/Transform
changes do not rewrite authored source values. Reimported environment data is resolved
through the same asset module as rendering and queries.

## Verified complete path

ProjectBuildAcceptance creates/builds a project, creates an entity through its registry,
attaches the engine body through ProjectSession, edits velocity/radius through schema
validation, and runs it against an assigned authored tilemap. A 100-unit displacement
stops at the wall instead of passing through. Save/reopen repeats the same collision
successfully. No hand-written collision code or extra registration is added to the fixture.

SceneProjectRuntimeAcceptance verifies high-speed contact, sliding, tangent velocity,
mask exclusion, pause and reset. TilemapRegression verifies rounded corners, outward
motion from a touching face, avoiding inflated-square false positives, and rotated/scaled
sweeps. Dependency lint checks backend-free public APIs.

**4/4 checks passed in 10.40 seconds.** Workbench target build passed.
Evidence: [tests](evidence/r6-kinematic-movement/tests.txt).

## Remaining limits and R6 work

This is enabled in generated SceneProjectRuntime projects, not automatically in Ant's
specialized physics runtime. No mass/forces, body-body contacts, moving-platform response,
penetration recovery, independent box/segment colliders or trigger callbacks are added.
Sweeps across large worlds still need candidate-index performance acceptance. A graphical
click-through capture of component attachment remains separate from the editor API test.
Materials/tilesets, custom-tool generation, Ant environment migration, remaining native
isolation, performance gates and full R6 acceptance remain open. R6 is not marked complete.

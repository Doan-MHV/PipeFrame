# PipeFrame 2D physics and environment services

Milestone 17D adds headless simulation primitives to `PipeFrame::Foundation`.
The implementation is header-only and has no SFML dependency.

`PipeFrame/Physics/Physics2D.h` defines particles, circles, segments, bodies,
collision layers and masks, contacts, deterministic integration, bounds,
uniform-index broadphase, mass-weighted circle separation, segment intersection,
and backend-neutral debug lines and circles. `PipeFrame/Physics/PhysicsWorld2D.h`
adds dense world ownership with stable numeric body IDs, immediate removal,
lookup, stepping and contact solving.

`PipeFrame/Environment/ScalarField2D.h` defines scalar and vector fields over a
bounded grid. Fields support clamped deposition, nearest-cell sampling, central
gradients, decay, diffusion and double-buffered updates. Application policy such
as pheromone ownership, food accounting, or wind meaning stays outside the field.
Obstacle traversal uses `PipeFrame/Spatial/GridRaycast.h`; callers supply only a
blocked-cell predicate.

Ant now derives `AntPhysicsBody` from the common body and adds only ant and colony
identity plus facing direction. Its world handles ant lifecycle synchronization,
while PipeFrame performs integration and bounds. Its contact adapter applies wall
policy, then calls the common broadphase and circle solver. Its debug renderer
visits common debug-circle data, and its legacy pheromone adapter composes two
common scalar fields.

`PhysicsEnvironmentRegression` is a small non-Ant laboratory. It creates, moves,
collides, looks up, and removes bodies; verifies collision masks and deterministic
replay; traverses debug geometry; integrates particle lifetime; deposits,
diffuses, decays and samples fields; and checks segment and obstacle raycasts.

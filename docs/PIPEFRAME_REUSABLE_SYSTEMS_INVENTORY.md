# PipeFrame reusable systems inventory

This is the durable capability ledger for PipeFrame. Milestone 17 owns reusable
runtime extraction and Pezzza parity. Milestone 18 owns the general-purpose
editor work. A class existing is not completion: a capability must be reachable,
tested in a real workflow, serializable where applicable, and backend-neutral at
the project boundary.

## Ownership rule

PipeFrame owns reusable mechanisms; projects own simulation policy. Physics
integration belongs in PipeFrame while ant energy belongs in Ant. Render passes
belong in PipeFrame while SailBoat chooses water behavior. Spatial indexing
belongs in PipeFrame while pheromone interpretation belongs in Ant. Learning
infrastructure belongs in PipeFrame while sailing inputs and scoring remain in
SailBoat.

## Runtime and data

- Neutral math, colors, transforms, angles, time, input, and presentation types.
- Stable IDs, generational handles, dense/sparse stores, and components.
- Signals, observers, command queues, actions, logs, diagnostics, and errors.
- Deterministic RNG streams, fixed steps, replay, time scaling, and stepping.
- Jobs, thread pools, cancellation, progress, profiling, and performance counters.
- Serialization, schema migration, settings, project preferences, and undoable
  transactions.
- Plugin discovery, ABI validation, failure isolation, safe unload, and hot reload.

## Spatial and environment systems

- `Grid2D<T>`, bounded cells, world/cell conversion, ranges, and grid traversal.
- Uniform spatial indexes with insert, move, remove, rebuild, radius, rectangle,
  nearest-object, and line-of-sight queries.
- Grid raycasts, shape casts, collision broadphase, dirty cells, parallel rebuild,
  and spatial debug visualization.
- Scalar/vector fields with deposition, sampling, interpolation, diffusion,
  decay, flow, and navigation cost.

## Reusable 2D physics

- Particles and bodies with position, velocity, acceleration, mass, and damping.
- Circle, rectangle, capsule, and segment shapes; static, dynamic, kinematic, and
  trigger bodies.
- Collision layers/masks, broadphase, narrow phase, contacts, penetration
  correction, resolution, boundaries, constraints, raycasts, and shape casts.
- Deterministic integration, substeps, immediate destroyed-body removal, debug
  geometry, and physics metrics.

## Rendering, textures, effects, and audio

- Neutral render commands, vertices, sprites, shapes, paths, trajectories, text,
  meshes, materials, layers, cameras, viewports, sorting, batching, instancing,
  culling, offscreen surfaces, target pooling, and render statistics.
- Typed texture/font/shader/sound/model/material handles, caching, lifetime,
  asynchronous loading, hot reload, dependency tracking, project-relative IDs,
  missing fallbacks, loss recovery, atlases, sprite sheets, nine-slice assets,
  thumbnails, previews, and duplicate detection.
- Shader uniforms, composed effect passes, blur, shadows, color grading,
  ping-pong textures, GPU simulation, water, depth, shoreline, particles, trails,
  wakes, smoke, splatter, previews, and capability fallbacks.
- Audio clips/instances, master/effect/music/ambient/UI buses, spatial audio,
  volume, pitch, loops, concurrency, pause/resume, device fallback, and preview.

## Input and cameras

- Keyboard, pointer, wheel, touch, and controller events.
- Action maps, contexts, shortcut rebinding/conflicts, focus, pointer capture,
  drag thresholds, multi-selection modifiers, recording, and replay.
- Editor/simulation cameras, pan, zoom, framing, follow, bounds, bookmarks,
  smoothing, multiple viewports, and world/screen conversion.

## Learning and experiments

- Genome/network representation, DAG validation, activation, mutation,
  selection, elitism, generations, and deterministic RNG.
- Policy/environment/evaluator/trainer interfaces and parallel evaluation.
- Iteration/exploration clocks, progress, statistics, history, checkpoints,
  deterministic resume, best-model tracking, and experiment metadata.
- Live activations, biases, signed weights, disabled edges, topology, and
  generalization/evaluation runs.
- Optional PPO/Torch adapter outside the engine core.

## Editor viewport

- Adaptive major/minor grid, origin axes, rulers, and measurements.
- Grid, position, angle, and scale snapping.
- Hover, box, single, additive, and subtractive selection.
- Move, rotate, and scale gizmos with local/world modes and pivot controls.
- Drag/drop creation, placement previews, context actions, frame selection/scene,
  camera bookmarks, editor/game cameras, and zoom-independent gizmo sizing.
- Registered debug overlays for physics, spatial cells, raycasts, fields,
  learning state, and performance.

## Hierarchy, scenes, and components

- Parent-child objects, inherited transforms, stable references, components,
  runtime/editor-only components, layers, tags, locking, and visibility.
- Rename, reparent, reorder, duplicate, delete, group, search, and filtering.
- Component metadata/reflection, add/remove/reorder, object references,
  validation, templates, scene duplication, multiple scenes, and additive load.
- A versioned, backend-neutral registration builder describes component fields,
  defaults, constraints, units, serialization IDs, and editor hints. PipeFrame
  registers shared Transform, PhysicsBody, collider, renderer, and identity
  components; projects register domain components without editing Workbench.

## Inspector and transactions

- Boolean, integer, number, text, vector, color, enum, range, asset,
  object-reference, and component fields generated from metadata.
- Units, validation, errors, reset/defaults, foldouts, and custom drawers.
- Multi-selection, mixed values, copy/paste, batch edits, and live inspection.
- Undo/redo for every mutation, drag coalescing, cancellation, runtime-to-scene
  edits, dirty state, and save points.
- Transform/physics synchronization with explicit authority: authored transforms
  initialize bodies, runtime physics publishes read-only poses, and permitted
  paused edits use transactional teleport/body-update commands that keep the
  scene document, physics world, dirty state, and history consistent.

## Assets, prefabs, and projects

- Asset database/browser with folders, search, thumbnails, previews, import
  status/settings, create, rename, move, duplicate, delete, reveal, and reimport.
- Importers for textures, audio, shaders, materials, models, and scenes; cached
  results, importer versions, dependencies, drag/drop, and reference repair.
- Prefab creation, instances, nesting, variants, overrides, apply/revert, unpack,
  conflicts, and scene/object templates.
- Project templates, recent projects, packages, and dependency status.

## Project code architecture

- A generated standard layout separates Config, Assets, Scenes, Components,
  Systems, Runtime, Editor extensions, and Tests; custom roots must be declared
  in the project manifest.
- A single plugin registration entry point publishes components, archetypes,
  systems, editor extensions, importers, migrations, and project settings.
- Typed component descriptors remain the canonical property-exposure API.
  Optional `PF_COMPONENT` and `PF_PROPERTY` macros generate those descriptors
  without raw field offsets or compiler-specific reflection.
- Registered systems declare stable IDs, lifecycle phases, component reads and
  writes, dependencies, ordering constraints, editor/runtime scope, and whether
  parallel execution is safe.
- System contexts provide component queries, command buffers, fixed and variable
  time, spatial queries, physics, resources, events, render submission,
  deterministic jobs, logging, and profiling through public PipeFrame APIs.
- Project validation reports duplicate registrations, missing and cyclic system
  dependencies, conflicting writers, invalid phase access, missing migrations,
  undeclared roots, backend leakage, and editor/runtime boundary violations.
- Workbench generators create projects, components, systems, and editor
  extensions with build registration and starter tests so official examples do
  not invent incompatible structures.
- PipeFrame provides component storage/queries, command buffers, registered
  system lifecycle and scheduling, spatial/physics/resource/render/event/job/
  logging/profiling contexts, serialization and migration, plugin loading, and
  the editor metadata bridge. Projects retain domain algorithms and presentation.
- BasicSimulation, AntSimulation, and SailBoatSimulation must use the generated
  layout and public project contract. Ant updater orchestration and SailBoat
  runtime/training orchestration migrate to registered systems without losing
  Pezzza parity, network/telemetry, simulation time, determinism, or performance.

## Workspaces and extensibility

- Dockable/resizable panels, tab groups, floating windows, splitters, named/saved
  layouts, reset, size constraints, DPI handling, and multiple monitors.
- A coherent editor design system for typography, spacing, icons, colors,
  borders, focus, hover, selection, disabled states, validation, menus, dialogs,
  notifications, progress, empty states, scrolling, and keyboard navigation.
- Hierarchy, inspector, assets, console, profiler, learning, scene, and game
  panels plus multiple viewports.
- Plugin-defined panels, property drawers, tools, gizmos, menus, commands,
  overlays, importers, settings, and command search.
- Pezzza runtime drawers remain application presentation surfaces inside the
  simulation view rather than substitutes for editor panels.

## Execution, debugging, and delivery

- Edit, simulate, game/Zen, and training modes with play, pause, step, speed,
  current time, tick count, and active-scene controls.
- Line/bar/histogram/timeline/streaming charts, units, hover values, zoom/pan,
  multiple series, and CSV/image export.
- Frame/simulation/render/grid/memory/task profiling; collision, raycast, field,
  object, vertex, event, and neural inspection; deterministic capture/export.
- Build, package, run, platform settings, validation reports, and logs.

## Testing and automation

- Headless simulation/project validation, deterministic fixtures, and replay.
- Automated editor workflows, stable-region screenshots, golden states, and
  visual comparison boards.
- UI acceptance across compact laptop, 1080p, 1440p, high-DPI, ultrawide, and
  multi-monitor layouts, including overlap, clipping, hidden hit-testing,
  reachability, focus order, and layout restoration checks.
- Performance and memory budgets, sanitizers, resource reload, shader fallback,
  DPI/resize, plugin ABI/reload, serialization migration, and Debug/Release runs.
- Batch training and command-line build/package validation.

## Milestone ownership

| Capability | Owner |
|---|---|
| Neutral public foundation and backend boundary | 17B |
| Data, grids, spatial, RNG, time, tasks, profiling | 17C |
| Physics, collision, raycasts, fields | 17D |
| Assets, textures, rendering, effects, audio | 17E |
| Learning, experiments, checkpoints, live inference | 17F |
| Ant behavior parity | 17G |
| SailBoat behavior/training parity | 17H |
| Pezzza runtime UI/interaction parity | 17I |
| Adoption, cleanup, complete parity validation | 17J |
| General-purpose editor foundation | 18A-18H |
| Modular robotics reference project | 19A-19I |

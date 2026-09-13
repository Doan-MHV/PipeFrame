# Milestone 17 — Engine extraction and Pezzza parity

Status: active; 17A-17I complete. Milestone 16 proved that shared widgets can host the projects,
but it did not complete the engine boundary or reproduce the reference
applications closely enough. Milestone 17 closes those gaps before Ant and
SailBoat are called finished.

## Why this milestone exists

The current projects still expose the rendering backend throughout domain code.
At the planning baseline, 37 Ant source files and 11 SailBoat source files include
SFML directly. Ant also has two project-specific uniform spatial grids with nearly
the same storage scheme. Generic stores, grids, raycasts, physics broadphase,
resource loading, render batching, shader passes, audio, NEAT, training lifecycle,
history, and checkpoint patterns remain outside the engine.

The UI migration also changed the products too aggressively. Current Ant and
SailBoat use one full-height left drawer with tabs. The reference applications
keep the simulation visible and expose compact, independently controlled drawers
on the top, bottom, left, and right edges. In the current 1200-pixel SailBoat
fixture, training time and the network technically exist, but both are buried in
the TRAIN tab; the network view is cramped and shows a genome topology rather
than the reference's live activation presentation. This explains the reported
experience: code presence is not feature parity.

PipeFrame may continue to use SFML privately as its first rendering/window/audio
backend. The goal is that an application uses PipeFrame math, input, assets,
rendering, audio, simulation, and UI APIs. Application code should not need SFML
types or link SFML components directly. An explicit backend adapter is allowed
inside PipeFrame; backend leakage into example logic is not.

## Reference audit

The local `AntPezzaSource`, `SailBoatPezza`, and `AreaPezzaSource` trees are the
primary implementation references. Area is important because it tests whether
an abstraction works beyond ants and sailboats: it contains reusable grids,
stores, signals, async tasks, a thread pool, interpolation, rendering effects,
charts, particles, audio, and a PPO training lifecycle.

The visual and interaction references are:

- [Ant Simulator 2 — Multiple Colonies](https://www.youtube.com/watch?v=TfhrF7VrzWQ)
- [Evolution and Neural Networks for Sailboat Races](https://www.youtube.com/watch?v=4ac1Kh30Ppo)
- [How I create my UIs — an example](https://www.youtube.com/watch?v=Tdua8-9CKck)
- [How to animate anything](https://www.youtube.com/watch?v=Lw8LPXPyrl0)

Exact reference timestamps, intended PNG filenames, and frame-level observations
are persisted in the [video reference index](parity/milestone17/references/README.md).

The Ant video shows the current rule set: follower/explorer roles, energy loss,
death cleanup, return-to-colony behavior, food-powered births, arbitrary colony
count, colony-owned marker cells, foreign-marker erosion/takeover, deterministic
recording, and multi-colony resource competition. Its UI uses a persistent world,
small edge handles, a top timer/profiler, bottom transport/modes, right-side
colony/selection panels, translucent surfaces, and an uncluttered demo state.

The SailBoat video shows an always-readable iteration timer, wind control,
editor/settings drawers, bottom transport, training results, selected-boat data,
and an independent network drawer around a dominant water view. The network
visualization exposes live node values, weighted signed connections, labels, and
topology. Course marks, target guidance, trajectory, water response, boat depth,
sound, follow-best behavior, generalization, and fast training are all visible.

The UI and animation videos establish the design rules: centralized spacing,
parallel nested corner radii, restrained drop shadows, smoked/blurred backgrounds,
consistent translucent text, color used for meaning, clear selection, hover lift,
quick tooltip transitions, and declarative interpolation of position, scale,
color, opacity, and other state.

## Definition of parity

Each reference feature must be recorded in a parity matrix as `matched`,
`missing`, `incorrect`, or `intentional difference`. A feature cannot be marked
matched because a class or widget exists. It must be reachable in the running
application, update from live state, survive the expected workflow, and pass its
behavior or visual check.

Parity has four gates:

1. **Logic:** deterministic scenarios compare state transitions and numerical
   outputs against the matching reference algorithm and configuration.
2. **Interaction:** every reference control and panel is reachable at the same
   stage of the workflow, including during playback and training.
3. **Presentation:** approved screenshots reproduce composition, hierarchy,
   spacing, color, typography, effects, animation end states, and world visibility.
4. **Performance:** the representative population and training workloads meet or
   exceed the recorded reference/PipeFrame baseline without retained growth.

Dynamic simulations cannot use a single whole-frame pixel equality test. Tests
will isolate deterministic world states and UI states, compare stable regions,
and use numerical probes for stochastic behavior. Any deliberate product change
must be listed with a reason and approved before parity is accepted.

## 17A — Forensic parity matrix and reproducible references

Status: complete. The classified baseline, build blockers, numerical conventions,
capture procedure, and closure ledger are in the
[Milestone 17 parity baseline](parity/milestone17/README.md), with separate
[Ant](parity/milestone17/ANT_MATRIX.md) and
[SailBoat](parity/milestone17/SAILBOAT_MATRIX.md) matrices.

- Build and run the original Ant and SailBoat sources where their toolchains and
  assets permit; record any platform blocker instead of guessing behavior.
- Inventory every feature, setting, shortcut, drawer, state transition, renderer,
  simulation rule, training field, persistence artifact, and sound.
- Capture timecoded reference frames for default, hover, pressed, selected,
  open/closed drawer, editor, training, inspection, demo, and Zen states.
- Capture the same states from PipeFrame at matching logical resolutions.
- Record constants, seeds, timestep semantics, coordinate systems, and asset
  provenance. Resolve source/video version differences explicitly.
- Add the matrix to `docs/parity/` and make every later phase close matrix rows.

Acceptance: there is no unclassified Ant or SailBoat reference behavior, and a
reviewer can reproduce both sides of every visual comparison.

## 17B — Backend-neutral public foundation

Status: complete. `PipeFrame::Foundation` now exposes vectors, rectangles,
transforms, colors, angles, time spans, input events, render command data, and UI
view models without linking SFML. Project scene data and runtime input callbacks
use those types. SFML conversion code is isolated under `PipeFrame/Backend/SFML`;
the legacy renderer and the 48 existing Ant/SailBoat implementation adapters are
recorded in
[`SFML_MIGRATION_ALLOWLIST.txt`](parity/milestone17/SFML_MIGRATION_ALLOWLIST.txt)
and cannot grow. Ant links only `PipeFrame::Engine`; SailBoat links
`PipeFrame::Engine` and `PipeFrame::Audio`. The headless contract is compiled and
executed by `FoundationHeadlessRegression`, while `PipeFrameDependencyLint`
rejects public-contract leaks, new example includes, stale exceptions, and direct
example links to `SFML::*`. The remaining allowlist is removal work owned by
17C-17E, not part of the neutral public ABI.

- Add PipeFrame math and presentation value types: vectors, rectangles,
  transforms, colors, angles, time spans, and backend-neutral input events.
- Add explicit conversion only in the SFML backend implementation.
- Stop exposing SFML through public simulation, project-runtime, renderer, UI
  view-model, and callback contracts.
- Split engine targets into public modules and private `Backend/SFML` adapters.
- Add a dependency-lint target that rejects SFML includes and direct SFML links
  in example domain, training, editor, and UI composition code.

Acceptance: a headless example compiles and runs simulation logic without SFML;
Ant and SailBoat application targets depend on PipeFrame modules only. SFML is
visible in the backend and temporary migration adapters listed in the matrix.

## 17C — Generic data, spatial, time, and task services

Status: complete. The backend-neutral Foundation now provides bounded `Grid2D`,
an ID-based `UniformSpatialIndex`, grid DDA raycasts, generational handles and
stores, deterministic named RNG streams, signals, fixed-step scheduling, async
tasks, a blocking thread pool, and profiling scopes. The public API and ownership
rules are recorded in the [runtime services guide](ENGINE_RUNTIME_SERVICES.md).
Both Ant spatial-grid implementations now delegate to the common index, and Ant
wall queries delegate to the common raycast while ant IDs, cells, and collision
policy remain in the project. `SpatialServicesRegression` is the independent
synthetic consumer and checks brute-force radius equivalence, exact world edges,
cell clamping, insert/update/remove/rebuild, stale handles, deterministic streams,
observer removal, fixed-step clamping, task results, and profile aggregation.
The dependency lint covers every extracted header. The full Debug build and all
59 tests pass at this checkpoint.

- Extract `Grid2D<T>`, bounded grid coordinates, uniform spatial index, cell
  ranges, rebuild/update queries, and grid raycasting.
- Replace both Ant-specific spatial grids with one ID-based engine index; keep ant
  data and collision policy outside the generic container.
- Extract stable/generational handles, dense stores, deterministic RNG streams,
  signals/observers, fixed-step scheduling, async tasks, thread pool, and profiling
  scopes where at least two consumers justify them.
- Use Area source as the third design input and add small non-Ant tests so an
  Ant-shaped abstraction cannot pass acceptance.

Acceptance: Ant uses the common spatial index for proximity and collision
broadphase; a separate synthetic workload uses the same API; queries match brute
force at boundaries and under insert/remove/rebuild operations.

## 17D — Reusable 2D physics and environment primitives

Status: complete. `PipeFrame::Foundation` now owns backend-neutral particles,
bodies, circles, segments, collision layers and masks, uniform-index broadphase,
mass-weighted contacts, deterministic integration, bounds constraints, and debug
geometry visitation. It also owns scalar and vector grids with deposition,
sampling, gradients, decay, and diffusion. `PhysicsWorld2D` supplies stable body
creation, lookup, immediate removal, stepping, collision resolution, and dense
iteration. The [physics and environment services guide](PHYSICS_ENVIRONMENT_SERVICES.md)
records the API boundary. The headless `PhysicsEnvironmentRegression` laboratory
exercises the whole lifecycle plus obstacle raycasts and deterministic replay.

Ant's body type extends the common body, `AntPhysicsWorld` delegates integration
and bounds, `ContactSolver` delegates broadphase and circle resolution, the debug
renderer consumes common debug geometry, the wall raycast delegates grid DDA,
and the legacy pheromone field delegates both scalar fields. Ant retains ant IDs,
colony IDs, food/marker ownership, wall policy, speed limits, and synchronization.
The physics, contact, spatial, raycast, and field-facing headers no longer include
or name SFML. Their former migration-allowlist entries were removed, so dependency
lint prevents those leaks from returning. Renderer/resource adapters that still
use SFML are explicitly owned by 17E rather than hidden by this status.
The full Debug build and all 60 tests pass at this checkpoint, including the Ant
stress and UI dashboard stress profiles.

- Extract backend-neutral particles/bodies, circles and segments, collision
  layers, uniform-grid broadphase, contacts, deterministic integration, and
  debug geometry from Ant physics.
- Extract reusable scalar/vector fields, decay/deposition operations, world-grid
  sampling, and obstacle raycasts without embedding pheromone or colony rules.
- Keep ant locomotion, marker ownership, food rules, and colony policy in Ant.
- Provide a small physics/environment laboratory that uses only public PipeFrame
  APIs and exercises creation, removal, collision, raycast, and field updates.

Acceptance: dead bodies disappear from physics immediately, deterministic runs
are repeatable, boundary/contact tests match the reference, and Ant no longer
owns a general collision grid or solver implementation.

## 17E — Resource, rendering, effects, and audio layer

Status: complete. PipeFrame now owns typed and cached texture, font, shader,
audio, render-surface, and image resources behind backend-neutral handles.
`VertexBatch2D`, paths, surfaces, ping-pong buffers, effect passes, shader
fallbacks, particles, debug drawing, and the SFML geometry adapter provide the
shared render layer. Ant texture/font/shader/image ownership and SailBoat
texture/font/shader/render-surface/audio ownership moved into PipeFrame. The
dependency lint rejects those owned SFML resource types in either example's
headers. Resource cache, loss/reload, failed-load, effect-fallback, path,
surface, particle, and debug-draw regressions are recorded in
`ResourceRenderTests`; project renderer and performance regressions preserve
the existing output contracts. The final Debug acceptance run passes 61/61
tests. See `RENDER_RESOURCE_AUDIO_SERVICES.md` for the API and ownership map.
Visual comparison and styling parity remain assigned to 17I.

- Add typed asset handles, caching, lifetime/error reporting, texture atlases,
  fonts, shaders, and audio clips to PipeFrame resources.
- Add sprite and vertex batching, paths/trajectories, offscreen surfaces,
  ping-pong simulation textures, effect passes, blur, shadows, particles, and
  debug drawing behind backend-neutral commands.
- Move reusable Ant geometry/batching/shadow work and SailBoat water/wake/
  trajectory passes into engine facilities; keep domain draw-list construction
  in each application.
- Move mark sounds and future ambient/audio routing through PipeFrame audio.
- Preserve a shader-unavailable fallback and test resource loss/reload.

Acceptance: example renderers and runtimes contain no direct texture, shader,
render-target, sound, vertex-array, or window ownership. Both simulations retain
their visual output and resource lifetime behavior through PipeFrame handles.

## 17F — Reusable learning and experiment runtime

Status: complete. `PipeFrame::Learning` now owns DAG validation, activation,
versioned genomes, compiled networks, mutation, selection, deterministic random
streams, generic evolution, iteration clocks, history, checkpoint metadata,
background evaluation, and policy/environment/trainer interfaces. Network
snapshots expose live values, sums, biases, roles, signed weights, and disabled
edges, including the propagated signal used by Pezzza's network animation.
SailBoat consumes these facilities for its agents, evolution, clock,
history, checkpoints, async evaluation, deterministic resume, and dashboard;
its project-local generic NEAT/history implementation was removed and lint
prevents it from returning. `LearningExperimentTests` supplies the independent
non-boat training, visualization, checkpoint, and deterministic-resume consumer.
The API and Area/PPO adapter boundary are documented in
`LEARNING_EXPERIMENT_RUNTIME.md`. SailBoat behavioral differential parity remains
assigned to 17H and final network presentation remains assigned to 17I. The
Pezzza's source and complete SailBoat/Ant transcripts were re-audited; mutation
candidate pools now match the reference and SailBoat checkpoints preserve the
best score and training configuration. The post-audit Debug acceptance run
passes 62/62 tests.

- Extract NEAT DAG validation, genome/network representation, mutation,
  selection, generation, activation, serialization, and deterministic RNG into
  `PipeFrame::Learning`.
- Extract population evaluation, iteration clocks, progress, statistics,
  checkpoint metadata, history streams, and background-training coordination.
- Define policy/environment/trainer interfaces that can support both SailBoat
  evolution and Area's PPO workflow without placing Torch/CUDA in the engine
  core. PPO remains an optional adapter until a second in-repository consumer
  needs it.
- Make live inference state observable so network views can show activations,
  biases, signed weights, disabled edges, and topology without knowing SailBoat.

Acceptance: a small non-boat optimization example trains, checkpoints, resumes,
and visualizes through public APIs. SailBoat's results remain deterministic for a
fixed seed and no project-local generic NEAT implementation remains.

## 17G — Ant behavioral parity

Status: complete. Source-level rules are covered by focused unit fixtures and a
scripted four-colony parity regression. The regression records full-state
signatures at fixed ticks, reproduces them from the same seed, and proves that
the deterministic one-worker mode and race-free four-worker mode produce the
same state. It also verifies Pezzza's foreign-marker erosion/takeover ordering,
food-funded births, cumulative death accounting, same-tick physics cleanup, and
safe colony removal including owned markers. The update-worker count is exposed
on the Simulation Settings object. Existing encounter, worker behavior,
selection, editor, rendering-option, 10k throughput, and 100k capacity fixtures
complete the behavioral gate. Final composition and appearance remain 17I.
The full Debug suite passes 63/63 tests.

- Verify follower/explorer sampling, marker choice, marker deposition/decay,
  foreign-marker erosion and ownership transfer against source constants.
- Verify energy consumption, exhaustion fallback, trail exception, colony/food
  refill, death, physics cleanup, food resource accounting, and births.
- Verify arbitrary dynamic colony creation/removal, per-colony colors/stats,
  symmetric competition, walls/terrain/food editing, selection/follow, and every
  debug/render setting.
- Implement deterministic single-thread mode and a race-free parallel mode with
  equivalent aggregate results within defined tolerances.

Acceptance: scripted one-, two-, and four-colony scenarios close every Ant matrix
row; long deterministic runs match expected population, food, death/birth, and
marker ownership checkpoints; 10k and 100k stress budgets remain green.

## 17H — SailBoat simulation and training parity

Status: complete. A final source differential corrected two material drift
points: race steering/scoring now targets the first endpoint of each Pezzza mark
instead of the closest segment projection, and generation zero now begins with
identical neutral four-input/one-output genomes before evolution mutates them.
Numerical fixtures cover polar interpolation, wind-relative speed, rudder
integration, bounds, exact progress scoring, ordered endpoint completion,
reset, deterministic exploration seeds, generation rollover, async evaluation,
periodic best saving, checkpoint continuation, and varied-course/wind
generalization. The live async timer advances while work is running. Training
time is a persistent top metric and the live activation graph has an independent
right-edge drawer, both covered by the dashboard interaction regression. The
10,000-boat gate remains green. Final Pezzza styling is assigned to 17I. Current
17H UI evidence is stored at
[`current/sailboat-17h-dashboard.png`](parity/milestone17/current/sailboat-17h-dashboard.png).
The full Debug suite passes 63/63 tests.

- Differentially verify polar lookup, apparent wind, speed, steering, angular
  command integration, crash bounds, gate/segment projection, mark order,
  scoring, completion, reset, and generalization.
- Verify the four network inputs and one output, activation semantics, mutation
  probabilities, selection, generation rollover, new exploration, best-agent
  choice, async training, history, save/load, and deterministic resume.
- Match editor state transitions for waypoint, start direction, and finish; match
  follow-best, best-only, world visibility, target arrow/labels, trajectories,
  water/wakes, boat depth, wind, and mark sounds.
- Restore a continuously visible iteration/simulation timer and an independently
  accessible live network panel. They must update while training and remain
  legible without scrolling through unrelated content.

Acceptance: numerical fixtures compare the complete race/training lifecycle to
the reference; a 10,000-boat run is stable; timer, network, course editing,
generalization, checkpoint resume, and playback are demonstrated in one automated
interaction recording and corresponding assertions.

## 17I — Visual and interaction parity

Status: complete. The source/video audit, corrected composition, and retained
720p/1080p/1440p/narrow captures are recorded in the
[visual parity audit](parity/milestone17/VISUAL_PARITY_AUDIT.md) and
[comparison board](parity/milestone17/COMPARISON_BOARD.md).

- Replace the single full-height tab drawer with compact white edge buttons.
  Activating one button displays one complete popup with a CLOSE action and
  suppresses the other buttons until that popup closes.
- Keep the world visible while timer, profiler, wind, settings, editor, colony,
  selected entity, training result, boat info, and network surfaces are used.
- Implement the reference design grammar in engine primitives: proportional UI
  scale, compact handles, smoked/blurred cards, parallel radii, consistent
  spacing, hierarchy-aware typography, meaningful accent colors, subtle shadows,
  hover lift, selection borders/pills, fast tooltips, and eased drawer motion.
- Give Ant and SailBoat distinct reference palettes and compositions while using
  the same engine primitives. A shared component must not force identical apps.
- Preserve full editor, simulation, settings, and Zen/demo modes with keyboard,
  pointer, focus, and capture behavior.

Acceptance: reviewed reference/current comparison boards pass in the all-collapsed
state and with representative popups open at 720p, 1080p, 1440p, narrow layout,
and at least two UI scales. Panel access, exclusive popup behavior, hidden handles,
and animation end states have interaction tests. No essential information is
clipped, hidden behind a tab, or wrapped into an adjacent control.

## 17J — Adoption, hardening, and completion gate

- Port ThermalLab and a small Area-inspired arena/training harness to prove the
  extracted APIs outside Ant and SailBoat.
- Remove migration adapters, duplicate grids/NEAT/render utilities, obsolete
  assets, and project-level SFML links only after parity checks are green.
- Run sanitizers, deterministic replay, serialization compatibility, resource
  reload, resize/DPI, GPU fallback, interaction, screenshot, and stress suites in
  Debug and Release.
- Publish dependency and ownership diagrams, public API examples, parity matrix,
  benchmark captures, known platform limits, and attribution/license records.

Acceptance: all parity rows are matched or explicitly approved differences; Ant,
SailBoat, ThermalLab, and the Area-inspired harness use public PipeFrame APIs;
example domain/UI code has zero direct SFML includes and links; complete Debug
and Release suites pass; visual boards are approved; performance does not regress.

## Required order and stop conditions

17A is mandatory before implementation. 17B–17F may proceed in small vertical
slices, but every extraction must migrate a real consumer and add a second or
third proof before duplicate code is removed. Ant and SailBoat parity work starts
against the matrix as soon as the relevant foundation exists; it does not wait
for a speculative framework rewrite.

Do not accept a phase when only compile-time structure exists, when a feature is
present but inaccessible, or when tests exercise synthetic state instead of the
running workflow. Do not broaden PipeFrame with project nouns or copy Area's PPO
stack into the core. Do not remove a known-good implementation until behavioral,
visual, and performance comparisons pass.

The project is finished only at 17J. Intermediate green test counts show that the
current behavior is internally consistent; they do not establish Pezzza parity.

## Scope boundary and editor follow-on

Milestone 17 delivers reusable runtime systems and Ant/SailBoat parity. It does
not claim every Unity/Unreal-style authoring feature. The complete extraction and
editor checklist is stored in
[`PIPEFRAME_REUSABLE_SYSTEMS_INVENTORY.md`](PIPEFRAME_REUSABLE_SYSTEMS_INVENTORY.md).
Viewport grids/gizmos, component hierarchies, advanced inspectors, asset imports,
prefabs, docking, editor extensions, multi-scene authoring, and build/package
workflows are explicitly owned by
[Milestone 18](MILESTONE_18_PLAN.md).

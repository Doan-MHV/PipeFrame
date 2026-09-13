# R3 runtime migration audit — 2026-09-12

The Ant ECS ownership migration is implemented and verified against the existing
engine/editor/Ant regression suite. This is not completion of the overall rework,
R2's live Inspector workflow, or a claim of frame-for-frame Pezzza parity.

## Runtime ownership

- AntEntity composes AntIdentityComponent, AntPoseComponent, EnergyComponent,
  ForagingComponent, AntEncounterComponent, Transform2DComponent and Motion2DComponent.
- ColonyEntity composes ColonyStateComponent, ColonyHistoryComponent,
  ColonySettingsComponent and Transform2DComponent in that same engine scene.
- AntView and ColonyView derive from the engine's ComponentView. They borrow
  scene components; they own no simulation state. Copies alias an entity. Copy
  a component explicitly when a detached data snapshot is required.
- AntQuery uses the engine's SceneViewCache. The cache stores handles and an ID
  index, not a second population or ECS world. Spans and references from this
  cache expire on structural refresh; retain a view by value for a durable handle.
- Components/Ant.h/.cpp, Components/Colony.h/.cpp, AntStore,
  AntSimulationPipeline and the ComponentValue adapter are removed.
- The runtime holds movement, foraging and cleanup systems directly. The existing
  plugin SystemRegistry dispatches the guarded scene phases. Colony spawning is
  an attached ColonySpawnerBehaviour. Headless updates use those same systems.
- Ant geometry, selection, previews, colony telemetry and history now consume
  scene views. AntGeometry emits neutral PipeFrame Vertex2D streams; dependency
  lint forbids backend references in the geometry producer. The engine adapts
  those streams for legacy draw hosts. Remaining draw/UI hosts are R5/R6 work.

## Shared engine services

ComponentView caches typed pointers and revalidates entity identity when storage
changes; it checks scene lifetime on every access. A view instance is confined
to one thread. Different copies can be used during batches without structural
mutation. Tests cover relocation, deletion and numeric ID reuse.

SceneViewCache supplies borrowed population indexing. Transform2DComponent and
Motion2DComponent provide shared spatial data. DirectionTracker and SeekVelocity
provide shared interpolation and steering. BodyStorage supplies dense physics
body allocation, lookup and compaction, consumed by AntPhysicsWorld and the
independent engine PhysicsWorld2D. Ant-specific collision/marker rules remain
project logic. Solver bodies are physics working state; movement writes solved
position and motion telemetry back into scene components at the phase boundary.

## Structure and naming

Components now contains only role-named state and its colocated domain records.
AntRole lives with AntIdentityComponent; ForagingState lives with
ForagingComponent; AntLegPose lives with AntPoseComponent. Pose calculations live
under Systems/AntPoseAlgorithms.h and Systems/AntLegPose.cpp. Generic direction
tracking lives in the engine. Registration and type IDs moved to Runtime;
ColonySettingsComponent and AntForagingSystem use explicit role names.

## Verification and performance observations

Build passed. The optimized runtime run passed all 61 tests in 60.05 seconds.
After neutral geometry migration, the final full run passed all runtime and
rendering checks in 58.58 seconds; lint identified a stale geometry allowlist
entry. Removing that entry and rerunning lint passed, completing all 61 checks.
SailBoat was excluded. Existing behavior, contact, marker, food delivery, colony
lifetime, deterministic/parallel replay, rendering and Inspector telemetry tests
were retained; old value-model fixtures now instantiate actual scene recipes.
New engine tests exercise an unrelated sensor-like component view, relocation,
destruction, ID reuse, shared steering and direction tracking.

| Same stress workload | Recorded pre-R3 run | Initial R3 run | Optimized runtime run |
|---|---:|---:|---:|
| Create and synchronize 100K ants | 509.789 ms | 1121.71 ms | 988.01 ms |
| 10K ants, 30 ticks | 7.34022 s | 7.74363 s | 6.03973 s |
| Ant updates/second | 40,870.7 | 38,741.5 | 49,671.1 |
| 2K ants, 600 ticks | 55.7201 s | 56.5189 s | 51.3214 s |
| Suite wall time | 64.64 s | 66.80 s | 60.05 s |

The initial R3 lookup cost was reduced with cached, version-validated component
access. Capacity creation remains slower: each ant now creates seven component
pools' entries instead of four, plus a borrowed view. This is a visible creation
cost of the decomposition; no passing numerical threshold was invented to hide
it. These sequential local measurements are regression observations, not a
controlled benchmark or a claim of universal speedup. The earlier 57.35-second
pre-Energy run is also retained in the progress history.

## Reference boundary and remaining work

Re-read the supplied current AntPezzaSource/src/simulation/simulation.hpp,
ant/ant_updater.hpp, colony_updater.hpp and ant/tracking_direction.hpp during
this migration. The source confirms the 0.3 steering blend, elapsed energy
consumption, colony funding and radius formula, and angular tracking rule.
The migration preserves the existing PipeFrame regression behavior.

There are pre-existing reference differences that the older milestone matrix
must not obscure: the supplied Pezzza simulation steps physics before the ant
update and colonies last; PipeFrame begins with environment/colony lifecycle,
then movement and behavior. Pezzza's updater measures travel using velocity times
dt; PipeFrame measures the solver's displacement and clamps remaining distance.
These are recorded reference-validation work for R0, not new behavior introduced
by moving state to components. Full visual/source acceptance remains open.

R1 still needs broader lifecycle/scheduler consolidation. R2 still needs generic
live property registration, Inspector transactions, undo and persistence. R4/R5
still need the complete declarative widget runtime and UI migration. R6/R7 still
need generated authoring, backend isolation and final whole-project acceptance.

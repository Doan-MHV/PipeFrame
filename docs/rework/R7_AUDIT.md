# R7 removal, documentation and completion audit

September 13, 2026. **R7 complete / verified.** All nine rework acceptance scenarios
have same-source regression evidence. Supported scope and limits are explicit below.
SailBoat is not changed, built or tested by the configured audit.

## Removed and retained deliberately

Removed `Simulation/Agent2D.h`, `AgentGroup2D.h`, and `EntityStore.h`. These were unused
prototype aggregate owners; the only source consumer was a foundation test. The workspace
search included engine, apps, examples and local projects. The test now uses the actual
BehaviourScene, SceneObject, Transform components and ECS queries instead. All foundation
lifecycle checks now execute in Release too, including mutations previously inside assert.

| Retired API | Supported replacement |
| --- | --- |
| Agent2D position/velocity owner | EntityArchetype composes Transform/Motion components; Behaviour or batch systems execute rules |
| AgentGroup2D membership owner | Project component holds group state; scene entities and a system maintain membership |
| EntityStore's optionally owned second scene | BehaviourScene owns lifetime; SceneViewCache/ComponentView borrow its entities |
| Generated ComponentContract/SystemContract/ExtensionContract headers | Folder READMEs explain real schema, world/system and UI/tool contracts; Generate Source creates the chosen implementation |
| Generated empty `Tests/ProjectTests.cpp` | Tests README explains adding real executable tests and CTest entries; no fake passing test is emitted |

No compatibility forwarding headers remain for the retired aggregate APIs. This is a
source migration; existing external source users must use the replacements. No installed
SDK or published package was modified. ABI is **13** because Behaviour now owns coroutine tasks and exposes service callbacks;
existing plugins must be rebuilt. The earlier native Core/Scene/Application/Input and
imperative widget APIs remain under Backend/SFML: they have real desktop-host/rendering
consumers and must not be confused with project authoring APIs. EntityUpdateSystem is
retained for its out-of-scope legacy consumer; it receives no Ant-rework completion credit.

## Real consumers of the rework's abstractions

| Public abstraction | Real consumer and responsibility |
| --- | --- |
| BehaviourScene, SceneObject, EntityArchetype | AntRuntimeWorld owns the scene; AntEntity/ColonyEntity compose data; CounterEntity proves independent use |
| ComponentRegistry/ComponentSchema | AntRegistration registers component-owned schemas; Workbench Inspector and scene serialization consume them |
| ComponentView/SceneViewCache | AntView/ColonyView/AntQuery borrow typed data; foundation tests cover relocation, removal and stale handles |
| Behaviour / Coroutine / service callbacks | BehaviourServicesAcceptance exercises real contact, trigger and camera transitions plus cancellation; ColonySpawnerBehaviour and SignalBeaconBehaviour execute lifecycle logic; independent CounterBehaviour consumes UI-edited data |
| World/RuntimeWorld | AntWorld declares phases; AntRuntimeWorld registers environment/colony updates around scene lifecycle |
| PhysicsWorld/FixedUpdateSystem/PhysicsSolver | AntPhysicsWorld, AntMovementSystem and ContactSolver execute domain physics through ordered engine contracts |
| RenderingWorld/RenderLayer/Canvas | AntRenderingWorld composes real geometry layers with backend-neutral submissions |
| SystemRegistry/FixedStepSequence | AntSimulationRuntime registers runtime phases; AntWorld enforces phase sequence; world-module tests check order |
| SimulationController/FixedStepScheduler | Workbench play/pause/step/speed/reset; simulation-controller and UI-input tests |
| RuntimeModule | Asset/runtime modules and generated opt-in module template; registration does not imply automatic scheduling |
| ServiceRegistry/EnvironmentQueries | SceneProjectRuntime supplies scene-local queries; the compiled blank-project ProbeBehaviour consumes them |
| UniformSpatialIndex/BodyStorage/CircleContactWorkspace | Ant avoidance/contact solvers and generic PhysicsWorld2D; exact reuse/contact and stress checks |
| TilemapAssetModule/VisualAssetModule/shared geometry queries | Generated SceneProjectRuntime and Ant authored environments consume materials, cells, collision and ray/sweep queries |
| EditorTool/BrushTool/SchemaBrush | Shared map gesture controller and AntFoodBrush; generated brush build/reload fixture |
| ViewPanel/View/StatefulView/ViewSource/MountedView/State | Workbench presenters, AntDashboard, mounted lifecycle tests, and the independent CounterPanel |
| GraphicsResourceService/RenderSurface/InputEvent | Ant's neutral resource/render/input integration and host backend boundary compile checks |
| SampleHistory/DeterministicRandom/DirectionTracker | ColonyHistory, Ant replay/pose systems and independent foundation tests |

None of the retained project-facing contracts is justified solely by its name or an ID.
Some plain-data types deliberately do not inherit a virtual base. System registration,
world ownership and UI mounting are explicit; the engine does not execute arbitrary files
by scanning folders.

## Nine-scenario evidence map

| Scenario | Specific verification |
| --- | --- |
| 1. Blank and Ant environment authoring; scene/asset persistence and transport | BlankProjectAcceptance, AntEnvironmentAuthoringAcceptance, SceneModelRegression, SimulationControllerRegression, WorkbenchUIAcceptance; retained native R6 map/build captures |
| 2. Colony and individual-ant selection; supported properties and persistence | AntComponentEditingAcceptance exercises live Transform/Settings, validation, undo/redo and serialized values; AntInspectorRegression and AntColonyInspectorRegression check runtime selection/telemetry. Runtime ants and telemetry are not authored records; read-only fields are intentionally not editable or serialized. |
| 3. Generate, attach, build/reload and recover failures | ProjectBuildAcceptance, BlankProjectAcceptance, SceneProjectRuntimeAcceptance, ProjectManagerRegression; stale ABI and failed compiler/cancellation recovery retained |
| 4. A new nested stateful panel without project-side coordinates/backend types | IndependentAuthoringAcceptance compiles the published CounterProject.h, mounts it, clicks a real control and checks the attached Behaviour consumes the changed component |
| 5. Resize, scroll, pointer/keyboard and input ownership | MountedViewRegression, UIFrameworkRegression, UIHardeningRegression, WorkbenchUIAcceptance, AntDashboardAcceptance; existing native desktop captures retained |
| 6. Population lifecycle, deterministic/reference checks and performance | FoundationHeadlessRegression (always-active Release checks), AntStressRegression, AntBehaviorParityRegression, AntSimulationWorldRegression and three unchanged performance runs |
| 7. No Ant backend allowlist exception | PipeFrameDependencyLint, AntUIBoundaryCompile, RenderPublicHeadersTests, public Engine consumer and individual header compile targets |
| 8. Discoverable current structure | Rewritten Ant README and folder guides, current schema example, explicit World hierarchy and registration map; retired names appear only in migration/history notes |
| 9. Independent components + Behaviour + stateful UI, no Ant linkage | IndependentAuthoringAcceptance links only Engine/backend host; IndependentAuthoringPublicCompile checks the exact project header without backend paths; generated blank project uses the same public APIs |

A runtime ant is generated from colony settings, so this evidence does not claim arbitrary
per-ant authoring/save support. Native interaction captures and programmatic editor API
checks are identified separately in [R6's acceptance report](R6_BLANK_PROJECT_ACCEPTANCE.md).

## Learning documentation

- [Ant source map and authoring guide](../../examples/AntSimulation/README.md)
- [Complete blank-project lesson](../tutorials/blank-project/README.md)
- [Independent scene/Behaviour/stateful panel lesson](../tutorials/independent-ui/README.md)
- [Lifecycle tasks and service events](BEHAVIOUR_LIFECYCLE.md)
- [Component-owned schema convention](COMPONENT_SCHEMA_CONVENTION.md)
- [Ant reference boundary](../../examples/AntSimulation/PARITY.md)

## Limits of the verified claim

The original lifecycle gap is now implemented and exercised: Behaviour owns simulation-time
coroutines, and SceneProjectRuntime dispatches actual kinematic collision/trigger and
camera visibility transitions. See [lifecycle contracts and limits](BEHAVIOUR_LIFECYCLE.md).
Custom World solvers explicitly supply their own event semantics; the engine does not
infer events from arbitrary project code. Ant keeps its batch population physics.
Likewise, generic Renderer2D metadata alone is not a complete sprite-rendering workflow;
the blank project uses the implemented environment shape rendering path.

Pezzza update ordering and displacement differences remain documented reference policies,
not a claim of perfect source/video equivalence. The supplied videos were not rewatched
in this cleanup. No Ant physics/foraging algorithm, reference tolerance, population or
performance threshold was changed to pass R7. Full Unity/Unreal/Flutter parity, per-ant
save authoring, and Milestone 19 robotics are not established by these nine scenarios.

## Final verification

The first full ABI-13 Release pass found a stale SceneObject traversal when an object
was destroyed between ticks. The lifecycle fixture exposed it, and SceneProjectRuntime
now prunes externally destroyed entries before physics and skips inactive objects in
render/query traversal. Reciprocal contact transitions are deduplicated; a surviving
receiver gets exit even if the other endpoint was destroyed. The failing checkpoint is
retained in `evidence/r7/release-final-tests.txt`. Final verification follows the fix.

Final Debug and Release builds pass, with **79/79 CTests in each configuration**.
After the contact-pass correction, Release took 41.06 s; Debug took 98.94 s. The independent panel screenshot was rendered
through the real host and inspected: its card, button and wrapped help text are distinct
and readable. Clicking updates the component and retained state; resizing and reset are
asserted by the same fixture.

- [Release build](evidence/r7/release-contact-build.txt), [Release tests](evidence/r7/release-contact-tests.txt)
- [Debug build](evidence/r7/debug-contact-build.txt), [Debug tests](evidence/r7/debug-contact-tests.txt)
- [Independent panel](evidence/r7/independent-panel.png), [interaction result](evidence/r7/independent-panel.txt)
- [Source SHA-256 manifest](evidence/r7/source-sha256.json), [documentation link check](evidence/r7/doc-links.json)
- Ant backend-reference and retired-API consumer scans both returned no matches;
  full dependency lint and public-header compile targets passed in both builds.

The first three-repeat performance check passed 11/12 fixture executions, but the third
five-colony p95 was **16.8255 ms** against 16.67 ms. All raw samples remain in
`evidence/r7-performance`; earlier repetitions were 16.0676 and 16.0616 ms. This failed
checkpoint was not averaged away or replaced by an unchanged retry.

Inspection found two redundant full-population position publications in ContactSolver.
Wall and circle solvers consume body storage, with no intermediate ECS position reader.
The pass now publishes only after both solves. The final position regression checks every
body against its scene component; solver order, candidate order and calculations stay
unchanged. No fixture population, warmup, speed, timestep, worker count or budget changed.
Final same-source suites and all 12 fixture executions across three repetitions pass after
this correction. The existing Workbench process was temporarily suspended during each
benchmark run and resumed afterward, preserving its scene; builds and CTests were finished.


### Final measured results

Apple M3 Pro / 18 GiB, macOS 26.6.2 arm64, Release. Same R6 seed, population, timestep,
resolution and warmup. Raw samples and runner metadata:
[final results](evidence/r7-performance-final/results.json),
[earlier failed checkpoint](evidence/r7-performance/results.json).

| p95 frame time, ms | Run 1 | Run 2 | Run 3 | Budget |
| --- | ---: | ---: | ---: | ---: |
| 1 colony | 5.216 | 5.069 | 5.068 | 16.67 |
| 3 colonies | 10.983 | 11.643 | 11.067 | 16.67 |
| 5 colonies | 16.420 | 16.125 | 16.569 | 16.67 |
| Selected Inspector | 2.129 | 2.076 | 2.509 | 16.67 |
| Inspector scrolling | 1.937 | 1.950 | 1.939 | 16.67 |
| Colony dragging | 3.126 | 3.146 | 3.468 | 16.67 |
| 384×216 map painting | 1.110 | 1.088 | 1.133 | 16.67 |
| 1024×1024 map painting | 8.897 | 9.240 | 9.379 | 16.67 |

Large-map peak memory was at most **467.156 MiB**, below 512 MiB. Load/save gates also
passed (large-map maximum load 195.743 ms, save 157.628 ms). Matched live populations
were 999 / 3023 / 4832. Five-colony worst-case headroom is **0.101 ms**: the agreed
reference gate passes, but this is a narrow margin, not a universal 60 FPS guarantee or
proof of a statistically significant speedup. Physical input-to-photon latency was not
measured. No failing checkpoint was discarded.

The public API is ABI 13. Restart any older Workbench instance and use Build & Reload
for existing project plugins. R7 closes this engine/editor/Ant rework scope; documented
reference differences, complete Unity/Flutter feature equivalence, and Milestone 19
robotics are not claimed by this completion.

## September 13 — R7 complete: cleanup, lifecycle and independent reuse audit

R7 removed unused Agent2D/AgentGroup2D/EntityStore prototypes and empty generated
contract/test files; current templates and Ant guides use scene recipes, component-owned
schemas, world systems and declarative views. The independent Counter example compiles
without Ant/backend authoring dependencies and verifies mounted stateful UI changing real
component data consumed by a Behaviour. Its rendered screenshot is retained.

The missing Behaviour coroutine and service-event contract is now implemented, with real
kinematic collision/trigger and render-camera callbacks, cancellation and callback-safe
destruction tests. ABI is **13**; rebuild plugins. Specialized world solvers retain their
explicit event integration contract. Ant contact solving now publishes final ECS positions
once, eliminating two unread intermediate population copies without changing calculations.

Final **79/79 Debug and 79/79 Release** checks pass, plus all three unchanged performance
repetitions: five-colony p95 **16.420 / 16.125 / 16.569 ms**. The 0.101 ms minimum margin
is narrow. Earlier lifecycle and performance failures are retained alongside the fixes.
**R7 is complete; the scoped engine/editor/Ant rework gates are verified.**
[Final evidence, all nine scenarios, API migration and limits](R7_AUDIT.md).

Earlier entries below are historical checkpoints, not current status.

## September 13 — R6 complete: final performance verification

The shared circle-contact solver now retains its spatial-grid workspace per world.
Ant and PhysicsWorld2D use it; collision order and numerical results are unchanged.
ABI 12 protects the PhysicsWorld2D layout change. Final Debug and Release builds and
**77/77 tests each pass**, including exact reused/fresh contact comparisons.
All three unchanged performance runs pass. Five-colony p95:
**16.356 / 16.185 / 16.491 ms** against 16.67 ms; other editor/map gates also pass.
Raw samples and the earlier failed checkpoint are retained. The remaining 0.179 ms
headroom is a reference-machine pass, not a universal frame-rate guarantee.
**All eight R6 packages are verified. R6 is complete; R7 is next.**
[Final implementation and evidence](R6_FINAL_PERFORMANCE.md).

Earlier entries below are historical checkpoints, including the intervening reopened gate.

## September 13 — R6 blank-project acceptance and learning documentation

Package 8 is complete: a generated project authors a maze/material/markers, attaches
component-owned settings and query-driven Behaviour, and passes play/pause/reset,
prefab/save/reopen/build/reload acceptance. The guide's exact source files are compiled
by the test. Native painting, Inspector, motion/reset and rebuild captures are retained;
the report distinguishes native coverage from the integrated editor API scenario.
Generated Behaviours can use scene-local EnvironmentQueries through GetService<T>();
plugin ABI is 11. Final Debug and Release: **77/77 each**. Complete learning project:
`projects/R6LearningArena`. [Guide and evidence](R6_BLANK_PROJECT_ACCEPTANCE.md).

**R6 remains open:** the same-build performance recheck returned five-colony p95
16.521 / 16.241 / **16.707 ms** (16.67 ms target). Other fixtures passed. Package 7
is reopened for sufficient margin and another complete matched verification. No failing
run was omitted. R7 is still the subsequent audit. Older entries below are checkpoints.

## September 13 — R6 measured performance gates

Package 7 is complete. Three matched Release runs pass editor scrolling/dragging,
1/3/5-colony simulation, and small/large map frame/load/save/memory gates. Worst
five-colony p95: 16.56 ms (16.67 ms target); worst large-map peak: 466.67 MiB
(512 MiB budget). Native scroll/drag/paint/undo evidence is saved; physical
input-to-photon latency is not instrumented.
Tilemap geometry capacity is reused across edits and redundant full-map copies are
avoided. No simulation quality/count/timestep reduction. Debug 76/76 plus affected
recheck; final Release 76/76. A Release-only test setup/assert bug was corrected.
See [performance report and reproducible runner](R6_PERFORMANCE_GATES.md).
R6 package 8 (blank-project acceptance and learning documentation) remains open.

## September 12 — R6 Ant editor-authored environments

Package 6 is complete. Ant ships a shared Playground/Tilemap entity and consumes
editor-painted terrain and food; save/reopen/reset and invalid-reimport recovery are
verified. Native Frame Selection now includes full Playground bounds.
Debug: 76/76; affected rechecks: 8/8; Release: 9/9; native evidence inspected.
See [migration workflow and evidence](R6_ANT_ENVIRONMENT_AUTHORING.md).
R6 packages 7 (performance) and 8 (learning/closeout) remain open. Plugin ABI stays 10.

## September 12 — R6 shared environment collision and queries

Package 5 is complete: shared tile/box/segment queries, translating obstacle CCD,
Ant wall sweep/slide, source hit identity and changed render chunks. Debug: 75/75;
final affected checks: 7/7 Debug and 8/8 Release. Current plugin ABI is 10.
See [supported geometry, API and evidence](R6_ENVIRONMENT_COLLISION.md).
Packages 6–8 remain open; older entries below are historical checkpoints.

## September 12 — R6 neutral Workbench panel presenters

Six editor presenters now use neutral ViewPanel and the same declarative View API.
Their actual sources compile without backend include paths. Plugin ABI is 6.
Debug/Release builds and 69/69 tests pass (97.49 seconds); native UI evidence was saved
and inspected. Native window/dock/widget interfaces remain open. Tile editor panels
must use this same API. See [presenter implementation](R6_PANEL_PRESENTERS.md).

## September 12 — R6 neutral dashboard and controller slice

Ant's dashboard facade/theme and actual Ant UI source now compile without native
include paths. CameraController2D uses neutral InputEvent and owns its pan modifier.
Plugin ABI is now 5. Debug suite: 69/69 passed (93.59 seconds); Ant native drawer
interaction checks passed. Package 1 remains in progress for Widget/ViewPanel and
Workbench headers. See [implementation and remaining work](R6_UI_BOUNDARY.md).

## September 12 — approved R6 render boundary

Camera2D/RenderContext now expose PipeFrame types and delegate native operations to the
engine adapter. Plugin ABI 4 requires rebuilding project libraries. The remaining
native UI/header migration is still open; see [the current boundary report](R6_RENDER_BOUNDARY.md).

# Engine rework progress — Ant first

## Latest R6 build-workflow checkpoint

Native build/cancel/reload and generated-plugin persistence/recovery are implemented;
68/68 engine/editor/Ant tests pass. See [current R6 status](R6_AUTHORING_PROGRESS.md).
The broader render-boundary API migration remains unapplied pending approval. R6 and R7
are not marked complete. Release 1/3/5-colony offscreen measurements now fit the 16.7 ms
frame budget after a deterministic-parity-preserving avoidance lookup optimization.


## Latest world-composition checkpoint — September 12, 2026

Ant now uses `World/AntWorld`, `World/Physics/AntPhysicsWorld`,
`World/Rendering/AntRenderingWorld` and `World/Runtime/AntRuntimeWorld` with executable
PipeFrame module bases. All Ant Source direct backend references have been removed and
are guarded by dependency lint. See [the current architecture and boundaries](ANT_WORLD_COMPOSITION.md).
R6 remains open for the integrated build/reload workflow, broader editor public-header
isolation and the previously recorded performance acceptance. Earlier counts and paths below
are historical snapshots, not the current Ant folder layout.


Governing plan: [Engine rework plan](../ENGINE_REWORK_ANT_PLAN.md).

## Current status overview

| Area | Status | Remaining gate |
|---|---|---|
| Scope and baseline (R0) | Ant-only rework build/test mode and control baseline implemented | Complete reference-divergence inventory and screenshot matrix |
| Scene/lifecycle (R1) | Shared ant/colony storage, checked handles, activation/lifetime hierarchy, destruction and phase guards implemented | Transform hierarchy, older scene/runtime API consolidation, complete scheduling integration |
| Properties (R2) | Documented colony-editing gate verified: typed registry, live discovery, validated edits, undo/redo, save/reopen and play/reset; 64/64 checks passed | Generated registration/attachment workflow remains R6; panel migration verified in R5 |
| Ant ECS runtime migration (R3) | Implemented and verified: scene-owned components, borrowed views, direct systems, shared services and neutral geometry | Full Pezzza comparison remains R0; native draw/UI host isolation remains R6; transient Ant panel migrated in R5 |
| Declarative UI runtime (R4) | Mounted lifecycle, automatic nested rebuilds, keyed disposal, constrained layout/input and schema-generated Inspector implemented; R4 gate verified, 62/62 checks passed | Ant/editor UI content conversion verified in R5; native host boundary remains R6 |
| Ant/editor UI migration (R5) | Implemented and verified: editor and all six Ant panels use shared declarative views; 64/64 tests passed; saved interaction and visual evidence | R5 UI-content gate complete; native host isolation and generated authoring remain R6 |
| Authoring/completion (R6–R7) | Incomplete | End-to-end create/build/reload/attach workflow, backend isolation, obsolete-path removal and final demonstration |

No overall completion percentage is assigned: verified slices do not imply that their entire stage is complete. SailBoat remains excluded. The engine remains general-purpose; Ant is the current integration reference.

## Reproducible build configuration

Configure the existing development build with:

```sh
cmake -S . -B cmake-build-debug -DBUILD_TESTING=ON -DPIPEFRAME_ANT_REWORK_ONLY=ON
cmake --build cmake-build-debug -j 6
ctest --test-dir cmake-build-debug --output-on-failure -j 6
```

This option excludes SailBoat runtime/build targets, its tests, and its cases in shared reference and Workbench integration tests. Dependency lint audits the active Ant scope. General engine/editor and small existing infrastructure examples remain available as independent reuse checks. The option defaults OFF for existing non-rework builds; the current development cache is ON.

AntDashboardAcceptance now belongs to Ant and does not link SailBoat. It checks each drawer's open/close controls and press/release capture across a render refresh, and writes `cmake-build-debug/ant-rework-baseline.png`. The old mixed dashboard tests remain outside the rework configuration. This is a control baseline, not a claim of visual parity or a completed declarative UI migration.

## R0 — in progress

Ant-only build/test isolation implemented. Existing AntBehaviorParityRegression covers deterministic replay, parallel parity, marker transfer, colony removal and death accounting. AntStressRegression and AntRenderPerformanceRegression cover current population/render workloads. A complete recorded Pezzza divergence inventory and world/UI screenshot matrix remain outstanding.

## R1 — in progress

The existing ECS scene now provides checked SceneObject access: component access/addition, behaviour attachment, activation, destruction and retrieval of its entity. Objects validate scene lifetime and a per-incarnation revision. Explicit numeric-ID reuse and scene Clear cannot revive an old SceneObject. Legacy numeric IDs still exist during migration; this is not a completed opaque-handle migration.

Behaviours can retrieve their SceneObject. Scene object activation controls attached lifecycle execution. Ant colony construction uses the checked facade. Engine tests cover stale references after destruction, reuse, clear and scene teardown, plus activation lifecycle order and unrelated-object operation.

Remaining R1 gates: consolidate Core/Scene and ECS lifecycle, hierarchy semantics, deferred component mutation and scheduler integration. Ant/colony entity storage now shares one scene; Ant-specific environment/physics services and compatibility classes still require further migration. The compatibility AntStore and AntSimulationWorld remain. No claim of full scene consolidation.

R2/R3 now have an initial component slice; their end-to-end acceptance gates and R4–R7 remain unfinished. No new SailBoat work is part of this scope.

### Shared scene ownership — implemented

AntSimulationWorld::State now owns one BehaviourScene. AntStore borrows that scene through the engine EntityStore API; ColonyLifecycleSystem uses the same scene rather than owning a second one. Standalone EntityStore tests can still use an internally owned scene.

Colony IDs remain domain IDs and map to checked SceneObjects. They no longer serve as raw ECS identities. Ant recipe creation goes through BehaviourScene::Instantiate, and ant deletion goes through scene lifecycle handling. EntityStore::Clear destroys only entities carrying its stored component; it does not clear the whole shared world. Colony cleanup removes its colonies and their ants while preserving unrelated objects.

The colony controller detaches its scripts during destruction so their references cannot outlive the controller. Its former captured registration factory was removed; typed behaviour attachment supplies the same lifecycle execution without leaving a dangling factory in the shared scene.

Tests cover shared ownership, scoped clearing, stale handles after simulation teardown, and domain IDs that numerically collide with unrelated ant entity IDs. Legacy numeric ant IDs may differ from the old separate allocation sequence; colony ownership remains stable. Existing deterministic/parallel parity checks validate behavior under the new sequence.

Next: bring hierarchy and scheduling under the public scene model, then migrate monolithic Ant/Colony state into registered components. The remaining AntSimulationWorld wrapper is not evidence that all runtime responsibility has been extracted.

## Verification — 2026-09-11

- Rework configuration build passed.
- 61/61 registered tests passed in 57.86 seconds, including scene-reference safety, activation lifecycle, Ant behavior parity, Ant stress, editor integration and the standalone Ant dashboard interaction test.
- No SailBoat runtime was built or loaded by this configuration's checks.
- Saved and visually inspected the [1000×800 collapsed controls baseline](baseline/ant-controls-1000x800.png). This image intentionally captures the dashboard alone; it does not show the simulated world or claim final UI quality.
- R0 and R1 remain in progress for the outstanding gates described above.

### Shared ownership verification

After the shared scene migration, all 61 rework checks passed in 56.78 seconds. This includes the new colony/ant numeric-ID collision and scoped cleanup tests, simulation-handle invalidation, deterministic/parallel behavior parity, Ant stress, and Ant-only dashboard acceptance. R1 remains in progress; this verifies shared entity storage, not full scene/scheduler consolidation.

### Hierarchy and activation — implemented slice

SceneObject now exposes SetParent, DetachFromParent, GetParent, GetChildren, IsActiveSelf and effective IsActive. Parenting rejects cross-scene objects, cycles, and objects pending destruction. Parent links validate object revisions, so reused legacy numeric IDs cannot silently inherit old parent links. Child enumeration is stable by entity ID.

Destroy queues descendants before ancestors and freezes the subtree against reparenting during teardown. Scene Clear uses the same destruction path, including objects without behaviours. Effective activation inherits from ancestors while preserving the child's own activation choice. Disabling an object during Start suppresses its update in that dispatch.

Ant's colony controller exposes its checked colony object for scene authoring. ColonyBehaviour::OnDestroy clears colony markers and removes dependent ants, whether destruction comes from the controller or from a parent group. Parent-group activation suspends/resumes the colony spawning behaviour. Existing ants still use batch simulation systems and are not implicitly parented or paused by colony-group activation; batch activation policy is part of subsequent scheduler/component migration.

Engine tests cover parent enumeration, detachment, inherited activation, cycles, cross-scene rejection, child-before-parent teardown, and Start-time deactivation. Ant tests exercise a real colony under a parent group, spawning suspension/resumption, and cascading domain cleanup with unrelated-object preservation.

This is lifetime/activation hierarchy. Transform inheritance, persisted hierarchy, editor hierarchy integration, and common scheduler consolidation remain R1/R2 work; no completion claim for those features.

Bulk destruction initially increased the 100K stress run to 84.53 seconds. The pending-destruction list used linear membership checks and front erasure. It now uses a deque and a membership set so queue membership and front removal are constant time. The full rework suite is rerun after this correction; the earlier passing-but-slower run is not treated as the final performance result.

Final hierarchy verification: build passed and 61/61 tests passed in 57.66 seconds. AntStressRegression completed in 57.65 seconds after the queue correction, compared with 56.78 seconds in the preceding shared-ownership run and 84.53 seconds before this correction. These wall-clock measurements are regression observations, not a controlled benchmark claim. SailBoat remained excluded.

### Fixed-step ownership — implemented slice

AntSimulationWorld now dispatches its engine scene once at the beginning of the existing fixed tick. ColonyLifecycleSystem::Update only maintains colony counts/radii; it no longer advances every script in the scene. Standalone colony fixtures explicitly advance their scene before colony maintenance.

The backend-neutral FixedStepSequence validates the four externally scheduled phases without adding a second elapsed-time accumulator or replacing SystemRegistry. Both direct FixedUpdate and Workbench's registered phase callbacks use the same guarded operations. Repeated/skipped/reentrant phases and mismatched/nonfinite timesteps are rejected before their callback executes. A callback exception faults the sequence: resetting/recreating simulation state is required because partially mutated simulation state cannot be rolled back safely.

The phased path again records behavior and cleanup timings. Tests cover single dispatch through both entry points, colony-maintenance isolation, duplicate/skipped phases, inconsistent deltas and faulted-step rejection. Zero/nonpositive world-update calls retain their previous no-op semantics.

Remaining: consolidate the older Core/Scene API and runtime host lifecycle, remove the compatibility AntSimulationPipeline after its consumers migrate, and move domain state into registered ECS components. The existing SystemRegistry remains the registered-system scheduler; this phase guard is not a new system-registration framework.

Fixed-step ownership verification: build passed; 61/61 engine/editor/Ant tests passed in 57.35 seconds, including new phase-order and single-dispatch tests. SailBoat remained excluded. R1 remains in progress for the consolidation work above.

### First Ant component extraction — Energy

PipeFrame::Energy contains reusable float-precision current/capacity state and depletion/refill rules. Scene-instantiated AntEntity creates a separately queryable Energy component during its OnInstantiated hook. AntStore uses that scene path; movement consumes the ECS component directly, and compatibility Ant methods, checkpoints and existing inspector telemetry resolve that same state.

The old scalar Ant::energy is removed. ComponentValue<T> is an explicitly transitional adapter: isolated value-model tests retain a local value, binding transfers it into the scene and clears local storage, copying creates a detached snapshot, and moving preserves binding during dense ECS relocation. It is not the long-term authoring API; new systems should query components directly. Remove it as remaining Ant algorithms are converted to explicit component inputs. Low-level recipe instantiation against a bare World does not execute scene hooks; use BehaviourScene::Instantiate for complete scene objects.

Entity recipes now have a scene initialization hook, with scene-owned cleanup if that hook fails. Tests cover hook rollback, energy pool population, authoritative direct edits, detached copies, destruction and dense compaction. Existing Ant algorithms retain float energy arithmetic.

ComponentSchema now supports float members using numeric property storage, with range validation preventing overflow during conversion. EnergySchema binds current/capacity and round-trips them; its fields are read-only metadata. This does not yet register a live Energy editor attachment or replace the generic Inspector's scene-data bridge. Existing Ant telemetry reads the live ECS-backed value.

Remaining extraction slices: transform/motion, foraging, colony state, encounter state and procedural pose; complete registry/editor/serialization integration; removal of compatibility Ant and its binding adapter. R2/R3 are in progress, not complete.

Energy extraction verification: build passed; 61/61 tests passed in 61.88 seconds. Ant behavior parity, float-schema round-trip/overflow rejection, recipe-hook rollback and energy ownership/compaction checks passed. Stress wall time was 61.88 seconds versus 57.35 seconds in the preceding run; this is an observed regression signal, not a controlled attribution. Repeated compatibility-binding lookups remain a performance concern to reduce as remaining systems move to direct component queries. SailBoat remained excluded.

### Typed component access for batch systems

World::BorrowComponents<T>() resolves a component pool once and performs entity lookups directly within that pool. It rejects access after structural edits (creation, component changes, destruction, clear); ordinary field changes remain valid. It is a borrowed view: the world must outlive it and returned references. Acquire before parallel work, join workers before structural mutation, and reacquire at the next structural boundary.

Ant movement and behavior use this engine API for their direct energy checks, reducing compatibility-adapter/pool-resolution work. Remaining algorithms still use compatibility methods and are not fully migrated. Tests cover field writes, missing entities, structural invalidation and reacquisition; existing parallel parity and population tests validate the Ant usage.

Batch-access verification: build passed; 61/61 tests passed in 61.22 seconds. AntStressRegression took 61.21 seconds versus 61.88 seconds in the preceding run. This small wall-clock difference does not establish a controlled speedup, and performance has not demonstrably returned to the pre-energy-migration observation of 57.35 seconds. Continue removing compatibility access from remaining hot paths; do not mark the performance concern resolved. SailBoat remained excluded.

### Naming requirement clarified

Role-based names and correct folder ownership are mandatory completion gates in the governing plan. `Components/Ant.h` and `Components/Colony.h` remain unfinished aggregate models, not accepted final component designs. Decomposition must remove them, normalize component names and move registration/helpers to their proper roles. Generated code must use the same conventions. No source rename was performed in this clarification; current nonconformance remains visible and tracked.

### Ant encounter component — implemented storage slice

`AntEncounterComponent` now owns the enemy-alert timer and optional opponent
identity. These fields no longer live directly on `Ant`. Scene recipe creation
binds both Energy and encounter components; existing isolated Ant values retain
detached state through the temporary ComponentValue adapter.

Contact, behavior and movement decisions query the encounter pool through the
engine's borrowed component access. Existing worker behavior, marker intensity,
checkpoint hashing and compatibility methods resolve the same authoritative
component. Alert timing, opponent assignment and Pezzza's existing empty soldier
contact behavior are preserved. This does not introduce combat or change opponent
lifetime semantics.

Colony tests verify one encounter component per ant, direct ECS edits, detached
copies, compatibility writes, destruction and dense-storage relocation. Existing
behavior/contact/parallel parity checks remain the behavioral regression gates.
The Components README now explicitly describes the unfinished aggregates and
live Inspector limitations rather than presenting the old inheritance tree as
final architecture.

R3 remains incomplete: encounter timing/marker methods still run through the
compatibility model, other state remains monolithic, and live ECS Inspector
registration/editing is not provided by this extraction.

Encounter extraction verification: build passed and all 61 engine/editor/Ant
checks passed in 62.12 seconds. The previous suite took 61.22 seconds; these
wall-clock observations do not demonstrate a speedup or resolve the earlier
performance concern. SailBoat remained excluded.

### Foraging component — implemented storage and system-input slice

`ForagingComponent` now owns food-search mode, target and remaining distance,
last marker position, marker/walk timers, blocked status and food-delivery count.
These eight fields are removed from the Ant aggregate. `AntState.h` is removed;
the enum is now named `ForagingState` and colocated with its owning component.
Ant code, rendering and tests use the new name.

Scene recipes bind this component alongside Energy and AntEncounterComponent.
The behavior system queries it once and passes it explicitly to WorkerBehavior,
which updates food pickup/delivery state and counters directly. Movement applies
remaining target distance to the queried component. Existing marker/target
methods still bridge through the Ant compatibility model; this is not a claim
that all foraging logic or the aggregate has been removed.

Ownership tests cover direct live edits, detached copies, deletion and dense
relocation. The Inspector regression sets food count directly in ECS and checks
its existing telemetry. This verifies shared state, not automatic property
registration or editable live Inspector support, which remains R2 work.

Foraging extraction verification: build passed without reported warnings; all
61 engine/editor/Ant tests passed in 64.64 seconds, compared with the previous
62.12-second suite. This is another wall-clock regression observation, not a
controlled attribution; compatibility-path performance remains unresolved.
SailBoat remained excluded.

### R3 complete runtime migration — 2026-09-12

See [R3 runtime audit](R3_RUNTIME_AUDIT.md) for ownership, exact renamed/deleted
paths, performance observations and reference limits. Historical slice reports
above describe intermediate revisions; they are not the current architecture.

AntEntity/ColonyEntity now assemble engine-owned state. AntView/ColonyView borrow
it through ComponentView; AntQuery uses SceneViewCache and owns no world.
Ant/Colony aggregate files, AntStore, ComponentValue and AntSimulationPipeline
are removed. Movement/foraging/cleanup are held directly by the runtime and run
through its registered engine phases. ColonySpawnerBehaviour supplies attached
lifecycle. BodyStorage, DirectionTracker, SeekVelocity and shared spatial
components are consumed by Ant and independently exercised in engine fixtures.
AntGeometry produces neutral Vertex2D streams. Remaining legacy backend draw
hosts and UI are R5/R6 work.

The supplied Pezzza source was re-read. Existing update-order and travel-distance
calculation differences are documented in the audit; previous blanket parity
claims are not repeated. R3 preserves the current tested behavior. R0's full
reference audit and R2's editable live Inspector workflow remain incomplete.

Final R3 verification: build passed. The full final-revision run passed all 60
runtime/editor/rendering checks, including stress in 58.58 seconds. Dependency
lint correctly rejected the obsolete AntGeometry.h SFML allowlist entry; the
entry was removed, and its focused rerun passed. All 61 registered checks are
now passing. No broader retest was needed for that allowlist-only correction.
The geometry producer is now explicitly guarded by neutral-boundary lint.
See R3_ACCEPTANCE_RESULTS.txt for the full run and correction record.

## R4 — mounted UI runtime (2026-09-12)

R4 runtime gate verified. Final build passed and all 62 registered Ant-only rework checks passed.

The mounted path now uses the existing retained widget implementation and UI-manager frame/input loop. It owns nested stateful lifecycle, cached dirty builds, key reorder/replacement/removal, source and observer disposal, checked input lifetime, constrained text measurement, scroll retention/clipping and popup ownership. The neutral schema Inspector builds controls from component metadata and dispatches validated edits to a caller-owned command.

See [R4 authoring and runtime contract](R4_UI_RUNTIME.md), [acceptance log](R4_ACCEPTANCE_RESULTS.txt), and [saved Inspector fixture](evidence/r4-inspector.png). The fixture demonstrates reusable engine UI with a movement component; it is not the finished Ant editor.

R5 is next for full Ant/editor UI migration. R0–R2's outstanding gates and R6–R7 remain open; this entry does not retroactively mark them complete. SailBoat remains excluded.

## R2 completion — live component editing (2026-09-12)

The R2 gate is verified. Common and Ant typed component registrations now join actual ECS presence, property metadata, serialization and validated writes. Workbench discovers the selected colony's attached components and routes edits through its scene history. Property-only undo/redo preserves preview population/reserve, and saved authored values restore into actual components on reopen/reset.

The first colony no longer supplies other colonies' effective settings. Speed/radius/color are connected to domain behavior, and per-colony random streams use authored seeds on reset. Initial population and seed edits during preview affect the next reset. Telemetry stays outside scene persistence.

The real Inspector test also verifies exact large-integer input. Its labels now sit above inputs, and rows grow beyond the former fixed limit. Final build and all **64 registered checks passed** in Ant-only mode.

See [R2 implementation, authoring contract and boundaries](R2_COMPONENT_EDITING.md), [saved results](R2_ACCEPTANCE_RESULTS.txt), and [real Inspector render](evidence/r2-live-inspector.png).

R5 is next for full Ant/editor UI conversion. R0/R1 and R6/R7 remain open. Ant's transient individual-ant panel, full transform hierarchy/render integration, generated authoring workflow and remaining backend boundaries are not implied complete by the R2 colony gate. Earlier dated entries describe historical progress.

## Inspector scrolling and R5 scope clarification — September 12

Inspector refresh temporarily hid all rows, shrinking its content and clamping the scroll offset. It now restores the previous offset after final layout for the same selection, clamps after foldout changes, and starts at the top for a different selection. The shared engine ScrollPanel paints an overflow position indicator; scrolling uses the wheel/trackpad (the indicator is not a draggable scrollbar). This primitive is shared with declarative Scroll views.

The real Ant component editing acceptance test now exercises a 420×550 Inspector, wheel input over child content, ten live refreshes without jumping, scrolling to the bottom, resizing and changing selection. [Saved short Inspector evidence](evidence/r2-scrolled-inspector.png) shows the lower fields and clipped content beneath the fixed header.

R5 explicitly covers the editor toolbars, hierarchy, schema Inspector, asset browser, console, dialogs and dock content alongside Ant overlays. Both must use the same public View/StatefulView/MountedView authoring API, with source-linked learning examples. Missing reusable controls belong in the engine. This records the required migration; the current imperative editor has not yet been converted.

Validation: full Ant-only rework build succeeded; all 64/64 tests passed (61.95 seconds). [Test output](INSPECTOR_SCROLL_ACCEPTANCE_RESULTS.txt). The saved image is the real Inspector integration fixture, not a full running Workbench capture.

## R5 complete — shared Ant/editor declarative UI (2026-09-12)

Editor toolbar, hierarchy, component Inspector, assets, workspace tabs, project browser, diagnostics, context-menu content, floating headers and Zen controls now build shared public views. All six Ant panels and its timer use the same API. Shared Wrap, Card, Chart, Progress, Mesh and labeled fields replace project-owned widget construction and synchronization. Inspector transactions and scrolling remain covered by the actual component-editing acceptance test. Ant's selected component disclosure reads the live registry as read-only telemetry.

The build succeeded and all **64/64 Ant-only rework tests passed** in 60.68 seconds. Final Workbench acceptance was rerun after the tab-width readability adjustment and passed; Ant dashboard interaction/capture acceptance also passed. Reviewed captures cover narrow and normal windows, each Ant panel, editor tabs, metrics, floating/Zen modes and the real Ant runtime.

See the [implementation and public API guide](R5_UI_MIGRATION.md), [full test results](R5_ACCEPTANCE_RESULTS.txt), and [saved screenshot gallery](evidence/r5/README.md). Earlier dated entries describe intermediate revisions.

This completes R5's UI-content migration, not the entire rework. Native font/window/dock adapters and legacy host APIs still exist and belong to R6, along with the create/build/reload/attach workflow. R0 reference parity, R1 consolidation and R7 final cleanup remain open. SailBoat remains excluded.

## R5 performance correction — September 12, 2026

User-reported paused-editor scrolling and drag latency exposed a gap in the earlier functional acceptance. Shared runtime size-reset/layout churn and repeated text measurement have been corrected, and unchanged editor values now avoid control rebuilds. The reproducible paused refresh/update fixture improved from 801.63 ms median to 0.295 ms in Debug. This excludes rendering and is not an FPS claim.

The real Ant offscreen benchmark still measures about 20.3 ms paused and 47.3 ms playing one colony. Overall responsiveness/60 FPS acceptance remains open before R6. See [performance implementation, results and remaining work](UI_PERFORMANCE.md). R5's migration is implemented; its earlier completion must not be read as a performance guarantee.

## Component schema convention — September 12, 2026

Active Ant and shared registered components now own `Schema()` beside their data. Registration consumes schemas, generated components follow the same layout, and explicit Editable/ReadOnly bindings replace positional editability flags. Built-in validation is joined by atomic candidate-level Validate rules. [Convention, example and boundaries](COMPONENT_SCHEMA_CONVENTION.md).

## R6 — initial runtime/generation integration

Implemented a reusable SceneProjectRuntime and generated typed component/behaviour registration for new projects. The generated native plugin was compiled without SFML include paths, loaded and exercised through scene creation, lifecycle, inspection, reset and unload. R6 remains **in progress**: editor UI/build/reload/attachment, Ant extension integration, backend migration and the full SoldierAnt editor acceptance are unfinished. [Detailed status and boundaries](R6_AUTHORING_PROGRESS.md).

## R6 current implementation checkpoint

Ant and Colony now use the engine entity registry alongside Beacon; Food and Settings use generic component recipes. Shared engine services replace Ant's scene-diff/edit/property boilerplate, reducing AntSimulationRuntime.cpp from about 1,330 to 1,090 lines without additional Ant implementation files. The editor now provides native source generation and component/behaviour attachment controls. Generated entities register automatically, and Ant accepts generated registration. Full R6 remains open for integrated build/configure/reload recovery, remaining backend adapters and final performance/end-to-end acceptance. See R6_AUTHORING_PROGRESS.md for the current authoritative checklist.

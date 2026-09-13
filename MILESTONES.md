# PipeFrame Milestones

This file is the canonical roadmap for the simulation examples and the reusable
PipeFrame engine work extracted from them.

## Milestone 14 — Full Ant Simulation

Status: complete as a PipeFrame simulation baseline; Pezzza behavioral and visual
parity is reopened and tracked by Milestone 17.

Milestone 14 proves that PipeFrame can host a large, interactive ant simulation:

- editable colonies, food sources, walls, and simulation settings;
- multi-colony behavior and high ant counts;
- selection, inspection, metrics, and simulation controls;
- 1x, 2x, 4x, and maximum-speed execution;
- performance instrumentation and stress coverage;
- a single workbench view where editor and simulation tools remain available.

The Ant project is now an extraction target for Milestone 16. Its reusable HUD,
tool-panel, chart, input-routing, and performance patterns should move into the
engine instead of remaining Ant-specific code.

## Milestone 15 — Full SailBoat Simulation

Status: complete as a PipeFrame training baseline; Pezzza behavioral and visual
parity is reopened and tracked by Milestone 17.

Milestone 15 proves the second, substantially different PipeFrame use case:

- race start, finish line, ordered waypoints, wind, and training settings;
- sailboat motion and steering through a course;
- generation-based neural-network/evolution training;
- randomized first-generation population rather than a single cold-start boat;
- selected-boat, training-result, network, wind, and workflow diagnostics;
- save/load support for training state and history;
- Pezzza-inspired water, boat, wake, and race presentation;
- 1x, 2x, 4x, and maximum-speed execution.

The SailBoat project is also an extraction target for Milestone 16. Training
dashboards, charts, network visualization, drawers, overlays, and controls must
be reusable PipeFrame facilities rather than bespoke rendering functions.

## Milestone 16 — PipeFrame Extraction and UI Framework

Status: complete — 16A–16H accepted. Full Debug suite: 56/56 passing.
See [`docs/MILESTONE_16_ACCEPTANCE.md`](docs/MILESTONE_16_ACCEPTANCE.md)
for coverage, logic fixes, reviewed goldens, and stress measurements.

The detailed source and video comparison is recorded in
[`docs/MILESTONE_16_UI_EXTRACTION.md`](docs/MILESTONE_16_UI_EXTRACTION.md).

Goal: extract the generic systems proven by Milestones 14 and 15 into PipeFrame
itself. A new project should be able to compose a polished simulation editor
mostly from engine widgets and application state, without drawing panels with
raw SFML shapes or manually positioning every control.

“Flutter-like” means a concise compositional C++ API, predictable constraints,
reusable stateful widgets, and responsive layout. It does not mean copying
Flutter's implementation or forcing every frame to rebuild the entire tree.

The visual and interaction direction follows Pezzza: a clear playfield,
lightweight floating surfaces, edge drawers, rounded nested containers,
consistent spacing, restrained shadows, fast hover/press motion, tooltips, and
a minimal/Zen presentation mode.

### 16A — Extraction contract and baselines — COMPLETE

- Inventory all UI and runtime utilities in PipeFrame, Ant, and SailBoat.
- Classify each feature as engine-generic or project-domain-specific.
- Capture input, layout, visual, and performance baselines before migration.
- Define the dependency boundary: engine widgets consume plain view models and
  callbacks; they must not know about ants, boats, colonies, or neural genomes.

Acceptance: every duplicated system has an explicit destination, and migration
can proceed without moving domain logic into the engine.

### 16B — Constraint and layout core — COMPLETE

- Add fixed, fit-content, and stretch sizing policies.
- Add min/max constraints and intrinsic-size measurement.
- Implement deterministic measure-then-arrange layout propagation.
- Provide Row, Column, Stack/Overlay, Padding, Align, Flex, and Spacer building
  blocks.
- Preserve UI scale and DPI behavior across common viewport sizes.

Acceptance: representative Ant and SailBoat panels resize without per-screen
coordinate calculations.

### 16C — Compositional UI API and state — COMPLETE

- Replace callback-heavy construction with a concise nested C++ composition API.
- Support stable widget identity, keyed child reconciliation where needed, and
  targeted invalidation instead of full-tree churn.
- Add typed bindings for text, values, visibility, enabled state, selection, and
  callbacks.
- Separate persistent application state from transient visual state.

Acceptance: a panel can be expressed as a readable component tree, updated from
state, and reused unchanged by more than one example.

### 16D — Pezzza-style theme, surfaces, and motion — COMPLETE

- Centralize spacing, margins, corner radii, outlines, shadows, palette,
  typography, opacity, and motion-duration tokens.
- Add rounded cards, translucent/smoked-glass surfaces, separators, badges, and
  selected/focused treatments.
- Add real-time interpolation for hover, press, lift, fade, slide, and tooltip
  transitions so UI motion is independent of simulation speed.
- Keep visual effects restrained enough that the simulation remains legible.

Acceptance: controls share one theme and expose consistent normal, hovered,
pressed, focused, selected, disabled, and hidden states.

### 16E — Reusable controls and simulation surfaces — COMPLETE

- Edge Drawer and collapsible panel with animated handles.
- Overlay, popup, tooltip, toast, and modal layers with correct input capture.
- Button, segmented control, toggle, slider, numeric field, tabs, scroll view,
  and list/table primitives.
- Metric cards, time-series and bar charts, gauges, progress displays, and a
  generic node/network visualization surface.
- Standard simulation transport: play/pause, step, reset, 1x/2x/4x/max.
- Standard minimal/Zen mode that hides chrome without removing capabilities.

Acceptance: the common controls required by both simulations exist in the
engine and can be exercised in a standalone UI gallery.

Completed 2026-09-09. `examples/UIGallery` exercises every control family above,
including collapsible settings, four edge drawers, overlays, numeric entry,
lists/tables, monitoring, network inspection, working transport, and Zen recovery.
Input barriers isolate keyboard focus and pointer capture; Tab/Shift-Tab
traversal is available. All six 16E acceptance/regression targets pass, and
narrow/wide/scaled layouts were visually checked. See
[`docs/UI_GALLERY.md`](docs/UI_GALLERY.md) for the requirement mapping and checks.
The full debug build passes; the full suite is 48/50 with two documented SailBoat
failures remaining at the 16E checkpoint. Subsequent step statuses follow below.

### 16F — PipeFrame Workbench migration

- Rebuild the top bar, hierarchy, inspector, metrics, menus, and dialogs with the
  new composition and layout APIs.
- Keep editor tools available while a simulation runs; simulation mode must not
  hide capabilities required to create or inspect the scene.
- Formalize world-versus-UI event routing, including right-click selection,
  pointer capture, focus, overlays, and context menus.
- Add responsive layouts and Zen mode while preserving the existing dark
  PipeFrame identity under the Pezzza interaction language.

Acceptance: the Workbench contains no essential hand-positioned panel layout,
and editor interactions continue to work while the simulation is active.

Status: complete. The Workbench now composes its responsive shell, hierarchy,
inspector, diagnostics, browser, context menu, and Zen transport from shared
widgets. Authoring remains available during playback, and runtime synchronization
preserves play/pause state. UI barriers, right-click selection, drag capture,
focus cancellation, and Tab traversal are verified by `WorkbenchUIAcceptance`.
See [`docs/WORKBENCH_UI.md`](docs/WORKBENCH_UI.md) for controls, runtime rebuild
semantics, and reproduction commands. The full debug build passes and the suite
is 49/51 at the 16F checkpoint; the two previously recorded SailBoat failures
remain. Subsequent step statuses follow below.

### 16G — Ant and SailBoat migration

- Replace Ant's custom HUD/tool-panel renderers with PipeFrame components.
- Replace SailBoat's raw HUD panel/text functions with the same components.
- Share drawers, charts, metric cards, inspectors, transport controls, and
  training-history/model-selection UI.
- Leave ant behavior, sailing physics, training algorithms, and domain-specific
  visualization in their respective projects.
- Remove duplicated UI implementations after parity is verified.

Acceptance: both projects exercise the PipeFrame UI library directly, and a
shared control or style change appears in both without project-local rewrites.

Status: complete. Ant and SailBoat now compose the shared SimulationDashboard,
drawers, scrolling tabs, metrics, tables, charts, and controls. Domain callbacks,
history, model persistence, and inspectors remain project-owned. Workbench
transport is shared by both. The old Ant HUD/tool-panel implementations and
SailBoat raw HUD functions are removed. Live tool use, project keyboard focus,
world capture, checkpoint selection/save/load, and Zen recovery are verified.
See [`docs/SIMULATION_DASHBOARDS.md`](docs/SIMULATION_DASHBOARDS.md) for the
component mapping, input contract, and verification commands. The full build
passes. The 16G checkpoint was 50/52; 16H resolves both SailBoat failures and
adds four acceptance tests, bringing the full suite to 56/56.

### 16H — Hardening, documentation, and performance — COMPLETE

- Add unit tests for constraint resolution, layout invalidation, state binding,
  focus, hit testing, pointer capture, and animation clocks.
- Add screenshot/golden tests at multiple resolutions and UI scales.
- Add interaction tests for drawers, overlays, context menus, right-click world
  selection, keyboard focus, and transport controls.
- Profile layout, rendering, allocations, and event dispatch under Ant and
  SailBoat stress scenarios.
- Document the component API with a small third example that uses only public
  PipeFrame facilities.

Acceptance: Milestones 14 and 15 retain behavior and performance, common UI is
owned by the engine, and a new application can be assembled without copying
code from either example.

Status: complete. Unit/interaction hardening, three reviewed golden baselines,
real-population dashboard profiles, and the public-API-only ThermalLab example
are implemented. NEAT population seeding and foundation asset/property checks
are fixed; Debug/Release runtime outputs are isolated. Full Debug validation
passes all 56 tests. See the acceptance report and component API guide.

## Architectural evidence for Milestone 16

Before this extraction, the local source contained three UI approaches:

1. PipeFrame's retained widgets, `StackPanel`, `UIBuilder`, theme, and manager.
2. Ant-specific HUD renderers and editor tool panels.
3. SailBoat-specific HUD functions that directly draw SFML rectangles and text.

Pezzza's source provides a useful reference boundary:

- widgets own a retained parent/child tree and event state;
- a separate layout tree performs bottom-up measurement and top-down allocation;
- fixed, fit, and stretch policies replace repeated absolute positioning;
- drawers and layout primitives compose application-specific panels;
- centralized theme constants and interpolated values produce consistent visual
  states and motion.

PipeFrame adopted those general principles while preserving its own public
API, rendering backend boundaries, editor requirements, and testability.

## Milestone 17 — Engine Extraction and Pezzza Parity

**Progress:** 17A-17I complete; 17J remains.

Status: active. Milestone 16's green suite establishes a stable migration
baseline, not final feature or visual parity.

17E acceptance: complete with the full Debug suite at 61/61 passing. Resource,
render-surface, image, effect, and audio ownership now routes through PipeFrame
handles and services; visual parity remains scheduled for 17I.

17F acceptance: complete. Generic NEAT, evolution, experiment history,
checkpoints, background evaluation, live inference snapshots, and neutral
policy/environment/trainer contracts now live in `PipeFrame::Learning`.
SailBoat consumes them and the independent non-boat regression trains,
checkpoints, resumes deterministically, and exposes its network for inspection.
The final Pezzza source/video audit added live propagated edge signals, matched
the reference mutation candidate pools, and made SailBoat checkpoints restore
their best score and evolution settings. The full Debug suite passes 62/62
tests.

17G acceptance: complete. Ant behavior now has deterministic full-state
checkpoints, cumulative birth/death accounting, safe colony removal with ant,
physics-body, and marker cleanup, and an editor-exposed update-worker setting.
The scripted parity regression verifies Pezzza's marker takeover order and
one/four-worker equivalence through a 120-tick four-colony run and same-seed
replay. Existing encounter, behavior, editor, render-option, 10k throughput,
and 100k capacity checks remain green. Pezzza visual composition remains 17I.
The full Debug suite passes 63/63 tests.

17H acceptance: complete. SailBoat now follows Pezzza's mark-endpoint targeting
and neutral generation-zero population exactly. Source-formula fixtures cover
movement, progress scoring, ordered completion, exploration reseeding,
generalization across courses and wind, async evaluation, periodic saves, and
deterministic checkpoint continuation. Training time remains visible in a
persistent top metric, and the live network has an independent right-edge
drawer. The integrated playback/editor/checkpoint regression and 10,000-boat
stress gate pass. Final styling and comparison-board approval remain 17I. The
full Debug suite passes 63/63 tests.

17I acceptance: complete. Ant and SailBoat now use compact white edge buttons
instead of the shared full-height tab surface. Activating a button replaces the
button layer with one complete popup and a CLOSE action; opening another popup
closes the first. The shared drawer supports authored minimum content widths,
side anchoring, overlap-free motion, and per-application glass themes. Ant exposes
separate timer, editor, settings, colony, selected-ant, profiler, and transport
surfaces; SailBoat exposes wind, iteration, editor, settings, training result,
selected boat, history, models, live network, and transport surfaces. The live
network adds labels, values, signed colors, and weighted edges. Interaction and
resize assertions pass from 640x720 through 2560x1440, the retained comparison
board covers collapsed, 720p/1080p/1440p/narrow states, and the full Debug suite passes
63/63 tests.

Goal: extract reusable simulation, spatial, physics, learning, resource,
rendering, audio, timing, and task infrastructure into PipeFrame; remove SFML
from example-facing contracts; and reproduce the Ant and SailBoat reference
behavior, interaction, and presentation with evidence.

The detailed plan, source/video findings, dependency target, phases 17A–17J,
and completion gates are recorded in
[`docs/MILESTONE_17_PLAN.md`](docs/MILESTONE_17_PLAN.md).

Acceptance: every reference feature is matched or an intentional difference is
explicitly approved; example domain/UI code contains no direct SFML includes or
links; generic systems have a non-originating consumer; complete Debug and
Release suites pass; deterministic logic, interaction recordings, reviewed
visual comparison boards, and stress benchmarks demonstrate parity.

## Milestone 18 — Production Editor Foundation

Status: active; 18A-18E are complete, while audited work in 18F-18H still blocks
Milestone 18 completion. Adaptive and fixed world-space grids, rulers,
measurements, independent snapping, direct move/rotate/non-uniform-scale gizmos,
pivots, selection, placement, registered previews, framing, bookmarks, and
separate editor/simulation cameras are implemented and covered by Workbench
acceptance tests and stored multi-resolution screenshots. The 18B version-6 component scene model,
hierarchy, stable references and connections, units, filtering, transactional
commands, templates, and additive scene workspace are implemented and covered.
The metadata-generated Inspector, typed property validation, live telemetry,
multi-edit transactions, copy/paste/defaults, foldouts, and safe runtime edit
commands are complete in 18C. Layout and extension integration continues in
18F-18G.
The persistent asset database, built-in import pipeline, deterministic cache,
generic part metadata, Asset Browser, transactional assignment, and missing
reference repair are complete in 18D.
Persistent prefabs, nested instance identity, variants, overrides,
apply/revert/unpack, and reusable multi-part assemblies are complete in 18E.
The 18F workspace adds resizable side and bottom docks, saved/reset layouts,
monitor-safe floating panel state, compact modal tools, and Console, Profiler,
Learning, Game, Connections, and Telemetry tabs. Its implementation record is
[`docs/milestone18/18F_DOCKING_AND_PROFESSIONAL_WORKSPACE.md`](docs/milestone18/18F_DOCKING_AND_PROFESSIONAL_WORKSPACE.md).
The 18F checkpoint passes the complete Debug build and all 68 registered tests.
The 18G plugin API adds all editor/domain extension families, searchable
contextual actions, registered lifecycle systems, service contexts, validation,
exception isolation, and state-preserving hot reload. Basic, Ant, and SailBoat
now declare stable plugin identities and run through the shared scheduler and
conformance contract. The implementation record is
[`docs/milestone18/18G_EXTENSIBLE_TOOLS_ACTIONS_AND_PLUGINS.md`](docs/milestone18/18G_EXTENSIBLE_TOOLS_ACTIONS_AND_PLUGINS.md).
The 18G checkpoint passes the complete Debug build and all 70 registered tests.

Goal: turn SimulationWorkbench into a reusable editor with an adaptive grid,
snapping and transform gizmos; component hierarchy; metadata-driven inspector;
asset database, browser, and importers; prefabs; docking and saved layouts;
project-extensible tools; multiple scenes; and build/package workflows. The
editor must also support typed attachment points, mechanical, power, and signal
connections, project units, modular assemblies, environment authoring, and live
telemetry so future projects can assemble machines without special-case
Workbench code. Its interaction model and workspace quality target the familiar
structure of Unity and Unreal editors: a central viewport, hierarchy, inspector,
assets, dockable tools, contextual actions, clear simulation state, diagnostics,
saved layouts, and consistent keyboard and drag/drop workflows.

Milestone 18 also standardizes generated project structure, typed property
exposure with optional Unreal-style convenience macros, registered component
systems, lifecycle phases, service contexts, dependency validation, and project
conformance checks. These requirements are recorded in
[`docs/milestone18/PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md`](docs/milestone18/PROJECT_STRUCTURE_AND_AUTHORING_CONTRACT.md).
BasicSimulation, AntSimulation, and SailBoatSimulation must migrate to this same
structure and PipeFrame-provided component/system lifecycle while retaining all
Milestone 17 behavior, visual, interaction, deterministic, and performance gates.

The durable scope and phased acceptance plan are recorded in
[`docs/PIPEFRAME_REUSABLE_SYSTEMS_INVENTORY.md`](docs/PIPEFRAME_REUSABLE_SYSTEMS_INVENTORY.md)
and [`docs/MILESTONE_18_PLAN.md`](docs/MILESTONE_18_PLAN.md).

Acceptance: the generic readiness fixture assembles connected parts, authors an
obstacle environment, previews placement and sensor ranges, preserves stable
connections through save/load and undo/redo, and displays mock telemetry through
plugin extension points. A reviewed UI acceptance matrix proves that primary
workflows remain readable, reachable, consistent, and overlap-free at supported
resolutions and DPI settings. Milestone 18 contains no robot simulation policy.
Milestone 19 is the external-user dogfood gate: each robotics feature starts
through Workbench and public PipeFrame APIs. Engine/editor changes are expected
when this reveals bugs, missing features, awkward APIs, broken controls, or poor
design. Those discoveries reopen the relevant Milestone 18 gate and require a
shared fix, regression coverage, and verification in Basic, Ant, and SailBoat;
project-specific framework workarounds are not accepted. Create Object Type must
support data-only archetypes such as Soldier Ant through component composition.
The component and system generators must handle files, registration, build
integration, validation, and hot reload when new behavior is required.

## Milestone 19 — Modular Robotics Simulation

Status: planned; begins after Milestone 18 completes its robot-readiness gate.

Goal: deliver the third PipeFrame reference project as a brand-independent
modular robotics simulator. Users assemble chassis, wheels, motors, controllers,
batteries, ultrasonic sensors, LiDAR, infrared sensors, servos, LEDs, and other
compatible parts; connect power and signals; write control logic; construct
environments; and test autonomous behavior. An ELEGOO UNO R3 Smart Robot Car
style build is one reference configuration rather than a fixed product model.

The phased plan and completion gates are recorded in
[`docs/MILESTONE_19_PLAN.md`](docs/MILESTONE_19_PLAN.md).

Acceptance: an ELEGOO-style differential-drive robot and a materially different
custom robot can both be assembled, programmed, simulated, inspected, saved,
replayed, and validated without modifying Workbench or exposing SFML through
project-facing contracts.

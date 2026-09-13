# Milestone 16 UI Extraction Audit

This document records the UI-specific evidence behind Milestone 16. It compares
the supplied Pezzza videos, the three local Pezzza source trees, PipeFrame's
current UI library, and the two PipeFrame simulation examples.

## What Milestone 16 means

Milestone 16 is not another simulation. It turns the common infrastructure
proven by the Ant and SailBoat milestones into public PipeFrame facilities so a
third user can build a different project without copying either example.

PipeFrame owns reusable composition, layout, surfaces, controls, input routing,
motion, data visualization, and editor-shell behavior. An example project owns
its world model, simulation rules, domain rendering, and domain-specific
commands.

## What each Pezzza project contributes

### SailBoatPezza: the simulation-shell vocabulary

`SailBoatPezza/src/ui/ui.hpp` builds a retained widget root and surrounds a
large playfield with individually collapsible edge drawers:

- Settings and Editor drawers on the left;
- Training Timer on the upper-left edge;
- Training Result, Selected Boat, and Network drawers on the right;
- Simulation Control centered on the bottom edge;
- a compact Wind Direction display in the corner;
- background blur behind translucent panels;
- UI visibility independent from world visibility;
- world editing only when the UI did not consume the pointer event.

The older SailBoat UI still manually computes drawer positions. Its value for
PipeFrame is the shell and component vocabulary, not its layout implementation.

### AntPezzaSource: the reusable layout and input architecture

The newer Ant source replaces much of that manual placement with a separate
layout tree:

- `Size::Policy` supports `Fixed`, `Fit`, and `Stretch`;
- fit sizes are measured bottom-up from children;
- slots allocate and align children top-down;
- dirty layout state propagates to parents;
- horizontal, vertical, simple, custom, and drawer layouts compose together;
- widgets retain children, transforms, hover state, overflow hit testing, and
  pointer capture;
- left, right, top, and bottom drawers animate in real time;
- the root UI scales from a 1440p design coordinate system;
- editor, settings, timer, controls, colony, and selected-ant surfaces all use
  the same infrastructure.

This is the strongest architectural starting point for PipeFrame's layout
engine, but its shared-pointer ownership and global application dependencies
should not be copied blindly.

### TimeTrackerPezza: interaction and visual-system polish

TimeTracker supplies the reusable visual behavior that is less visible in the
simulation source:

- centralized margin, spacing, radii, shadow, outline, opacity, and type sizes;
- rounded filled and outlined card renderers;
- nested corner radii derived from the parent radius and inset;
- hover lift/scale with the shadow offset changing consistently;
- selected cards that shrink, gain an outline, and expose an active pill;
- animated toggles and radio buttons;
- bar/timeline visualization with hover inspection;
- transient tooltips rendered over a blurred copy of the scene;
- resolution-relative UI scaling;
- application colors and activities supplied from configuration.

This source also shows a limitation that PipeFrame should improve: the main
TimeTracker screen still computes several positions manually. PipeFrame should
retain its visual behavior while expressing it through the new constraints.

## Video interaction inventory

The supplied videos establish these user-visible requirements:

| Reference | UI behavior to preserve |
| --- | --- |
| Simulator UI rewrite | Constraint-driven responsive layout, fast prototyping, cohesive colors, and a Zen mode that minimizes chrome. |
| AI Gladiator | Live training monitoring with score bars, trend graphs, event/status information, CPU and memory meters, and monitoring that does not interrupt training. |
| Ant Simulator | Edge drawers, bottom simulation controls, contextual colony/ant information, clear multi-colony colors, and a mostly unobstructed playfield. |
| SailBoat | Wind indicator, iteration timer, training results, selected-agent details, neural-network visualization, editor/settings drawers, and a compact play/pause/full-speed controller. |
| UI construction example | Shared visual constants, rounded nested cards, hover lift and shadow response, animated selection, segmented surfaces, tooltips, restrained transparency, and configuration-driven colors/content. |

These interactions are more important than reproducing one screenshot or one
palette. PipeFrame can keep its product identity while adopting the same visual
grammar and disclosure model.

## Extraction matrix

| Common capability | Pezzza evidence | PipeFrame destination | Must remain project-owned |
| --- | --- | --- | --- |
| Retained widget tree | All three projects | `PipeFrame::UI::Widget` lifecycle and ownership | Domain object lifetime |
| Constraints and measurement | Ant layout tree | `Constraints`, `SizePolicy`, measure/arrange passes | World coordinates |
| Composition | Nested Pezzza widgets/panels | Row, Column, Stack, Overlay, Align, Padding, Flex, Spacer | Domain panel contents |
| Input dispatch | Widget active child and capture | Hover, focus, capture, bubbling/consumption, context actions | Meaning of world clicks |
| Responsive shell | Ant layouts and edge drawers | Anchored overlays and breakpoint-aware workbench shell | Simulation camera behavior |
| Cards and visual surfaces | TimeTracker card shaders | Rounded Surface/Card, outline, shadow, translucency, blur | Domain artwork |
| Design tokens | Pezzza theme/configuration headers | Theme roles for spacing, radius, color, opacity, type, motion | Per-project accent palette |
| Motion | Interpolated values in controls/drawers | Tween/easing service using real UI time | Physics interpolation |
| Disclosure | SailBoat/Ant drawers and Zen mode | Drawer, collapsible panel, modal, popup, tooltip, Zen shell | Which domain tools appear |
| Basic controls | Buttons, toggles, radios, sliders | Button, segmented button, toggle, radio group, slider, fields | Domain command handlers |
| Data display | Gauges, line/bar charts, labels | Metric card, gauge, progress, line/bar chart, table/list | Metric calculation |
| Network view | SailBoat network panel | Generic graph/node visualization | Genome/network semantics |
| Simulation transport | Bottom controls | Play/pause, step, reset, 1x/2x/4x/max component | Simulation controller implementation |
| Editor feedback | Preview markers and cancelable actions | Tool state, contextual prompt, cancellable action pattern | Actual placement/edit command |
| Monitoring | AI and Ant diagnostics | Performance/telemetry view models and reusable charts | Simulation-specific profiler counters |
| Persistence UI | SailBoat race/training actions | File/action dialogs and list/browser surfaces | Race/model serialization formats |

## Original PipeFrame gap (before extraction)

PipeFrame already has a retained `Widget`, `UIManager`, `StackPanel`, basic
controls, text fields, scrolling, and a theme. It does not yet provide Pezzza's
full measurement model, surface rendering, standard motion/state system,
drawers, overlay layers, charts, tooltips, or concise component composition.

The examples therefore bypass the engine:

- Ant owns custom editor-tool, colony, selected-ant, timer, and profiler HUD
  renderers with their own SFML shapes and hit testing.
- SailBoat's runtime draws every training HUD card and label directly using
  `sf::RectangleShape` and `sf::Text`.
- The Workbench uses PipeFrame widgets, but much of its construction and sizing
  remains imperative and panel-specific.

That duplication is the concrete reason for Milestone 16.

## Engine boundary

Move into PipeFrame:

- layout constraints and compositional containers;
- visual themes, surfaces, animation, and control states;
- drawers, overlays, tooltips, charts, gauges, lists, and generic graph views;
- UI/world input arbitration and pointer capture;
- standard editor chrome and simulation transport;
- view-model/binding interfaces for metrics and state.

Keep outside PipeFrame:

- ant steering, pheromones, colonies, food, and ant rendering;
- boat physics, wind calculations, course scoring, water, wake, and boat
  rendering;
- NEAT mutation/evolution and training policy;
- project-specific inspectors and commands, except as compositions of public
  PipeFrame widgets;
- project save formats and assets.

## Milestone 16 success test

Milestone 16 is successful when a small third example can:

1. define its UI as a readable component tree;
2. adapt from a small window to 4K without hand-positioning each child;
3. use the same drawers, cards, controls, tooltips, charts, and transport as the
   Ant and SailBoat examples;
4. keep world input correct underneath overlays and context menus;
5. select a project theme without rewriting component rendering;
6. contain no copied Ant or SailBoat HUD implementation.

## 16A baseline record

Captured on 2026-09-07 before the constraint-layout implementation:

- the existing PipeFrame UI regression suite passes when compiled against the
  current engine library;
- transparent overlay space passes pointer input back to the world;
- interactive children consume pointer input and retain capture through an
  outside drag/release;
- the last root is the topmost popup/input layer and blocks roots below it;
- clicking a focusable overlay routes text input to it, while window focus loss
  returns keyboard input to the application;
- a 2,000-child `StackPanel` relayout completed in approximately 5 ms on the
  development machine. This is a comparison baseline, not a portable pass/fail
  threshold.

The corresponding executable checks live in
`engine/tests/UIFrameworkTests.cpp` and must remain green through Milestone 16.

The existing debug example binaries are not a clean full-suite baseline: the
Ant aggregate test, SailBoat foundation test, and SailBoat population-trainer
test currently terminate with signal 11, while the SailBoat movement, NEAT,
project-integration, and race-task binaries pass. These failures predate the
Milestone 16 layout changes and must be separated from UI migration regressions
before the Ant/SailBoat migration step.

## 16B layout contract

Completed on 2026-09-07. PipeFrame now exposes a deterministic two-phase
`Measure`/`Arrange` contract with fixed, fit-content, and stretch policies,
minimum/maximum constraints, and requested-size preservation during parent
arrangement. `Row`, `Column`, `StackPanel`, `OverlayPanel`, `PaddingPanel`,
per-child alignment, flex factors, and `Spacer` form the first reusable
composition layer.

Layout values remain logical UI units. The workbench's existing UI-scale/DPI
transform continues to sit outside this layout pass, so a scale change does not
alter constraint semantics or require project-specific coordinates. Regression
tests cover nested measurement, implicit and explicit stretch, constraints,
overlay alignment, and a 2,000-child relayout baseline.

## 16C composition/state contract

The first 16C slice adds stable string keys to widgets, keyed child reuse, and a
nested `Component`/`Compose` API for readable reusable trees. Recomposition
updates existing widgets rather than silently duplicating them. Typed `State<T>`
values notify only their subscribers, and widget-owned subscriptions detach
automatically with widget lifetime. Standard bindings cover text, numeric value,
visibility, enabled state, and selection; `BindProperty` remains the typed
extension point for project-specific controls.

State values are explicitly tagged as application or visual lifetime. This
keeps saveable domain/application data distinct from hover, expansion, focus,
and animation state without teaching the engine about ants, boats, or training
models. Conditional composition now hides omitted keyed children, restores them
with their prior identity, and releases stale pointer capture and keyboard focus.

The simulation transport is the first extracted production component. The
Workbench inspector now consumes the engine-owned play/pause, step, reset, and
1X/2X/4X/MAX panel, so Basic, Ant, and SailBoat projects receive the same control
without copying callbacks or selected-state rendering into each project.

## 16D visual-system progress

The Pezzza-inspired layer now starts from centralized roles instead of
screen-specific colors: small/medium/large/pill radii, restrained shadow roles,
base/elevated/floating/glass surfaces, scrim opacity, and fast/normal/slow motion
durations. `Panel` supports rounded geometry and optional offset shadows, while
`Surface` and `Card` apply semantic combinations of those tokens.

`UIManager::Update` advances widget motion from real frame time, independently
of the fixed-step simulation multiplier. Buttons interpolate between normal,
hovered, pressed, selected, and disabled colors with an ease-out curve and offer
a reduced-motion mode that snaps feedback immediately. These defaults now apply
to the shared simulation transport without adding more playfield chrome.

The base `Widget` now supplies reusable opacity and visual-offset animation, so
fade, slide, and lift preserve layout coordinates and propagate to nested
content. Hovered buttons use a restrained one-pixel lift. The engine-owned
`Tooltip` adds delayed, viewport-clamped show behavior, a short fade/slide exit,
and transparent hit testing so it cannot steal input from the playfield.
Reduced-motion mode snaps all of these transitions. Regression coverage verifies
real-time interpolation, inherited opacity, nested movement, selected/focused
input states, and a 2,000-widget layout baseline.

## 16E reusable-surface progress

The entries below are the historical extraction sequence. 16E is now complete;
the final acceptance record appears at the end of this document.

The first interaction layer is now engine-owned. `EdgeDrawer` supports all four
viewport edges, retains a narrow interactive handle while closed, slides using
real UI time, and snaps under reduced motion. Its content remains ordinary child
widgets, so applications can compose project-specific tools without copying the
drawer lifecycle.

`ModalBarrier` supplies an explicit scrim and background-input boundary. It
captures pointer movement, scrolling, presses, and releases before they reach
the simulation, optionally dismisses on a background click, and relinquishes
input immediately when hidden. Regression tests cover both the drawer handle
path and modal-to-world input restoration.

The shared control set now also includes an icon/label-agnostic `Toggle`, a
stepped or continuous `Slider`, and a flex-layout `SegmentedControl`. Each
supports pointer and keyboard operation, explicit selected/focused/disabled
feedback, value-change callbacks, and typed state bindings. The controls avoid
embedding project labels so Ant, SailBoat, Basic, and future applications can
provide their own text or icon children while sharing behavior.

Popup and toast presentation are now reusable engine surfaces. `PopupLayer`
keeps floating content inside the viewport, owns pointer input while open, and
returns input to the world only after dismissal. `Toast` is intentionally
hit-test transparent, advances its lifetime using real UI time, and supports
the same reduced-motion contract as drawers and tooltips.

`ZenModeShell` separates the playfield, optional chrome, and essential HUD into
retained layers. Entering Zen mode hides only optional chrome; simulation
transport or other recovery controls placed in the essential layer remain
available. `TabView` and `ListView` build on the shared selection model, retain
their page/row widgets, support pointer and keyboard navigation, and accept
typed selection bindings without embedding project-specific labels.

The first monitoring components are also engine-owned. `MetricCard` provides a
compact title/value/detail hierarchy with a semantic accent; `ProgressBar` and
`Gauge` expose clamped values, configurable visual roles, real-time value
interpolation, reduced-motion snapping, and typed numeric bindings. They are
display primitives only—the source and meaning of FPS, score, population,
training completion, or memory values remain application-owned.

`PopupLayer` now provides a viewport-clamped floating content surface over a
full input-owning scrim. Background dismissal is configurable, inside clicks do
not close the popup, and the world regains input only after the close transition
finishes. `Toast` is the passive counterpart: it uses real UI time for its
lifetime and fade/slide motion, stays hit-test transparent, and snaps cleanly in
reduced-motion mode. Together with `ModalBarrier` and `Tooltip`, these establish
the engine's initial overlay-layer contract without placing project HUDs over
the center of the playfield.

### 2026-09-09 — Charts and the first gallery slice

`TimeSeriesChart` and `BarChart` now share an engine-owned passive plotting
surface. Applications supply colored sample arrays; history retention, sampling,
metric calculation, labels, and domain semantics stay outside the renderer.
Automatic bounds handle empty and constant data, ignore non-finite samples, and
include the zero baseline and category widths for bars. Explicit ranges clip
geometry to the plot; missing samples break line segments rather than drawing
across gaps. Geometry rebuilds on data, layout, or opacity changes, and rendering
uses cached vertices.

`examples/UIGallery` is the first standalone gallery slice using only public
engine UI APIs. It demonstrates two-series history, signed category bars, a metric
card, gauge, progress display, slider, and animation toggle. Plot containers
switch between horizontal and vertical layout at a viewport breakpoint.
Deterministic screenshot export supports visual checks without running a
simulation. This is not yet the complete 16E control gallery.

The new `ChartRegression` checks range handling, clipped line slopes, missing
samples, extreme finite data, signed bars, inherited opacity/position, zero-size
geometry, and world-input pass-through. Its offscreen rendering requires a
working graphics context (macOS sandbox runs may fail without graphics access).
The API and gallery commands are documented in [`UI_CHARTS.md`](UI_CHARTS.md).

### 2026-09-09 — Generic network visualization

`NetworkView` adds a domain-independent node/edge surface with stable IDs,
application-supplied normalized layout, colored directed/undirected connections,
selection highlighting, and a node-inspection query. It validates graph
replacement before changing state, retains selection by ID, follows parent
motion/opacity, and does not capture world input. Geometry is cached until its
inputs change. Self loops and automatic layout are outside this initial slice.

The standalone gallery now uses engine tabs to separate charts and a network
example with application-owned node selection. Network screenshots were checked
at 1100×800 and 640×800. `NetworkViewRegression` adds offscreen rendering and
inspection/validation coverage. The public contract is documented in
[`UI_NETWORK_VIEW.md`](UI_NETWORK_VIEW.md). Table primitives and the remaining
control-gallery coverage are still outstanding for 16E; simulation migration
has not been performed in this slice.

### 2026-09-09 — Table primitive

`TableView` adds read-only tabular data with weighted columns, a fixed header,
vertical scrolling, UTF-8 truncation, focus feedback, and single-row selection.
Stable row IDs preserve selection across reordered snapshots. Invalid data is
rejected before replacement. Only visible rows are prepared for rendering;
cell/body scissors support normal and scaled axis-aligned views.

The gallery's third tab demonstrates 40 synthetic rows and an application-owned
selection readout. `TableViewRegression` covers pointer/capture cancellation,
keyboard navigation, scrolling, stable IDs, invalid input, clipping, and hidden,
disabled, and pre-layout states. See [`UI_TABLE_VIEW.md`](UI_TABLE_VIEW.md).
Broader control-gallery coverage and final acceptance checks remain before 16E
can be marked complete. Workbench and simulation migrations remain separate.

### 2026-09-09 — 16E completion

The missing `CollapsiblePanel` and complete Controls gallery are implemented.
Popup/modal barriers now constrain keyboard focus and revoke outside capture
before the next event, with Tab/Shift-Tab traversal and Escape dismissal. Modal
inside clicks no longer dismiss the scrim, and ScrollPanel clipping now follows
UI scaling. All required control families are exercised in the standalone
gallery, including real transport callbacks and recoverable Zen mode.

All six relevant acceptance/regression targets pass, and the full debug build
passes after repairing the AntFoodTests source dependency. Visual checks cover
narrow, wide, and 1.5× layouts, overlays, drawer clipping, and Zen recovery.
The full debug suite is 48/50: SailBoatFoundationRegression has a visual-control
count failure and asset-path diagnostics; SailBoatPerformanceRegression cannot
seed the 10,000-boat population. These remain outside the completed 16E scope.
See [`UI_GALLERY.md`](UI_GALLERY.md) for the requirement-to-widget mapping and
reproduction commands. Milestone 16 remains open for 16F, 16G, and 16H.

### 2026-09-09 — 16F completion

Workbench panels now use the shared composition/layout framework: responsive
rows and columns, a TableView hierarchy, scrolling inspector and project
browser, retained diagnostics with MetricCard, PopupLayer context actions, and
ZenModeShell recovery with SimulationTransport. Simulation mode retains editor
tools. Live edits synchronize authored state while preserving playback or pause;
runtime implementations may rebuild domain state under the existing contract.

Right-click selection, modal routing, world drag capture across panels, focus-loss
cancellation, Tab traversal, and Zen recovery are covered by the new isolated
Workbench acceptance test. Native file/directory dialogs remain platform UI.
Visual checks cover desktop/narrow layouts, metrics, browser, menu, and Zen.
The full debug build passes. The suite is 49/51 with the same two SailBoat
failures recorded for 16E; all Workbench and UI regressions pass. See
[`WORKBENCH_UI.md`](WORKBENCH_UI.md). Milestone 16 remains open for 16G and 16H.

### 2026-09-09 — 16G completion

Ant and SailBoat now use the same public SimulationDashboard drawer and scrolling
tabs. Shared controls render their tool actions, metrics, inspectors, histories,
network, and checkpoint selector. Ant's bespoke HUD/tool-panel implementations
and SailBoat's raw HUD drawing functions were removed. Domain data, commands,
history sampling, algorithms, and world/preview rendering remain in the projects.

Project UI focus and world pointer capture are explicit runtime contracts.
Workbench preserves project right-click tools and captured brush strokes across
its panels. Ant painting and SailBoat race placement/previews remain usable
during playback. The shared Workbench SimulationTransport still drives both.

The full debug build passes; the full suite is 50/52. All migration, Workbench,
UI, and Ant tests pass. The two SailBoat foundation/10,000-boat seeding failures
recorded before migration remain isolated. Visual checks cover narrow/desktop
tools, colony history, selected-ant preview, training, generation history, and
checkpoint selection. See [`SIMULATION_DASHBOARDS.md`](SIMULATION_DASHBOARDS.md)
for the mapping and reproduction commands. Only 16H remains in Milestone 16.

### 2026-09-09 — 16H completion

Milestone 16 is complete. The full Debug build and all 56 tests pass, including
the two previously failing SailBoat regressions. Hardening adds invalidation,
binding teardown, animation-clock and hidden-focus checks, reviewed screenshot
baselines at three size/scale combinations, real-population dashboard profiling,
and the standalone public-API-only ThermalLab application.

Logic fixes guarantee legal-edge fallback during NEAT seeding, correct foundation
asset/property validation, and configuration-specific runtime library resolution.
Measured layout costs prompted a wrapped-label cache with pixel regression
coverage. See [acceptance evidence](MILESTONE_16_ACCEPTANCE.md) and the
[component API guide](UI_COMPONENT_API.md) for measurements and reproduction.

# Ant and SailBoat dashboards (16G)

Both examples compose the public `SimulationDashboard` with PipeFrame controls.
The shared drawer owns viewport clipping, scrolling tabs, layout, focus, and
real-time motion. Its UI handle remains available when closed; closing hides
page controls from focus traversal. Zen hides the dashboard and releases input.
Changing shared controls or the engine theme affects both projects.

| Surface | Ant | SailBoat | Shared components |
| --- | --- | --- | --- |
| Tools | Select, erase, food, wall, radius, render options | Waypoints, start/finish placement, race save/load, follow/world options | TextButton, Slider, scrolling Column |
| Metrics | Timer, colony food/reserve, ant state, profiler | Wind, training time/results, best boat, timings | MetricCard, Label |
| Inspection | Colony selection, ant follow/highlight/target, energy/speed/blocked | Best-boat details and network | TableView, ProgressBar, NetworkView |
| History | Population and collection rate | Best/average scores, completion, generation details | TimeSeriesChart, TableView |
| Persistence | Existing authored scene persistence | Saved-run selection, checkpoint save/load, artifact export | TableView, TextButton, wrapped status labels |
| Transport | Workbench inspector and Zen recovery | Workbench inspector and Zen recovery | SimulationTransport and the host SimulationSession |

Ant history sampling, inspector data, brush behavior, and rendering remain in
Ant. SailBoat race commands, persistence, history, genome interpretation,
physics, and training remain in SailBoat. The selected-ant preview and wind
direction geometry remain project-specific widgets. Generic HUD panel, text,
chart, button, slider, and hit-test implementations were removed from the
examples after the migration checks.

## Behavior and routing

Brushes and race tools remain usable during playback in both Editor and
Simulation modes. Ant's old playback brush lockout was removed. Right-click
selects a live ant when SELECT is active; other brush modes use right-drag.
SailBoat reserves right-click while a race tool is armed. Cancel Tool or Escape
returns it to Workbench context-menu handling. Race-placement previews also
remain visible during playback.

The runtime interface now distinguishes UI events from world events:

- `HandleUIEvent` returns whether shared controls consumed an event.
- `HasUIFocus` gives a project's focused controls keyboard input before
  Workbench shortcuts, including Tab traversal.
- `ConsumesPointerAt` describes the visible dashboard footprint.
- `UsesRightClickTool` reserves domain right-click actions.
- `HasWorldPointerCapture` keeps a brush stroke routed to its project even
  when the pointer crosses Workbench panels. Focus loss cancels the stroke.

Workbench overlays preempt project controls and world capture. Clicking
Workbench controls clears project UI focus. Existing scene synchronization and
undo remain host-owned; synchronization may rebuild runtime domain state while
the host preserves play/pause, as documented for 16F. Runtime plug-ins must be
rebuilt against the updated interface; the repository build does this for all
three examples.

## Verification

The full debug build passes. After 16H hardening, the complete suite is
**56/56 passing**:

- `SimulationDashboardAcceptance` checks shared control callbacks, live Ant
  painting and ant selection/follow, live race placement, model-table clicks
  across rendered frames, checkpoint save/load, drawer recovery, focus-loss
  cancellation, Zen input release, and 640/1000/1600-pixel layouts. Save actions
  use an isolated temporary project.
- `WorkbenchUIAcceptance` checks project right-click ownership, drag capture
  across Workbench panels, and project keyboard focus in addition to 16F checks.
- `TableViewRegression` verifies that live cell updates preserve row clicks.
- Existing UI, Ant behavior/rendering/stress, SailBoat movement/race/NEAT,
  trainer, persistence, and project tests retain their previous results.

The two failures recorded at the 16G checkpoint are resolved in 16H.
`SailBoatFoundationRegression` now verifies required properties and uses the
correct asset root. NEAT mutation exhausts legal edges before reporting failure,
so `SailBoatPerformanceRegression` reliably seeds its 10,000-boat population.
See [final acceptance evidence](MILESTONE_16_ACCEPTANCE.md).

Run the migration checks with:

```sh
ctest --test-dir cmake-build-debug -R 'SimulationDashboardAcceptance|WorkbenchUIAcceptance|TableViewRegression' --output-on-failure
```

For visual verification:

```sh
cmake-build-debug/examples/SailBoatSimulation/SimulationDashboardTests /tmp/dashboard.png boat 1000 2
```

The arguments are output path, `ant` or `boat`, width, and zero-based tab index.
Tabs are Tools/Colony/Ant/Stats for Ant and Race/Train/History/Models for SailBoat.
The fixture advances real small populations and records snapshots; its timings
are not performance measurements. Golden-image coverage and real-population UI
stress profiling are covered by the completed 16H acceptance report.

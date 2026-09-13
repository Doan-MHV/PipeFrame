# Milestone 16E gallery and acceptance

16E is complete: the standalone `UIGallery` exercises every required engine
control family without loading Ant or SailBoat. Workbench/simulation migration
and milestone 16 as a whole remain unfinished.

## Run

```sh
cmake --build cmake-build-debug --target UIGallery
./cmake-build-debug/examples/UIGallery/UIGallery
./cmake-build-debug/examples/UIGallery/UIGallery --controls
./cmake-build-debug/examples/UIGallery/UIGallery --check
```

Monitoring provides Charts, Network, Table, and Controls navigation. The
Controls back button returns to monitoring. Scroll its content to reach the
four-edge drawer preview. Simulation transport and Zen recovery stay available
at the bottom. Use Tab/Shift-Tab or the pointer to focus controls; Enter commits
numeric input. Lists support arrows and Home/End with scroll reveal.

Popup and modal actions demonstrate input barriers and Escape dismissal. Hover
or activate Tooltip; Toast expires in real UI time. Reduced motion snaps the
demo's animations. Zen hides chrome while preserving transport, recovery, and
playfield input.

The demo owns its `SimulationController`: play/pause, step, reset, and
1X/2X/4X/MAX actually change the tick counter. MAX is bounded to 32 ticks per
displayed frame by this application's policy, not by the engine transport.

## Acceptance mapping

| 16E requirement | Engine facilities | Gallery exercise |
| --- | --- | --- |
| Drawers and collapsible panels | `EdgeDrawer`, `CollapsiblePanel` | Four clickable edge handles; retained animated settings |
| Overlay layers | `OverlayPanel`, `PopupLayer`, `Tooltip`, `Toast`, `ModalBarrier` | Open/dismiss, delay/expiry, background input isolation |
| Basic controls | `Button`, `TextButton`, `SegmentedControl`, `Toggle`, `Slider`, `NumericField` | Mouse/keyboard actions, numeric commit, gain, reduced motion |
| Tabs, scrolling, list/table | `TabView`, `ScrollPanel`, `ListView`, `TableView` | View navigation, scrolling list, 40-row table |
| Monitoring/network | `MetricCard`, `TimeSeriesChart`, `BarChart`, `Gauge`, `ProgressBar`, `NetworkView` | Synthetic metrics, signed bars, node inspection |
| Transport | `SimulationTransport` | Play, pause, step, reset, and all four speeds |
| Zen | `ZenModeShell` | Hidden chrome, retained transport/recovery, world input |

`UIGalleryAcceptance` drives real widget events through `UIManager` and checks
the actions above, collapse interpolation/input disablement, numeric commit,
list navigation, every drawer handle, reduced motion, all transport commands,
Zen recovery, and scaled scroll clipping. `UIFrameworkRegression` also checks
barrier keyboard isolation, capture revocation, Tab/Shift-Tab traversal, Escape,
and hidden-focus cleanup alongside existing layout/state/control regressions.
Chart, network, table, and simulation-controller tests remain separate targets.

```sh
ctest --test-dir cmake-build-debug -R 'UIGalleryAcceptance|UIFrameworkRegression|TableViewRegression|NetworkViewRegression|ChartRegression|SimulationControllerRegression' --output-on-failure
```

## Visual verification

```sh
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/controls.png 640 800 controls
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/zen.png 640 800 zen
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/scaled.png 1650 1200 controls 1.5
```

Scenes: `charts`, `network`, `table`, `controls`, `drawers`, `popup`, `modal`,
`toast`, `tooltip`, `zen`. The last optional argument is UI scale. Controls,
overlays, drawer clipping, and Zen layouts were visually checked at 640×800,
1100×800, and 1650×1200 with 1.5× scale. Golden-image automation and cross-platform
profiling remain in 16H. Graphics access and the configured repository font are
required for tests and screenshots.

## Final-slice public contracts

`CollapsiblePanel(font)` is a retained column with a text-button header and a
clipped, scrollable body. Use `SetCaption`, `SetExpandedHeight`, `GetContent`,
and `SetExpanded`. Expanded height is a viewport budget in logical units;
overflow scrolls. Closing disables content input immediately, animates height
using UI time, then hides the body. Children/state survive collapse. Reduced
motion snaps height.

`Widget::SetInputBarrier(true)` makes a visible, enabled subtree an input
boundary. Popup and modal widgets enable it by default. The topmost barrier
revokes outside pointer capture and confines keyboard focus/traversal before
the next event is delivered. Passive overlays stay input-transparent. Escape
closes a popup or invokes a modal dismissal callback; the callback should hide
the modal. Put the visible modal dialog in a direct child so inside clicks are
distinguishable from its background.

`ScrollPanel` now maps logical clipping bounds through the current axis-aligned
SFML view, preserving clipping under UI scaling. See the individual
[chart](UI_CHARTS.md), [network](UI_NETWORK_VIEW.md), and [table](UI_TABLE_VIEW.md)
contracts for the documented limits of those initial APIs.

## Final verification record — 2026-09-09

- Full debug build passed, including Workbench and all three runtime examples.
- All six 16E acceptance/regression targets passed.
- The full debug suite passed 48 of 50 tests, including Ant stress coverage.
- `SailBoatFoundationRegression` failed its expected visual-control count;
  its output also reported missing relative texture/font/audio asset paths.
- `SailBoatPerformanceRegression` failed to seed a connected initial genome
  for the 10,000-boat stress population.
- A missing `WallBuilder.cpp` source dependency in `AntFoodTests` was repaired
  during the build audit; that target now builds and passes.

The two SailBoat failures are outside the 16E acceptance targets and remain
recorded work for migration/hardening. This record does not claim a fully green
milestone 16 or full-project regression suite.

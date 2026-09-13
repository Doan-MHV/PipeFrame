# Shared charts and monitoring gallery

Include `<PipeFrame/UI/Chart.h>` and link `PipeFrame::Engine`. The chart widgets
use normal `Widget` sizing, composition, parent movement, and opacity. They are
passive: they do not take pointer input from the world. Labels, legends, and
hover inspection can be composed around the plotting surface; they are not
built into this initial API.

```cpp
auto &history = panel.CreateChild<TimeSeriesChart>();
history.SetSize({320, 160});
history.SetSeries({
    {{{0, 12}, {1, 15}, {2, 14}}, UITheme::Dark().accent},
    {{{0, 10}, {1, 11}, {2, 13}}, UITheme::Dark().success},
});
history.SetVerticalRange(ChartRange{0, 20});

auto &comparison = panel.CreateChild<BarChart>();
comparison.SetValues({4, -2, 7, 3});
comparison.SetVerticalRange(std::nullopt); // Automatic range includes zero.
```

- `SetSeries` replaces the owned display snapshot. The application controls
  history length and update frequency; no unbounded history accumulates inside
  the widget. Rendering reuses geometry until data, layout, or opacity changes.
- Line samples connect in supplied order. Supply increasing x coordinates for
  a time series. A non-finite x or y breaks the line. A single point contributes
  to axis bounds but has no connecting segment.
- Automatic bounds combine all finite samples across series. Empty data uses
  0–1; constant data gets a nonzero span. Fixed ranges must have finite,
  increasing endpoints and a finite span or the setter throws
  `std::invalid_argument` without changing the previous range.
- Bars extend from zero to the signed sample value. `SetValues` assigns integer
  category coordinates; bars are 0.8 data units wide. Multiple bar series at the
  same x coordinate overlap in supplied order; grouped/stacked bars are not
  implemented.
- Both fixed axes clip plotted data to the widget. Grid divisions are capped at
  32; zero hides the grid. Series and grid colors inherit parent opacity.
- No domain calculations, font dependency, simulation clock, or rendering
  animation is embedded in a chart. Use application sampling and widget motion
  as appropriate.

## Run the gallery

With the repository's debug build configured and examples enabled:

```sh
cmake --build cmake-build-debug --target UIGallery ChartTests NetworkViewTests TableViewTests UIFrameworkTests
./cmake-build-debug/examples/UIGallery/UIGallery
ctest --test-dir cmake-build-debug -R 'TableViewRegression|NetworkViewRegression|ChartRegression|UIFrameworkRegression|SimulationControllerRegression' --output-on-failure
```

The gallery loads the repository font via its CMake-configured source path.
Move the gain slider to change both charts and the monitoring values. The toggle
pauses or resumes synthetic signal animation. Resize the window to compare the
horizontal and stacked plot layouts. The Network tab demonstrates the generic
graph surface; click a node to change the inspection selection. See
[`UI_NETWORK_VIEW.md`](UI_NETWORK_VIEW.md) for its data contract. The Table tab
demonstrates scrolling and selection over 40 sample rows; see
[`UI_TABLE_VIEW.md`](UI_TABLE_VIEW.md). The Controls view now covers drawers,
overlays, collapsible settings, numeric fields, lists, transport, and Zen mode;
see [`UI_GALLERY.md`](UI_GALLERY.md) for the full 16E acceptance mapping.

Export a deterministic frame, optionally specifying width and height:

```sh
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/gallery-wide.png 1100 800
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/gallery-narrow.png 640 800
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/gallery-network.png 640 800 network
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/gallery-table.png 640 800 table
```

The gallery and rendering tests require graphics access, including when using an
offscreen render texture. On macOS, a restricted sandbox may not provide that
access. Screenshot export is a manual visual check, not a golden-image assertion.

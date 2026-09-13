# Building an application with PipeFrame UI

`examples/ThermalLab` is a standalone example using only public PipeFrame and
SFML headers. It contains no Ant, SailBoat, Workbench, or project-runtime code.
Build the `ThermalLab` target and run it to adjust a synthetic thermal target,
play/pause, step, reset, or change speed. It is deliberately small enough to
read as a complete application.

## Ownership and construction

Link your executable to `PipeFrame::Engine`. Keep its font alive longer than
widgets using it. `UIManager` owns roots; each widget owns its children. State
objects normally precede the UI manager in the application's member list so
widget subscriptions are destroyed first. Bindings also safely tolerate state
being destroyed first.

```cpp
pipeframe::ui::State<float> target{70};
UIManager ui;
auto &column = ui.CreateRoot<Column>();
column.SetPadding(Thickness{20});
column.SetSpacing(12);
auto &slider = column.CreateChild<Slider>();
slider.SetSize({0, 32});
slider.SetRange(0, 100);
pipeframe::ui::BindValue(slider, target);
slider.SetOnValueChanged([&](float value) { target.Set(value); });
```

`Compose` and keyed `Component<T>` descriptions reconcile an existing tree when
conditional structure changes. Ordinary live values should flow through state
bindings or setters. Recreating the tree every frame loses interaction state
and adds unnecessary work. Stable IDs likewise preserve selection in tables.

## Layout and scaling

`Column`, `Row`, and `StackPanel` distribute children in order. Use padding,
spacing, and `SetChildFlex` instead of computing coordinates for each control.
`Fixed`, `FitContent`, and `Stretch` describe sizing; `Measure` resolves desired
size under `BoxConstraints`, and `Arrange` assigns the available rectangle.
No-op arrangement preserves retained geometry. Changes to size or visibility
invalidate and reflow affected layouts.

ThermalLab switches its body from a row to a column below 760 logical pixels.
Only its root is arranged from the window dimensions. For high-DPI rendering,
set the SFML view to logical dimensions and render into the physical pixel
surface. Convert incoming pointer positions into those same logical coordinates
before dispatch. Do not scale both widget geometry and the view.

`ScrollPanel` clips content, including under a scaled render view. Labels can
wrap with `SetWrap(true)`; give multiline text enough vertical space. Layout
constraints cannot make an arbitrarily small window usable, so choose a useful
minimum window size for your application.

## Events, state, and clocks

Call `UIManager::HandleEvent` before world controls. A true result means the UI
owns the event. The manager routes focus, hover, pointer capture, and Tab
traversal. Overlay barriers isolate input and revoke outside ownership.
Project-runtime integrations use the additional contracts documented in
`SIMULATION_DASHBOARDS.md`.

Advance `UIManager::Update` using real frame time, even while simulation is
paused. Simulation time and speed belong to `SimulationController`, not to the
UI animation clock. ThermalLab's shared `SimulationTransport` callbacks control
its application-owned controller; the transport has no physics or training
knowledge.

Charts accept application-owned samples. NetworkView accepts nodes and edges;
it does not know about genomes. History persistence and selection semantics
remain with the application. `SimulationDashboard` is an optional common drawer
and scrolling-tab composition, not a requirement for using individual controls.

## Tests and reviewed screenshots

`ThermalLab --check` verifies state/keyboard behavior and compares three reviewed
baselines in `examples/ThermalLab/goldens`: 640×800 at 1×, 1200×800 at 1×, and
1200×1200 at 1.5×. The fixture uses a fixed font, paused state, and deterministic
samples. Baselines tolerate small rasterization differences: fewer than 0.5% of
pixels may differ by more than eight channel levels, with mean maximum-channel
error below 0.5. Geometry changes large enough to affect layout should fail.

`--update-goldens` explicitly regenerates the files. Inspect all three images
before accepting a baseline change. Normal CTest runs only compare and never
rewrite baselines. Different graphics/font backends may require a separately
reviewed platform baseline; do not silently loosen thresholds to hide failures.

The existing UI framework, gallery, Workbench, and simulation-dashboard tests
cover constraints, bindings, controls, overlays, drawers, capture, transport,
selection, and live edits. `UIHardeningRegression` adds no-op invalidation,
visibility/size reflow, frame-partition-invariant motion, paused real-time
animation, binding teardown, and closed-drawer focus checks.

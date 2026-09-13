# Generic network view

`NetworkView` is a passive plotting widget in `<PipeFrame/UI/NetworkView.h>`.
Link `PipeFrame::Engine`. Supply nodes and connections without importing any
simulation, training, or genome types into the engine:

```cpp
auto &network = panel.CreateChild<NetworkView>();
network.SetSize({320, 180});
network.SetGraph(
    {{10, {0, 0.5f}}, {20, {1, 0.5f}, UITheme::Dark().success}},
    {{10, 20, UITheme::Dark().textSecondary, true}});
network.SetSelectedNode(20);
```

Nodes have stable unsigned 64-bit IDs, normalized positions in [0,1], and colors.
Edges refer to node IDs and supply a color and optional direction arrow. The
application owns graph layout and domain meaning; positions adapt to the widget
size with an inset that keeps nodes and selection rings inside its bounds.
Node radius is in logical UI units and shrinks when the surface is too small.

`SetGraph` validates the complete snapshot before replacing the existing graph.
Duplicate IDs, non-finite/out-of-range positions, missing edge endpoints, and
self edges throw `std::invalid_argument`. Selection survives replacement while
its ID exists and clears when that node disappears. Selecting an unknown ID or
setting a non-finite/non-positive node radius also throws.

`FindNodeAt(screenPoint)` returns the last-painted node under a point, or
`std::nullopt`. It follows resize, parent movement, and visual offsets, and
returns no node when the widget or an ancestor is hidden or disabled. It is a
geometry query: applications remain responsible for overlay/scroll clipping
and event arbitration before invoking it. The widget itself does not consume
pointer input or acquire focus. For example, after routing UI overlays:

```cpp
if (auto id = network.FindNodeAt(pointerPosition)) {
    network.SetSelectedNode(id);
    // Update the application's inspector for this ID.
}
```

Edges render before nodes. Directed edges end at the target with an arrowhead;
connections too short to fit between node margins are omitted. Coincident nodes
remain valid and inspect in paint order. Geometry is cached between graph,
selection, size, and opacity changes. Parent opacity applies to nodes and edges.

This initial surface does not provide self loops, curved/parallel-edge routing,
automatic graph layout, labels, zoom/pan, or drag editing. Those capabilities
remain future work rather than hidden application dependencies.

The standalone `UIGallery` Network tab demonstrates nine nodes and eighteen
directed edges with application-owned selection. See [gallery commands](UI_CHARTS.md).
`NetworkViewRegression` checks rendering, arrow direction, inherited opacity,
selection, validation, motion/resize picking, hidden ancestors, collapsed views,
and world-input pass-through. Rendering tests require graphics access.

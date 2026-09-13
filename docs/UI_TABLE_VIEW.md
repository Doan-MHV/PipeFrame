# Table view

Include `<PipeFrame/UI/TableView.h>` and link `PipeFrame::Engine`. Keep the font
alive for the lifetime of the table. Data and selection are application-facing
values; the engine has no knowledge of runs, models, colonies, or boats.

```cpp
auto &table = panel.CreateChild<TableView>(font);
table.SetSize({480, 240});
table.SetData(
    {{"Name", 2}, {"Score", 1}},
    {{"baseline", {"Baseline", "100"}}, {"candidate", {"Candidate", "125"}}});
table.SetSelectedRow("candidate");
table.SetOnSelectionChanged([](std::optional<std::string> id) {
    // Update application inspection state.
});
```

Columns divide the available width using positive weights. The header stays
fixed while the body scrolls vertically. Cells accept UTF-8, replace line breaks
and tabs with spaces, and truncate at character boundaries with an ellipsis.
Text and partial rows are clipped to their cell/body bounds. Scissors respect
the current axis-aligned SFML view, including UI scaling and ancestor scissors.
Rotated render views are not supported.

`SetData` atomically replaces the snapshot. Row IDs must be unique and nonempty;
every row must have exactly one cell per column. Invalid data or non-finite/
non-positive column weights throw `std::invalid_argument` before changing state.
Selection is retained by ID across reordering and cleared if its row disappears.
Scroll offset is clamped after data or geometry changes. Data replacement cancels
an in-progress row click and does not emit a selection callback.

`SetSelectedRow` accepts an existing ID or `std::nullopt` to clear. An unknown ID
throws. Changing selection reveals the row when the body has usable height.
Programmatic selection is silent unless `notify = true`; user selection notifies
once when the ID changes. Typed state integration can use the existing
`BindProperty` extension point.

Pointer selection requires a left press and release on the same row. Releasing
outside cancels; header clicks do not select rows. The focused table supports
Up/Down, Home/End, and Page Up/Page Down. The wheel scrolls the body, and a passive
indicator shows its scroll position. Disabled tables cannot select rows; hidden
tables leave input available to the world. The table uses the existing manager's
focus, capture, and event routing.

Only visible rows are prepared as cached text on data, scroll, geometry, focus,
enabled-state, or opacity changes. The full data snapshot remains in memory.
This first primitive has read-only cells and single selection; sorting, editing,
column dragging, horizontal scrolling, a draggable scrollbar, and multiselection
are not included. Applications can sort their data before calling `SetData`.

## Gallery and verification

The `UIGallery` Table tab contains 40 sample rows and a selection readout.

```sh
cmake --build cmake-build-debug --target UIGallery TableViewTests
./cmake-build-debug/examples/UIGallery/UIGallery
./cmake-build-debug/examples/UIGallery/UIGallery --snapshot /tmp/gallery-table.png 640 800 table
ctest --test-dir cmake-build-debug -R TableViewRegression --output-on-failure
```

`TableViewRegression` covers click/capture cancellation, keyboard navigation,
fixed headers, scrolling/hit testing, stable selection, invalid replacements,
clipping at normal and scaled views, pre-layout selection, hidden/disabled
behavior, and collapsed geometry. Rendering and screenshots require graphics
access. The font path is configured by CMake from the repository assets.

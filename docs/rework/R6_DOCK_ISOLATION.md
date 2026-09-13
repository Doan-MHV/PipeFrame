# R6 dock isolation — September 12, 2026

Historical checkpoint. The remaining package-1 boundary is now closed; see [R6_PUBLIC_UI_ISOLATION.md](R6_PUBLIC_UI_ISOLATION.md).

Status: dock interaction slice completed; package 1 and R6 remain open.

## Implemented

`engine/include/PipeFrame/UI/DockResizeController.h` owns axis selection, incremental
pointer deltas, drag state and cancellation through neutral InputEvent. Layout clients
provide a callback and retain responsibility for size limits and persistence. Hosts
must hit-test presses and maintain pointer capture. This controller needs no native
headers, font, renderer or window.

The native Panel host now lives at `engine/include/PipeFrame/Backend/SFML/DockSplitter.h`
and its backend source, replacing the editor-local DockSplitter files. WorkbenchLayout
uses this engine host for both dividers. Native event conversion uses InputEventAdapter.
Focus loss, window exit, Escape and orientation changes cancel dragging. Hover styling
refreshes when dragging ends, including release outside the splitter.

This is a separation of reusable interaction logic from its native widget host, not
completion of the public Widget migration. WorkbenchLayout still owns native layout.
No Ant source change or plugin ABI change was needed (ABI remains 6).

## Verification

Debug Engine, SimulationWorkbench and WorkbenchUITests build passed. Dependency lint
passed with DockResizeController added to its neutral-header guard. DockResizeTests
links Foundation only, proving its public header compiles without native include paths.

DockResizeRegression and WorkbenchUIAcceptance: **2/2 passed**, 5.47 seconds.
Tests cover both axes, zero/negative deltas, button filtering, release, focus/window
exit/Escape cancellation, orientation change and malformed event payloads. Native editor
acceptance additionally drags the actual dock and verifies viewport width and release.
Saved test output: `evidence/r6-dock/tests.txt`.
Captured and visually inspected `evidence/r6-dock/editor-800.png`: wrapped toolbar,
stacked narrow-layout docks and clipped/scrollable Inspector remain separated from the viewport.

## Remaining

Window/layout/input host isolation and the broader native Widget/Controls boundary remain.
PopulationBoundsRenderer is another native editor header (currently unused by the build).
Playground creation, brush tools and their public developer lifecycle are still planned,
not implemented by this change. Asset/material integration and performance gates remain
as recorded in R6_EXECUTION_PLAN.md.

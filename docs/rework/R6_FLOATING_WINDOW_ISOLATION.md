# R6 floating-window interaction — September 12, 2026

Historical checkpoint. The remaining package-1 boundary is now closed; see [R6_PUBLIC_UI_ISOLATION.md](R6_PUBLIC_UI_ISOLATION.md).

Status: interaction extraction verified; native host/widget isolation remains open.

## Implementation

`engine/include/PipeFrame/UI/FloatingWindowController.h` provides neutral move/resize
state and bounds constraints. Hosts provide titlebar/grip hit testing, pointer capture,
focus-loss handling and persistence. Begin records the initial pointer and bounds;
Update calculates the constrained rectangle; End releases controller state.
Constrain is also used for host window resizing, replacing duplicated Workbench code.
The standard minimum is 360 x 240 with an 8-pixel margin. When the viewport is smaller,
the available size wins so the panel itself does not extend beyond the viewport.

WorkbenchView now uses this controller for floating tools. Its native geometry and event
conversion remain in the host; no SFML enters the controller. Switching floating/docked
mode ends any old drag. Focus loss/window exit retain and persist the current bounds,
matching the previous host behaviour. This change does not migrate the native window
widget itself or establish responsive layout for every child at extremely small sizes.

## Validation

Debug SimulationWorkbench and WorkbenchUITests builds passed. Dependency lint passed;
the new controller is protected by the neutral-header guard. Foundation-only
DockResizeRegression now checks floating movement, resize, minimums, offscreen clamping,
small viewport bounds and gesture completion. Native WorkbenchUIAcceptance checks actual
floating drag, resize, focus-loss stop and workspace-layout persistence.

**2/2 tests passed**, 4.13 seconds. Output: `evidence/r6-floating/tests.txt`.
No Ant behaviour, plugin ABI or SailBoat changes. ABI remains 6.

## Still open

WorkbenchView, WorkbenchLayout and WorkbenchInput still expose native host contracts.
The public Widget/Controls migration is unfinished. Playground and brush implementation
remain planned; this extraction does not complete those packages or all of R6.

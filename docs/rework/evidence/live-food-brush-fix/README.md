# Live food and held brush outline — 2026-09-13

Debug and Release: CustomBrushAcceptance, WorkbenchUIAcceptance and
AntDashboardAcceptance each passed (3/3 per configuration).

WorkbenchUIAcceptance sends press/move through WorkbenchInput and checks that
both the translucent footprint and solid yellow ring render while captured.
AntDashboardAcceptance opens the real declarative tools in an authored Playground
scene, selects Add Food, and right-clicks through runtime input while playing and
paused. It verifies food appears without replacing the world or changing its tick
or ant population. A radius of three accommodates world-to-pixel rounding at the
small viewport. These are focused regressions, not a full-suite/performance rerun.

The PNGs are native dashboard renders at 1000x800 and 640x480. Panel 0 is the
FOOD / TOOLS drawer; its lower controls are reachable by scrolling. They do not
capture the user's open editor or demonstrate a full interactive simulation frame.

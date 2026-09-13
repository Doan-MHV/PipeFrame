# Milestone 17 visual parity audit

Status: **passed after 17I correction**

The failures recorded below describe the pre-17I baseline. The corrected
resolution matrix and review result are stored in
[`COMPARISON_BOARD.md`](COMPARISON_BOARD.md).

This audit compares the running PipeFrame dashboards with frames from Pezzza's
published Ant and SailBoat videos and the matching checked-in reference source.
Behavioral completion in 17G and 17H does not imply visual parity.

## Ant

Reviewed video frames:

- [04:41 — selected-ant and settings surfaces](https://www.youtube.com/watch?v=TfhrF7VrzWQ&t=281s)
- [11:42 — two-colony overview](https://www.youtube.com/watch?v=TfhrF7VrzWQ&t=702s)

The video keeps the simulation full-screen. A compact timer floats at the top,
the transport floats at the bottom, settings/editor handles sit separately on
the left, and colony/selected-ant cards sit separately on the right. At 11:42,
two small colony cards overlap only the lower center of the world while trails,
food, walls, colony colors, and ant motion remain dominant.

`AntPezzaSource/src/ui/ui.hpp::initializeUI()` confirms this composition. It
constructs six separate drawers: settings, editor, timer, control, colony, and
selected ant. `AntPezzaSource/src/ui/standard/drawer.hpp` gives every drawer its
own side, side position, compact rotated handle, hidden/visible positions, and
eased motion.

Current PipeFrame failure:

- `SimulationDashboard` puts TOOLS, COLONY, ANT, and STATS into one 340-pixel
  full-height left drawer.
- Timer and transport are not independent top/bottom surfaces.
- Colony and selected-ant content cannot coexist as compact right-side cards.
- The large handle spans the drawer height; Pezzza's handle is a small rotated
  tab centered on its card.
- The dark opaque dashboard replaces a large part of the world instead of
  floating over a blurred world sample.

## SailBoat

Reviewed video frames:

- [05:39 — complete training workspace](https://www.youtube.com/watch?v=4ac1Kh30Ppo&t=339s)
- [09:53 — timer, result, selected boat, transport, and network](https://www.youtube.com/watch?v=4ac1Kh30Ppo&t=593s)

The video keeps water and the race course visible beneath the UI. Wind is a
small top-left card. The timer occupies a compact left card. Training result and
selected boat are separate right cards. The network is a wide, shallow
bottom-right drawer, and the transport remains centered at the bottom.

`SailBoatPezza/src/ui/ui.hpp::initializeUI()` confirms seven independent
drawers: settings, editor, training timer, training result, boat info,
simulation control, and network, plus a standalone wind widget. The source
scales every surface from viewport height (`height / 1200`) and calculates each
drawer's side position after measuring its own effective size.

Current PipeFrame failure:

- RACE, TRAIN, HISTORY, and MODELS share one full-height tab drawer.
- Training result and selected boat are stacked inside the TRAIN page instead
  of remaining independent right-side cards.
- The network drawer consumes the full viewport height instead of using a wide,
  shallow lower-right card.
- The wind indicator is trapped inside RACE rather than remaining visible.
- The timer is centered, while the reference places it in the left stack.
- Opaque dark surfaces and generic blue accents do not reproduce Pezzza's
  translucent blue-grey glass, white outlines, warm yellow progress accents,
  or blurred-water background.

## Required 17I correction

1. Replace the app-facing tab composition with independently measured edge
   drawers. Keep tabs only for editor/workbench workflows that actually need
   them.
2. Change `EdgeDrawer` to use a compact rotated handle whose length is
   independent of panel height.
3. Add side stacking and top/center/bottom anchoring to the shared layout so
   Ant and SailBoat can reproduce their distinct source arrangements.
4. Add viewport-height UI scaling and retain the world behind translucent
   surfaces.
5. Recompose Ant from six surfaces and SailBoat from seven surfaces plus wind,
   using their checked-in source widgets as the field and control checklist.
6. Capture closed, open, hover, and selected states at 720p, 1080p, 1440p,
   narrow width, and two UI scales before accepting 17I.

These structural differences were removed in 17I. The retained comparison board
is the acceptance evidence for the corrected composition.

## Composed Workbench correction

The first 17I acceptance pass used isolated dashboard captures and missed
overlap in the hosted Workbench view. The corrected interaction uses compact
white buttons only while all drawers are collapsed. Opening one button hides
the drawer-button layer and presents one complete popup with a CLOSE action.
Only one popup may remain open, collapsed drawer surfaces do not render behind
it, and popup contents keep their authored minimum width instead of wrapping
into neighboring rows. The comparison board retains both the collapsed state
and representative open-popup states.

The follow-up interaction correction also assigns non-overlapping slots to
buttons sharing an edge and removes collapsed drawer bodies and scroll surfaces
from hit testing. Acceptance now exercises close, restore, open-another, and
close-again through the same runtime input route used by the Workbench.

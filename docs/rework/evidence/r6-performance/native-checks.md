# Native visible-window interaction verification

September 13, 2026, Release Workbench and Release Ant plugin, disposable copy at
`/tmp/r6-performance-native-project`; original project assets were not painted or saved.
Captures are native window screenshots (998×768 pixels returned by the capture service).
These checks ran separately from the timed 1440×900 offscreen fixtures.

- Selected Ant Colony: Transform showed position (96, 108): `native-before.jpg`.
- Scrolled Inspector to Colony State/History and Add Component: `native-scrolled.jpg`.
- Dragged colony center from screenshot (211,368) to (246,349): visible marker and
  coordinate label moved to (116,97), preserving Inspector scroll: `native-dragged.jpg`.
- Selected Playground, opened Assets → Maps → Main.pftilemap → Edit Map and framed it:
  `native-map-before.jpg`.
- Selected Rectangle and dragged (360,278) to (438,320): wall rectangle visibly filled:
  `native-painted.jpg`.
- Scrolled to Undo Map and clicked it: rectangle removed, original terrain restored:
  `native-map-undone.jpg`.
- Closed the disposable editor after inspection; verified its process exited.

The mouse actions reached the real event loop and the following captures showed the
expected visible changes. These still images do not establish a numeric physical
input-to-photon latency or prove uninterrupted smoothness throughout a gesture.
Quantitative handler/software-frame timings are in the CSV fixtures, not inferred
from the duration of remote automation calls.

Test setup limitation: the shell-based .app launcher failed; a local compiled launcher
opened the project via --project. Keyboard injection in the native folder picker produced
incorrect characters, so that picker was bypassed in the launcher. No keyboard-based
latency claim is made. Native verification used mouse selection, scrolling and dragging.

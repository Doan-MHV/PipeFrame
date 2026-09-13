# R6 built-in environment workflow evidence

September 12, 2026. Ant-only configuration; SailBoat is excluded.

- debug-tests.txt: complete 73/73 suite passed, 103.78 seconds.
- final-focused-tests.txt: after the final axis-lock edge-case fix, all four affected
  dependency/gesture/environment/native-editor checks passed (4.84 seconds).
- release-tests.txt: final Release environment/gesture/dependency checks.
- debug-build.txt, release-build.txt, final-debug-build.txt, final-release-build.txt:
  successful all-target builds, including public header and Ant boundary compilation.
- normal-ui.txt, narrow-ui.txt: successful native Workbench acceptance captures.
- map-tools-1440.png: normal map tools, radius, axis lock, history and size controls.
- map-layers-800.png: narrow scrolled palette and tile metadata controls.

Both images were inspected: controls remain inside the dock and scroll to reachable
content; clipped rows at the scroll edge are intentional. These synthetic fixtures
verify layout and workflow, not Pezzza appearance parity or the later performance gate.

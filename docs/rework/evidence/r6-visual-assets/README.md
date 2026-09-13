# R6 visual asset verification

September 12, 2026. Debug/Release builds with PIPEFRAME_ANT_REWORK_ONLY=ON.

- debug-tests.txt: all 72 tests pass, 91.58 seconds.
- release-tests.txt: VisualAssetAcceptance and PipeFrameDependencyLint pass.
- debug-build.txt / release-build.txt: final incremental all-target builds succeed.
- material-ui.txt / tileset-ui.txt: native Workbench acceptance passes with captures.
- materials-1440.png / material-fields-1440.png: material browser and scrolled form.
- tilesets-800.png / tileset-fields-800.png: narrow tileset browser and scrolled form.
- textured-playground.png: pixel-tested two-color ground repetition and atlas tile.

Screenshots were inspected. Scroll clipping at the panel edge is intentional;
fields, save actions, previews and status remain accessible. These fixtures use
synthetic assets and the test runtime; they do not claim Pezzza visual parity or
completion of the other R6 work packages.

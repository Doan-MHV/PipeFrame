# World debug controls validation

Release all-target build passed. Initial full Release suite passed 77/80; generated
project compilation found a wrongly namespaced RenderContext and UI acceptance
found excess toolbar wrapping interfering with an asset drag. Both were corrected.
The three failing tests then passed: WorkbenchUIAcceptance, ProjectBuildAcceptance,
and BlankProjectAcceptance. All 80 Release tests have therefore passed across the
suite and targeted reruns.

A final correction to display a fallback motion circle alongside a trigger box
passed SpritePhysicsAcceptance, SceneProjectRuntimeAcceptance and WorldModuleRegression.
Tests cover channel independence, toolbar clicks, debug output with no simulation
advance, hidden collider outlines, sprite triangle diagonals, actual fallback
shapes, neutral untextured line submission, shared scene ownership, and generated
world-template compilation/save/reopen/build/reload.

Native UI acceptance was rerun with screenshot capture; both Ant overlays were
visually inspected and saved here as ant-debug-physics.png and ant-debug-mesh.png.
The generated-project acceptance now compiles custom debug implementations in each
world and verifies independent and combined geometry reaches the editor host.
All three focused tests and native UI acceptance passed.

No new performance percentile measurement is claimed. Debug
geometry has extra rendering cost while enabled; collection is skipped when both
channels are off. Ant point LOD has no triangle mesh.

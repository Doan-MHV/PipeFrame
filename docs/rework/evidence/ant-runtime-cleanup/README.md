# Ant outer Runtime cleanup

Release all-target build: passed, including AntUIBoundaryCompile with the relocated
Editor/AntUIHost.cpp. Full Release CTest: 80/80 passed. Native WorkbenchUIAcceptance
with evidence capture: passed.

The captured ant-debug-physics.png and ant-debug-mesh.png under
/tmp/pipeframe-ant-runtime-refactor were byte-for-byte identical to the saved
pre-refactor images in ../world-debug-controls/. No rendering difference was
introduced for these deterministic scenarios. This is not exhaustive visual
coverage of every scene or a new interactive latency benchmark.

Responsibilities moved out of the 1,335-line AntSimulationRuntime.cpp (now 954):
- AntWorld::FromScene: validation, environment loading, authored world creation,
  food placement and entity restoration, with injected project factory binding.
- AntRenderingWorld: asset loading, render options, frame geometry, terrain and
  material preparation, beacon drawing, layer ordering and render statistics.
- AntEnvironment: food quantity queries by radius.
- Editor/AntUIHost.cpp: dashboard hosting.

Runtime/ is retained for host lifecycle, registration, authored scene integration,
input/selection routing and editor state. No public project method was removed;
render option/statistic type aliases preserve dashboard callers.

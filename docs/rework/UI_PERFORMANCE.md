## September 13 — R6 package 7 complete

Three matched Release repetitions pass editor/1/3/5-colony gates. Worst five-colony
p95 is 16.56 ms; map memory/frame budgets and native interaction checks also pass.
See [current measurements, limitations and evidence](R6_PERFORMANCE_GATES.md).
The September 12 results below are historical checkpoints.

## Historical R6 render-boundary measurement — September 12

After the approved Camera2D/RenderContext migration, Release Inspector scrolling measured
1.64 ms median / 2.02 ms p95. Matched 1/3/5-colony runs measured 4.83/5.75,
10.44/11.48 and 15.62/17.00 ms median/p95 respectively. Five-colony p95 exceeds the
16.67 ms target; retain this performance concern. Earlier measurements below are
historical runs, not a guarantee that all runs meet the target. See
[R6 render-boundary evidence](R6_RENDER_BOUNDARY.md).

# Editor performance correction after R5

## R6 optimized follow-up — September 12, 2026

The older Debug results below are retained for comparison. The current Release benchmark
at 1440×900 uses Main.png, seed 1, 1000 initial ants per colony, fixed 1/60-second steps,
1200 warmup ticks and 180 measured frames. It renders the real editor and simulation into
an offscreen target. Positions, workload and live population are printed in the logs.

| Colonies | Live ants | Before median/p95 (ms) | After median/p95 (ms) |
|---|---:|---:|---:|
| 1 | 1000 | 4.73 / 5.80 | 4.40 / 5.26 |
| 3 | 3023 | 13.52 / 15.32 | 9.58 / 10.43 |
| 5 | 4764 | 20.20 / 22.37 | 14.03 / 15.25 |

Sampling identified avoidance candidate/body lookups as a major cost. The avoidance grid
now uses the existing engine UniformSpatialIndex with body pointers borrowed for the physics
batch, eliminating repeated candidate-ID/body-ID hash lookups. Grid insertion and candidate
iteration order are preserved. Body storage cannot be structurally changed during this batch.
No timestep, collision equations, solver order, image quality or population settings changed.

A separate 1200-tick three-colony before/after checkpoint matched exactly:
5345435303430926353; 3000 ants; 3000 births; 0 deaths. Full regression: 68/68 passed.

The short Release Inspector fixture measured paused scrolling at median/p95 1.57/1.91 ms.
This is software/offscreen timing, not physical event-to-photon latency. Debug builds are
not covered by the Release frame-budget result. Real-window/input latency remains a separate
acceptance item; these observations are not a universal 60 FPS guarantee.

Evidence: [before](evidence/r6-build/release-colonies-before.txt),
[after](evidence/r6-build/release-colonies-after.txt),
[Inspector](evidence/r6-build/release-inspector.txt), and
[checkpoint source](evidence/r6-build/checkpoint.cpp).


September 12, 2026. UI CPU improvements implemented. The overall 60 FPS performance gate remains open before R6; do not confuse functional R5 acceptance with responsiveness acceptance.

## Changes

- MountedViewHost checks root/source revisions before copying, validating and resolving descriptions. Clean frames still arrange changed bounds; input, animation and nested widget updates continue normally.
- ViewRenderer preserves arranged control sizes when requested dimensions have not changed. Previously every reconciliation could collapse stretch/fit controls to zero and trigger cascading layout work before restoring their final bounds.
- Label caches up to eight measured text widths. Text, font size, wrap and padding changes invalidate the cache; resize selects/recomputes the appropriate measurement. The bounded cache handles repeated constrained/unconstrained measurements without repeatedly wrapping each character through SFML text geometry.
- Toolbar and shared transport setters ignore unchanged values. Inspector refresh keeps authoritative schema/property queries and current command data, but only rebuilds controls when their visible presentation changes. Workspace tools do not rebuild the selected page merely because another page's telemetry changed.
- No simulation algorithms, ECS ownership, worker count, rendering quality or timestep were changed. No artificial UI update delay was added.

## Measurements

The reproducible `WorkbenchUITests --ui-benchmark` fixture refreshes a paused selected Inspector and the editor UI at 1100×800. Ten warmup iterations precede 120 measured iterations. Both revisions used the existing Debug build. It excludes GPU rendering, window presentation and operating-system input latency.

| CPU refresh/update fixture | Median | p95 |
|---|---:|---:|
| Before | 801.630 ms | 829.440 ms |
| After | 0.295 ms | 0.321 ms |

[Before log](evidence/ui-performance/before.txt), [after log](evidence/ui-performance/after.txt). These are local desktop observations, not a controlled hardware benchmark or an application FPS estimate. The synthetic fixture deliberately refreshes every frame; its absolute pre-fix time is not the user's screenshot frame time.

`WorkbenchUITests --ant-ui-benchmark` exercises the real Ant runtime at 1440×900 in an offscreen target, with a selected colony. Each case has ten warmup and 120 measured frames. The scrolling case routes wheel events through the editor. Playing advances exactly one 1/60-second simulation step per measured frame; it is not wall-clock catch-up or a sustained population benchmark.

| Real Ant, current Debug build | Median frame | p95 frame |
|---|---:|---:|
| Paused, selected colony | 20.30 ms | 21.32 ms |
| Paused, scrolling Inspector | 20.36 ms | 21.30 ms |
| Playing one colony | 47.30 ms | 49.60 ms |

[Ant results and stage timings](evidence/ui-performance/ant.txt). Paused UI refresh/update averages 0.50 ms, scrolling input 0.19 ms. Playing averages 21.73 ms in input/simulation and 5.61 ms in UI refresh/update. World submission is around 19 ms in these captures; this stage includes clearing the target and may include driver/GPU synchronization, so it does not establish a CPU-only world-render bottleneck. Offscreen display timings do not measure screen presentation latency. There is no pre-fix full Ant measurement for this harness.

## Verification and remaining gate

The mounted runtime regression checks that 100 unchanged frames do not re-resolve a source, while narrowing the viewport still reflows text. Existing checks cover live component writes, history/persistence, exact integer fields, Inspector wheel scrolling and retained offset, selection changes, dashboard capture/input, layout and golden compatibility. [Scrolled Inspector image](evidence/ui-performance/inspector-scrolled.png) was visually reviewed.

The measured UI regression is substantially reduced, but the full frame does not yet meet 16.7 ms. Before advancing R6, profile simulation and world rendering separately in comparable Debug and optimized builds, then test 1/3/5 colonies with matched populations, seed, camera, resolution and simulation speed. Track median/p95 frame time and real pointer-to-present latency; preserve deterministic behavior and visual output. Do not claim restored 4–5-colony capacity from this UI correction.

Final build passed; **64/64 tests passed** in 59.92 seconds. [Full acceptance results](evidence/ui-performance/tests.txt). No golden-image thresholds or baselines changed in this correction. Restart Workbench to load the rebuilt application/runtime.

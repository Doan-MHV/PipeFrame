# R6 package 7 — Measured performance gates

Status: **complete, final verification September 13, 2026**. A reusable contact
workspace closes the intervening ABI 11 miss. Final ABI 12 five-colony p95:
**16.356 / 16.185 / 16.491 ms**; all other gates pass in all three runs.
[Final implementation, tests, raw evidence and limits](R6_FINAL_PERFORMANCE.md).
The original measurements below remain preserved as the earlier package-7 checkpoint.

The existing editor and 1/3/5-colony Release fixtures retain their 16.67 ms p95
software-frame target, 1440×900 target, seed 1, 1000 configured ants per colony,
1/60-second fixed timestep and 1200 warmup ticks. Run three repetitions, record
live counts and simulation/UI/render stages, and retain every run. No population,
quality, timestep or worker-count reduction is permitted to pass a gate.

Before measuring the newly added map workload, define these fixtures and budgets:

- Small: 384×216; large: 1024×1024. Unit cells, one layer, deterministic 20% solid
  occupancy, whole map visible at 1440×900. No per-cell scene entities.
- 10 warmup frames, 120 measured radius-4 pencil stamps, alternating paint/erase.
  Include event handling, UI refresh, geometry updates, map drawing and offscreen
  display. Paint-frame p95 <=16.67 ms; input-handler p95 <=4 ms.
- Import/open/first rendered map <=1000 ms; save/reimport <=1000 ms; peak process
  resident memory <=512 MiB. These load/save budgets describe deliberate document
  operations, not a promised frame-time budget for synchronous saving.
- Retain raw per-frame samples, stage means, file size and measured peak memory.

Synthetic event-to-offscreen-display timings do not measure physical input-to-photon
latency. Native visible-window scroll/drag/paint checks are a separate required gate;
report their evidence and any remaining limitation explicitly.

## September 13 results

Reference: Apple M3 Pro, 18 GiB RAM, macOS 26.6.2 (25G83), arm64 Release
`-O3 -DNDEBUG`, Ant-only rework configuration. Desktop apps remained open; the measured
runs were serial, without a concurrent build or visible Workbench. See
[evidence and machine details](evidence/r6-performance/machine.txt).

All three repetitions passed. Values below are **p95 milliseconds**, with each run kept
independently rather than pooling samples or reporting only the fastest run.

| Fixture | Run 1 | Run 2 | Run 3 | Gate |
| --- | ---: | ---: | ---: | ---: |
| Paused selected Inspector | 1.940 | 2.263 | 2.234 | 16.67 |
| Paused Inspector scrolling | 1.848 | 1.887 | 1.945 | 16.67 |
| Paused colony drag, including input and redraw | 3.419 | 3.078 | 2.976 | 16.67 |
| Sustained 1 colony | 5.275 | 5.168 | 5.058 | 16.67 |
| Sustained 3 colonies | 10.969 | 11.243 | 11.060 | 16.67 |
| Sustained 5 colonies | 16.407 | 16.456 | 16.560 | 16.67 |
| 384×216 map paint | 1.189 | 1.155 | 1.147 | 16.67 |
| 1024×1024 map paint | 8.744 | 9.088 | 8.904 | 16.67 |

Live populations after each sustained fixture were **999 / 3023 / 4832**, identical
across runs. Births and deaths remain enabled. Placement uses the historical PNG to
choose the same coordinates; simulation loads the editor-authored `Main.pftilemap`.
The initial UI benchmark's separate startup-playing case also passes, but it is not
used as evidence for sustained 1/3/5-colony performance.

Worst recorded input-handler p95: scroll **0.0224 ms**, drag **0.3854 ms**, map painting
**0.0155 ms**. Drag dispatches press/move/release through `WorkbenchInput` and fails
if the colony does not move. Map painting fails if either event is not consumed.
Frame timings include that dispatch and the ensuing UI/map/render work.

| Map workload | Worst import/open/first frame | Worst save/reimport | Worst peak RSS |
| --- | ---: | ---: | ---: |
| 384×216, 166,182-byte source | 30.16 ms | 13.54 ms | 121.05 MiB |
| 1024×1024, 2,098,256-byte source | 194.63 ms | 156.98 ms | 466.67 MiB |
| Budget | 1000 ms | 1000 ms | 512 MiB |

The five-colony result has **0.11 ms headroom** at p95 on this machine. This is a
reference-fixture pass, not a guarantee for heavier scenes, background workloads,
other hardware, every individual frame, or GPU presentation latency. The runner is
opt-in so ordinary correctness CI does not acquire a machine-dependent time limit.

## Changes and regression protection

The initial large-map measurement met frame time but failed memory: **621.84 MiB**.
The first exact-size buffer-reuse attempt made peak memory worse (**1138.08 MiB**):
small paint growth repeatedly reserved a slightly larger multi-megabyte buffer.
The final implementation grows capacity with spare room and reuses that storage.
All exploratory logs remain in the evidence directory, including that rejected result.

- `TilemapChunkCache::Flatten(output)` retains host capacity across paint/erase and
  flattens cached chunks into it. Capacity grows geometrically when needed.
- `TilemapAssetModule` reuses runtime geometry storage on reimport/revision changes.
- `ProjectSession` reuses editor geometry and avoids a temporary full-map copy when
  Playground bounds fully contain the map. Partially clipped maps retain clipping.
- Geometry regression checks verify unchanged positions/colors/UVs, new painted cells,
  erased geometry removal and retained buffer storage. Existing transformed/bounded
  map tests protect clipping and material behavior.
- No simulation timestep, agent population, worker policy, render quality, collision
  semantics or Ant source architecture changed to achieve these numbers. Ant Source
  still contains no direct SFML/backend references. Plugin ABI remains 10.

## Native interaction verification

The Release editor opened a disposable Ant project. Real mouse input scrolled the
Inspector to Colony State/History, dragged the colony from (96,108) to (116,97), painted
a visible rectangle and undid it. See the [interaction record and screenshots](evidence/r6-performance/native-checks.md).
This completes the visible-interaction check. The captures verify resulting visual
changes; they do **not** establish measured physical input-to-photon latency. No such
measurement is inferred from automation call duration or offscreen `display()` time.

## Reproduction

Build Release with `PIPEFRAME_ANT_REWORK_ONLY=ON`, stop competing builds/native editor
instances, and run from the repository root:

```sh
python3 apps/SimulationWorkbench/tests/run_r6_performance.py --build cmake-build-release --output /tmp/r6-performance-new-run
```

The output directory must not already exist. The runner executes three serial
repetitions of `--ant-scale-benchmark`, `--ant-ui-benchmark`, `--ant-drag-benchmark`
and `--environment-benchmark`; each accepts an optional CSV output directory.
It retains stdout/stderr, every measured frame, exit codes and executable hash.
Exit 1 means a correctness/fixture error; benchmark exit 2 means a performance gate
failed. The Python runner returns failure for either condition or missing samples.
Do not run these fixtures concurrently: native tests share temporary fixture paths.

[Saved run 1](evidence/r6-performance/run-1/),
[run 2](evidence/r6-performance/run-2/),
[run 3](evidence/r6-performance/run-3/), and
[source/binary hashes](evidence/r6-performance/final-sha256.json).

## Correctness verification

- Full Debug suite: **76/76 passed**, plus the final affected learning-test recheck.
- Full final Release suite: **76/76 passed**.
- Release initially failed `LearningExperimentRegression`: setup calls inside `assert`
  disappeared under `NDEBUG`. The test now uses always-active checks, so setup and
  validation execute in both modes. This changes the test harness, not engine behavior.
  The failing and final suite logs are both retained.
- Release/Debug builds passed; `git diff --check` passed; Ant Source backend scan was empty.

[Debug log](evidence/r6-performance/debug-tests.txt),
[final Debug learning check](evidence/r6-performance/debug-learning-final.txt),
[final Release log](evidence/r6-performance/release-tests-final.txt).

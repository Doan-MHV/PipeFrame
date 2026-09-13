# Milestone 16 acceptance

Completed 2026-09-09 on macOS arm64. The full Debug build succeeds and **56/56
CTest tests pass** (159.61 seconds). Focused Release validation passes **5/5**
(8.48 seconds): runtime resolution, UI hardening, ThermalLab goldens, SailBoat
10,000-boat performance, and dashboard stress profiling. Release validation is
focused; the complete 56-test result is from Debug. `git diff --check` passes.

The extraction places generic layout, state bindings, controls, motion, overlays,
charts, tables, network visualization, and dashboard composition in the engine.
Workbench, Ant, and SailBoat supply application state and commands. Simulation
algorithms, persistence, sampling, and world rendering remain project-owned.

## Acceptance coverage

| Requirement | Evidence |
| --- | --- |
| Constraints, binding lifetime, focus, hit testing, capture | `UIFrameworkRegression`, `UIHardeningRegression` |
| Layout invalidation and independent animation clocks | No-op arrangement, sibling reflow, partitioned time, paused motion regressions |
| Reusable visualization | `ChartRegression`, `NetworkViewRegression`, `TableViewRegression` |
| Drawers, overlays, menus, keyboard focus, transport | `UIGalleryAcceptance`, `WorkbenchUIAcceptance`, `SimulationDashboardAcceptance` |
| Live world edits and right-click ownership | Workbench/runtime interaction acceptance, Ant painting/capture and SailBoat race placement |
| Multiple sizes and UI scales | `ThermalLabGoldenAcceptance`: 640×800 at 1×, 1200×800 at 1×, 1200×1200 at 1.5× |
| Public API adoption | Standalone `examples/ThermalLab`, [component guide](UI_COMPONENT_API.md) |
| Stress and retained allocation footprint | `UIDashboardStressProfile`: 10,000/100,000 ants and 10,000 boats, all four pages |
| Milestones 14/15 behavior | Full Ant and SailBoat regression suites, including stress, NEAT, training and persistence |

The three ThermalLab golden images were visually reviewed. Normal tests compare
the deterministic paused fixture against those files without rewriting them.
See the component guide for tolerances and explicit baseline regeneration.

## Logic and build fixes

NEAT connection mutation previously stopped after 64 random attempts, even when
a legal edge remained. A legal-pair fallback now prevents false failure when
seeding large populations. Regression coverage seeds 20,000 empty genomes and
checks both legal-edge exhaustion and the saturated result.

The foundation test now checks required environment properties instead of a
stale exact count, and resolves its assets from the actual example directory.
These address the two failures recorded at the 16G checkpoint.

Runtime libraries now build under `Build/Debug` or `Build/Release`. The loader
prefers its matching configuration and retains legacy flat-path compatibility.
This prevents one build configuration from overwriting another's plugins and
loading incompatible SFML variants together. Both configurations have a path
resolution regression. Runtime clients must rebuild against the updated public
runtime interface; the repository build does this.

Profiling exposed repeated label wrapping during geometry notifications. Labels
now retain wrapped text while content, character size, wrapping mode and usable
width are unchanged. Positioning still updates normally. A narrow/wide/narrow
pixel regression and the golden comparisons protect cache invalidation.

## Measurement method

The dashboard fixture initializes real populations, advances a simulation tick,
renders the world, and warms each page before measurement. It averages 24 layout
passes with alternating widths, 500 pointer-move dispatches, 24 render submissions,
and 24 runtime dashboard refresh/render calls. Reported milliseconds are CPU
wall time per call; render submission is not GPU completion latency.

macOS allocator snapshots measure process-wide live blocks and bytes before and
after each page, not total allocation churn or UI-only allocation calls. The
allocator high-water field is platform-dependent and is not used for conclusions.
Tests reject retained widget growth, more than 32 MiB retained heap growth per
page, or broad latency regressions (100 ms layout/render/refresh, 20 ms dispatch).
These are regression guards, not a promised frame-rate budget on every machine.

| Measurement across all pages | Debug | Release |
| --- | --- | --- |
| Layout, ms/call | 0.069–0.243 | 0.0034–0.0187 |
| Pointer dispatch, ms/call | 0.010–0.0154 | 0.00052–0.00095 |
| Render submission, ms/call | 0.048–0.417 | 0.033–0.726 |
| Runtime refresh/render, ms/call | 0.128–3.633 | 0.056–0.295 |
| Largest positive live heap delta per page | 202,528 bytes | 445,744 bytes |

Retained trees stay at 96 widgets for Ant and 89 for SailBoat throughout all
pages and populations. Before the wrapped-label cache, Debug layout averaged
1.001–5.617 ms/call; afterward it averages 0.069–0.243 ms/call. Runtime refresh
costs did not materially improve, so this is specifically a layout improvement.

Raw captures: [Debug before cache](benchmarks/milestone16-debug-ui-before-cache.csv),
[Debug after cache](benchmarks/milestone16-debug-ui.csv), and
[Release](benchmarks/milestone16-release-ui.csv). Timings are local macOS
measurements and should be rerun on target hardware before drawing platform-wide
conclusions.

## Reproduction

```sh
cmake --build cmake-build-debug -j 4
ctest --test-dir cmake-build-debug --output-on-failure
cmake --build cmake-build-release --target UIDashboardProfile ThermalLab UIHardeningTests RuntimeLibraryResolutionTests SailBoatPerformanceTests -j 4
ctest --test-dir cmake-build-release -R 'UIDashboardStressProfile|ThermalLabGoldenAcceptance|UIHardeningRegression|RuntimeLibraryResolutionRegression|SailBoatPerformanceRegression' --output-on-failure
```

The profiler writes `ui-dashboard-profile.csv` in the corresponding build
directory. Native graphics checks require an available graphics session.

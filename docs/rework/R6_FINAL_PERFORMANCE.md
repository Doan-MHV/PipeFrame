# R6 final performance verification

September 13, 2026. **Complete: R6 verified.** The previous five-colony miss remains
recorded in [package 8's report](R6_BLANK_PROJECT_ACCEPTANCE.md#final-verification).

## Change

The shared `SolveCircleContacts` previously constructed a new spatial index, cell storage,
and entry vector on every call. The Ant map uses 384×216 cells for the contact broadphase.
That allocation was discarded every physics tick even when its bounds and cell size
were unchanged.

[CircleContactWorkspace](../../engine/include/PipeFrame/Physics/Physics2D.h) now retains
those buffers between solves. Each call still rebuilds current positions and membership;
no body pointer or collision result survives a tick. A bounds or maximum-radius change
reinitializes the grid. The one-off overload retains its original call signature.
The contact candidate order, numerical solver, spatial cell size and collision policy
are unchanged.

Both [PhysicsWorld2D](../../engine/include/PipeFrame/Physics/PhysicsWorld2D.h) and
[Ant ContactSolver](../../examples/AntSimulation/Source/World/Physics/ContactSolver.cpp)
own a separate workspace. It is neither global nor shared across independently executing
worlds. The generic physics world therefore benefits without project-specific code.
Plugin ABI is **12**, because PhysicsWorld2D's layout changed. Rebuild existing plugins.

[PhysicsEnvironmentTests](../../engine/tests/PhysicsEnvironmentTests.cpp) compares exact
ordered contacts, normals, penetration and solved/current previous positions against a
fresh workspace over changing body counts, order, radii, bounds and empty ticks. These
new checks remain active in Release. Existing Ant parity, movement/contact, stress,
editor, public header and blank-project checks are retained.

## Measurement rules

Use the existing three-run runner and unchanged 16.67 ms p95 target, 1440×900 target,
seed 1, 1000 configured ants per colony, 1/60 fixed timestep and 1200 warmup ticks.
Preserve 1/3/5-colony live counts, map workloads and all raw samples. Do not change
worker count, simulation quality, population, gate thresholds or fixtures. Run serially
with no build/test or visible Workbench competing with the measurement.

## Final results

All 12 fixture executions across three repetitions passed, with raw samples present.
Reference remains Apple M3 Pro, 18 GiB, macOS 26.6.2, arm64 Release. Both builds and
both complete test suites had finished before these measurements. No failing run was
replaced and no additional attempt was needed after this code change.

| Fixture, p95 ms | Run 1 | Run 2 | Run 3 | Budget |
| --- | ---: | ---: | ---: | ---: |
| 1 colony | 5.126 | 5.096 | 4.988 | 16.67 |
| 3 colonies | 11.122 | 11.143 | 10.902 | 16.67 |
| 5 colonies | 16.355 | 16.185 | 16.491 | 16.67 |
| Selected Inspector | 2.079 | 1.976 | 2.070 | 16.67 |
| Inspector scrolling | 1.952 | 1.884 | 1.923 | 16.67 |
| Colony drag | 3.105 | 3.183 | 3.227 | 16.67 |
| 384×216 map paint | 1.147 | 1.131 | 1.139 | 16.67 |
| 1024×1024 map paint | 9.192 | 9.169 | 9.024 | 16.67 |

Worst five-colony p95 is **16.491 ms**, with **0.179 ms headroom**. This satisfies the
reference gate; the margin remains narrow and does not guarantee 60 FPS on other
hardware, heavier scenes or arbitrary background load. Live populations remain
**999 / 3023 / 4832**. All map input/load/save gates pass; worst peak RSS is
**467.406 MiB**, below the 512 MiB budget. Physical input-to-photon latency is
not measured by these offscreen fixtures. Prior native scroll/drag/paint checks are
retained; this change does not modify editor interaction or rendering code.

- Final Debug build and **77/77 tests passed**: [build](evidence/r6-performance-final/debug-build.txt), [tests](evidence/r6-performance-final/debug-tests.txt).
- Final Release build and **77/77 tests passed**: [build](evidence/r6-performance-final/release-build.txt), [tests](evidence/r6-performance-final/release-tests.txt).
- [Runner output](evidence/r6-performance-final/runner.txt), [machine/binary metadata and fixture statuses](evidence/r6-performance-final/measured/results.json).
- Raw [run 1](evidence/r6-performance-final/measured/run-1/), [run 2](evidence/r6-performance-final/measured/run-2/), [run 3](evidence/r6-performance-final/measured/run-3/).
- [Source and runtime/benchmark hashes](evidence/r6-performance-final/sha256.json). This identifies tested working-tree content, not a newly committed clean revision.
- `git diff --check` passes; Ant Source has no SFML, `sf::`, or backend references.

All eight R6 packages now have passing required verification. **R6 is complete.**
R7 remains the separate removal/documentation/completion audit; Milestone 19 robotics
work is not silently included in this completion claim.


# PipeFrame data, spatial, time, and task services

Milestone 17C moves general runtime mechanisms out of the simulation projects and
into `PipeFrame::Foundation`. These headers have no SFML dependency and work in a
headless target.

| Service | Public header | Contract |
|---|---|---|
| Bounded grid | `PipeFrame/Data/Grid2D.h` | Row-major typed cells, checked access, optional lookup, fill, resize, bounds and coordinate clamping. |
| Spatial index | `PipeFrame/Spatial/UniformSpatialIndex.h` | IDs and positions only; supports rebuild, insert, update, remove, cell spans and exact radius queries. Domain bodies and collision rules stay with the application. |
| Grid raycast | `PipeFrame/Spatial/GridRaycast.h` | Normalized DDA traversal over caller-defined blocked cells, returning cell, distance and entry normal. |
| Stable storage | `PipeFrame/Data/GenerationalStore.h` | Typed handles reject stale generations while slots are reused without exposing addresses. |
| Random streams | `PipeFrame/Core/DeterministicRandom.h` | Repeatable scalar generation and stream IDs derived from the root seed, independent of parent consumption order. |
| Signals | `PipeFrame/Core/Signal.h` | Token-based subscribe, disconnect and mutation-safe emission. |
| Simulation clock | `PipeFrame/Core/FixedStepScheduler.h` | Accumulated fixed steps, a per-frame catch-up limit and interpolation alpha. |
| Behaviour coroutines | `PipeFrame/Core/Coroutine.h` | Scene-owned cooperative fixed-time tasks, cancellation and teardown; distinct from background futures. |
| Async tasks | `PipeFrame/Core/AsyncTask.h` | Explicit asynchronous launch with future-based results and exception propagation. |
| Thread pool | `PipeFrame/Core/ThreadPool.h` | Future-returning submission and condition-variable workers without busy waiting. |
| Profiling | `PipeFrame/Core/Profiler.h` | Thread-safe named aggregates and RAII elapsed-time scopes. |

Current Ant consumers live under `Source/World/Physics` and `Source/World/Runtime`.
They use PipeFrame math and query types; Ant source has no SFML adapter or public backend
include exception. The old Milestone 17 compatibility folder paths have been retired.
Scene ownership, tasks and events are documented in the
[Behaviour lifecycle guide](rework/BEHAVIOUR_LIFECYCLE.md); the full source map is in the
[Ant guide](../examples/AntSimulation/README.md).

`SpatialServicesRegression` is deliberately unrelated to ants. It compares index
queries with brute force at minimum and maximum world boundaries, then covers
insert, cross-cell update, removal and rebuild. It also exercises every core
service. Existing Ant contact, avoidance and marker-sampler tests verify the real
consumer path, while `PipeFrameDependencyLint` rejects an SFML leak in these
headers.

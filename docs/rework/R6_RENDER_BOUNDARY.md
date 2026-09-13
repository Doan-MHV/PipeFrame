# R6 render boundary migration

The user approved the core migration on September 12, 2026. Camera2D and
RenderContext now expose only PipeFrame types; RenderSurface supplies the rendering
surface interface. Native window/texture construction, view conversion and target
access live in Backend/SFML/RenderContextAdapter.

The window or texture must outlive its context. Camera center, base size, zoom,
normalized viewport, pixel/world mapping and screen/world passes retain their
previous semantics. Offscreen contexts reject window access with an explicit error.

The engine application, camera controller, Workbench host, dashboard host overloads,
BasicSimulation, Ant benchmark and native tests use the new boundary. Ant simulation
algorithms, World composition and schemas are unchanged. SailBoat is excluded from
this rework and was neither migrated nor tested.

## Project compatibility

Plugin ABI is now **4** because the RenderContext layout changed. Rebuild existing
project runtime libraries with this SDK. The host rejects ABI 3 before plugin
registration or rendering; a rejected reload keeps the current runtime alive. Scene
and component serialization versions are unchanged.

## Verification

Final Debug suite: **69/69 passed** in 96.17 seconds. Debug and Release builds pass.

- Debug and Release builds include a render-header object compiled without Engine's
  transitive native include paths. Ant Plugin.cpp also passes a standalone syntax
  check with only engine/include and Ant Source.
- RenderContextRegression checks normalized viewport bounds, forward/inverse mapping,
  zoom, camera movement, screen resize, target resize, world/screen pass selection,
  offscreen pixel submission and invalid surface/window use.
- WorkbenchUIAcceptance checks ABI rejection on load/reload, native input, camera
  interactions, Inspector scrolling, source forms and build/cancel button routing.
- ProjectBuildAcceptance exercises generation, configure/build/reload, component and
  behaviour attachment, validation, running, prefab/save/reopen and failure recovery.
- Evidence and final test/benchmark output live in `evidence/r6-render-boundary/`.

## Release measurements after migration

Same map/seed/populations and offscreen fixture as the preceding R6 build checkpoint:

| Case | Median ms | p95 ms |
| --- | ---: | ---: |
| Inspector scrolling, paused | 1.64 | 2.02 |
| 1 colony, 1000 live ants | 4.83 | 5.75 |
| 3 colonies, 3023 live ants | 10.44 | 11.48 |
| 5 colonies, 4764 live ants | 15.62 | 17.00 |

The five-colony p95 exceeds the 16.67 ms target, unlike the previous run's 15.25 ms.
This is not evidence that the performance gate is universally closed, nor does a single
before/after run isolate the cause of the difference. Keep this tail-latency concern
open. These are software timings, not physical input-to-display measurements.

## Remaining R6 scope

This closes the approved camera/context migration, not every native UI interface.
SimulationDashboard, Widget/ViewPanel and 11 Workbench Editor headers still expose
native types or include native implementations. The exact Editor header inventory is
saved in `evidence/r6-render-boundary/remaining-editor-headers.txt`. Their public
interfaces still need conversion to neutral UI hosting contracts with implementation
behind engine adapters. Do not mark the broader R6 gate complete until that is done.

Offscreen frame-time results measure software execution, not physical pointer-to-display
latency. They must not be presented as universal hardware performance guarantees.

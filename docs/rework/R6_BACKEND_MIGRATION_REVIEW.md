# R6 core render boundary — approved September 12, 2026

Automatic approval review rejected the unexecuted migration command on September 12,
2026. It cited the broad effect of rewriting camera/render-context APIs and dependent
application/example call sites. No part of that rejected command was applied.

The user explicitly approved this proposal with “yes”. The camera/context migration
is now implemented; verification is recorded in R6_RENDER_BOUNDARY.md.

## Approved API proposal

1. Camera2D stores PipeFrame Vector2f/Rectanglef values, including its normalized viewport.
   Preserve the current center, base-size and zoom semantics. Remove the native View member
   and GetView() from the public camera contract.
2. RenderContext accepts a shared RenderSurface implementation. RenderSurface defines
   neutral canvas submission, surface dimensions, viewport bounds, pixel/world mapping,
   and world/screen pass selection. The context retains its camera and exposes only
   PipeFrame math/render types.
3. An engine Backend/SFML/RenderContextAdapter constructs a context from a native window or
   render texture. Its implementation creates native views and resolves native targets.
   Native widget/window hosts access their target through this explicit adapter.
4. Migrate known dependent context/camera calls in the engine's window host, Workbench,
   tests and the currently built Basic/Thermal/UI/benchmark consumers. Do not rewrite
   unrelated methods with matching names, Ant simulation algorithms, or SailBoat.
5. Preserve the existing Ant world modules, component schema convention and declarative
   View API. This is a graphics boundary migration, not an ECS or physics redesign.

## Files and scope

Core: engine/include/PipeFrame/Render/{Camera2D,RenderContext}.h,
engine/src/Render/Camera2D.cpp; new native adapter header/implementation beneath
engine/{include/PipeFrame,src}/Backend/SFML.

Dependent callers: engine/src/Core/Application.cpp, the native camera controller,
SimulationDashboard's host overloads, Workbench's native rendering/input/layout code,
render-context tests and the built reference/benchmark window consumers. Inventory actual
calls before each edit; avoid blanket replacement of unrelated GetTarget methods.

This does not by itself remove all older native Widget/ViewPanel public APIs. Those
remaining interfaces must still be audited against the R6/R7 boundary criteria.

## Verification required

- Compile the public camera/render-context headers and Ant Plugin.cpp without native include paths.
- Verify camera transforms, normalized viewports, resize, offscreen rendering and input mapping.
- Run the engine/editor/Ant tests and native Inspector, toolbar and dashboard interaction checks.
- Re-render existing UI evidence and repeat the optimized frame-time benchmarks.
- Keep the current native declarations until replacement callers compile, then remove the
  superseded API together; do not leave compatibility aliases that disguise native types.

The independent configure/build/reload work is implemented and can be verified without
this API migration. R6 must not be marked complete while its remaining boundary gates
are still pending.

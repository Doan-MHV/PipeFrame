# R6 package 1 — neutral dashboard and camera controller

Historical dashboard slice, September 12, 2026. For the final package-1 boundary and verification, see R6_PUBLIC_UI_ISOLATION.md. Overall R6 remains open.

## Actual changes

SimulationDashboard is now an opaque public declarative facade. It accepts neutral
resource/font handles, View builders, ViewTheme, InputEvent and RenderContext. Native
Widget/EdgeDrawer/Font/RenderTarget APIs are no longer part of this authoring header.
Ant uses SetDrawerOpen rather than reaching into a native drawer object.

The previous widget host lives under Backend/SFML/SimulationDashboardHost.h. Native
engine UI tests inspect it through DashboardAccess; Ant Source never includes that
adapter. ThemeTokens supplies one set of defaults for the neutral ViewTheme and legacy
native UITheme, with conversion inside the engine. Invalid font handles produce an
exception instead of dereferencing a missing font. Resources must outlive the dashboard.

CameraController2D now consumes PipeFrame InputEvent. It owns Space modifier and drag
state, has explicit cancellation, and preserves the grabbed world point while panning
or zooming. Focus loss cancels the interaction. Workbench converts native events at its
host boundary and uses the same tracked pan modifier for selection routing. The controller
contains no native keyboard polling. Horizontal wheel input does not trigger zoom.

Plugin ABI is **5**. Rebuild existing plugins; the host rejects the preceding ABI before
registration. Native UITheme's type and dashboard/controller contracts changed, so this
must not be loaded as a binary-compatible replacement for ABI 4.

## Developer example

```cpp
pipeframe::ui::ViewTheme theme;
theme.accent = {242, 79, 112};
SimulationDashboard dashboard(resources, fontHandle, "My simulation", theme);
dashboard.SetTabbedDrawerVisible(false);
dashboard.AddViewDrawer(DrawerEdge::Left, "TOOLS", 300, 380,
    DrawerAnchor::Center, [this] { return BuildToolsView(); });
dashboard.SetDrawerOpen(0, true);
// Host integration: Layout(context), HandleEvent(input), Render(context).
// Destroy the dashboard before destroying its resources or callback owner.
```

Actual Ant consumer: examples/AntSimulation/Source/Runtime/AntUIHost.cpp.
Ant's declarative content remains in Source/Editor/AntDashboard.cpp.

## Verification and remaining scope

Debug suite: **69/69 passed** in 93.59 seconds. The native Ant dashboard interaction
run passed and saved six panels at 1000- and 800-pixel widths; the 1000-pixel editor
panel was visually inspected. Debug/Release builds and both neutral compile targets pass.

RenderPublicHeadersTests includes dashboard, theme and camera-controller public APIs
without linking Engine or inheriting its native include paths. AntUIBoundaryCompile
compiles actual AntUIHost.cpp and AntDashboard.cpp using only engine/include and Ant
Source. Commands and final test/native evidence are saved in evidence/r6-ui-boundary/.

RenderContextRegression covers neutral middle/Space-left pan, release/focus state,
zoom anchoring and horizontal wheel handling. AntDashboardAcceptance covers font-handle
validation plus real drawer controls, selection, refresh and pointer routing. Workbench
acceptance retains load/reload compatibility checks against the previous ABI.

Still open: native Widget/ViewPanel, Controls and Workbench panel/host public contracts.
The native host has been isolated from Ant authoring; legacy UI migration is not claimed
complete. Playground/assets/tools, geometry/Ant environment migration and performance
acceptance remain the other R6 execution packages. This slice does not implement them.

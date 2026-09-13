# R6 native build workflow

Implemented September 12, 2026. This document covers the build/authoring work; it does
not mark all of R6 complete. The approved core render-boundary migration is implemented. Remaining work is tracked
in [R6_EXECUTION_PLAN.md](R6_EXECUTION_PLAN.md).

## Editor use

1. Open a project. Stop its simulation before building.
2. Use Hierarchy + → NEW ENTITY / COMPONENT / BEHAVIOUR to generate a class. Component
   fields and validation belong to Component::Schema() in the generated component header.
   GeneratedRegistration.h is maintained automatically; do not edit it manually.
3. Click BUILD & RELOAD in the toolbar. Configure/build output appears in the Console.
   While running, the same toolbar action becomes CANCEL BUILD.
4. On success, the editor activates the compiled runtime and updates project.pipeframe
   with its runtime path. The newly registered entity is available in the creation picker.
5. Add generated settings/behaviour through the Inspector's attachment picker. Author
   properties, create a prefab, save, and reopen as with other scene objects.

Compiler failure/cancellation does not unload the running plugin. Invalid library activation
is rejected by the existing staged reload path. The Console records the outcome; the full
compiler log is in .pipeframe/build.log. Project switching during a build does not load its
output into another project's session. A successful build still requires the original
project to be active for automatic activation.

## Build configuration

Generated projects receive Config/Build.pipeframe:

```
PIPEFRAME_BUILD 1
"MyProjectRuntime" standalone
```

The generated target name is recorded when scaffolding; it is not inferred from a library
search. Standalone projects configure under .pipeframe/build/<configuration> and link
against the engine library already built for the current editor. The current SDK include
path is supplied explicitly, so these generated plugins need no native graphics headers.
They can also use the earlier SDK/installed-package CMake paths outside the editor.

Ant uses target AntSimulationRuntime with mode workspace. Its build uses the editor's
configured repository build directory. The editor records its CMake executable, SDK path,
build directory and configuration at build time. This implementation targets the current
local development environment; moving an editor binary away from its SDK/build tree needs
an installed-SDK configuration flow.

The engine ProcessTask runs argument vectors directly, without shell interpolation, on
macOS/Linux. It reports stage/output/exit status asynchronously, stops subsequent stages
when one fails, and terminates the process group on cancellation. Windows reports an
explicit unsupported-process-execution error; a Windows process adapter is not implemented.

## Verification

ProcessTaskRegression covers literal arguments with shell punctuation, sequential output,
nonzero exit, skipped later stages, duplicate starts, cancellation and missing executables.

ProjectBuildAcceptance creates a project in a path containing spaces, generates SoldierAnt,
SoldierSettings and SoldierBehaviour, compiles through the same ProjectBuildPlan/ProcessTask
used by the editor, activates the library, validates/edits its exposed speed, executes the
behaviour, creates a prefab and reopens from the persisted manifest. It injects a compiler
error and an invalid library, checks that the loaded runtime/scene survive, and rebuilds
successfully after correcting source.

WorkbenchUIAcceptance clicks the real BUILD & RELOAD and CANCEL BUILD controls. The native
button routing check and the actual build/session integration check are separate tests;
they do not simulate editing arbitrary C++ source inside the Workbench UI.

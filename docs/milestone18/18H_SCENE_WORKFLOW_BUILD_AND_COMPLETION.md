# 18H — Scene workflow, build pipeline, and completion

Status: **in progress (2026-09-11)**.

## Implemented checkpoint

- New Project creates the standard Config, Assets, Scenes, Source, Tests, cache,
  manifest, starter scene, plugin entry point, runtime composition root, and
  CMake target.
- Add Component, System, Runtime Module, and Editor Extension generate code
  against public PipeFrame contracts and update `Modules.pfconfig` without a
  manual metadata edit. Runtime modules implement `pipeframe::RuntimeModule`.
- Create Object Type writes a data-only `.pftype` archetype. A project without a
  compiled runtime discovers it immediately and Create Object instantiates it
  with common Transform and Identity components.
- Generated component metadata is discovered by the Inspector before a native
  runtime is compiled. Transform, Physics Body, Collider, and Renderer remain
  common engine-owned component schemas.
- The project validator checks the required structure, registered module files,
  object-type format and duplicate component IDs, Workbench coupling, SFML
  symbols, and PipeFrame backend imports.
- Generated CMake discovers project sources automatically, accepts an installed
  PipeFrame package or an editor-supplied SDK root, and emits the loadable
  runtime under the project's `Build/<configuration>` directory.
- The backend-neutral robotics-readiness regression assembles a chassis, two
  wheels, distance sensor, light, and obstacle; parents parts; attaches physics
  and authoring components; wires mechanical and signal endpoints; and verifies
  scene save/reload.

## Completion blockers

18H cannot be marked complete while 18G's backend boundary is open. Ant has 23
and SailBoat has 5 rendering/dashboard/runtime source files that still name
SFML or `PipeFrame/Backend`. BasicSimulation also retains its old backend-bound
implementation. These files must move to PipeFrame render submissions, neutral
input/camera APIs, and engine-owned dashboard presentation before the zero-SFML
conformance gate can replace the temporary allowlist.

The remaining 18H work is the editor-facing build/package/run controller with
navigable diagnostics, automatic successful-build reload, the full
Debug/Release/sanitizer matrix, and final reviewed UX evidence. A successful
test run alone does not override these acceptance conditions.

## Verification

The Debug tree builds successfully and all 70 registered tests pass, including
project generation, data-only object creation, generated component discovery,
reference-project conformance, UI acceptance, Ant parity/stress/performance,
and SailBoat behavior/performance coverage.

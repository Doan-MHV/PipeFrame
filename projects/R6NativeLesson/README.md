# R6 native editor walkthrough

This is the saved project created with New Project during R6 package 8 acceptance.
Its Playground map was painted using native Line gestures, undo/redo, and Save Map.
Probe was generated, placed, and given its components through the Inspector. Its settings
and behaviour are the compiled examples in `docs/tutorials/blank-project` in the SDK.

Open `project.pipeframe` in SimulationWorkbench, Build & Reload for the current host,
then Play. The small visible box moves right and stops before the first wall. Pause and
Reset restore the authored setup. Probe Settings shows live ray distance and blocked state.

This native walkthrough contains Playground and Probe. The broader automated acceptance
also creates materials, Spawn/Goal/Obstacle markers, and a prefab instance, then verifies
save/reopen/reload. To retain that full project, run the built
`apps/SimulationWorkbench/BlankProjectAcceptanceTests` with a new output directory.
See `docs/tutorials/blank-project/README.md` for the complete lesson and API links.

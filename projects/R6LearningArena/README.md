# Complete blank-project learning arena

Generated and verified by BlankProjectAcceptanceTests on September 13, 2026.
Open project.pipeframe with the current SimulationWorkbench and Build & Reload.
The scene contains a Playground with an editor-authored maze and material, Spawn/Goal
markers, a separate Obstacle, a Probe, and a Probe prefab instance. Play moves the two
Probes toward their obstacles; Pause/Reset and live Probe Settings demonstrate lifecycle
and geometric query access. The markers do not implement navigation.

Source/Components/ProbeSettings.h and Source/Behaviours/ProbeBehaviour.h are the exact
compiled tutorial examples. The engine owns generic movement, collisions, scene queries,
serialization, asset editing, and Inspector generation. Full instructions and API links:
docs/tutorials/blank-project/README.md in the PipeFrame SDK repository.

The fixture used the editor's ProjectSession/asset editors and input gesture controller;
it did not generate a map PNG or hand-write scene data. Separate native interaction
captures are in docs/rework/evidence/r6-blank-project in the SDK repository.

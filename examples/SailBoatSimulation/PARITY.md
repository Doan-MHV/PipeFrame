# SailBoatPezza parity plan

The reference implementation is `SailBoatPezza`. `SailBoatSimulation` remains an independent example project: PipeFrame owns the editor/runtime contract, while this folder owns all sailing, race, training, NEAT, rendering, audio, and project UI logic.

## Reference audit

- Simulation: a population of 1,000 boats by default, wind-relative polar speed, rudder control, trajectories, crash/finish state, and fixed-step updates.
- Race: one directed start, an ordered list of directed marks, and one finish gate. The original editor creates these with right-click phases and can clear, save, and load a race.
- Training: four neural inputs, one output, NEAT genome/network generation, evaluation, score calculation, elite selection, mutation, repeated explorations, async blocking training, and genome/configuration persistence.
- Rendering: tiled animated water, parallax seascape and depth texture, start/finish/waypoint graphics, labels, best boat, ghost boats, trajectories, highlighting, and optional world rendering.
- Project UI: race editor, settings, play/pause and full-speed control, training timer/results, selected boat gauges, network graph, and wind direction/speed HUD.
- Audio: bubble sound effect triggered by simulation events.
- Reference keyboard controls: Space pause, S full speed, Tab reset camera, F follow, U UI visibility, D world rendering, R restart exploration, and B best-only rendering.

## Milestone 15 delivery slices

- [x] **15A — standalone project foundation:** runtime plugin, project manifest, initial scene, explicit scene object types, validated configuration, robust race geometry, viewport selection, original assets, and regression tests.
- [x] **15B — boat and environment simulation:** Pezza polar table, wind-relative speed, rudder integration, trajectory history, boundaries, and deterministic reset.
- [x] **15C — task and race progression:** neural inputs, mark ordering/projection, scoring, finish detection, race time, and distance.
- [x] **15D — NEAT core:** activation, DAG/genome/network, generator, mutation, selection, serialization, and deterministic tests.
- [x] **15E — population training:** agents, iterations, elite evolution, explorations, async training, persistence, and stress tests.
- [x] **15F — Pezza visual parity:** animated water/depth, boats and ghosts, marks, trajectory, highlighting, labels, and effects.
- [x] **15G — project editor and HUD parity:** race creation tools, save/load, settings, timer/results, boat inspector, network panel, and wind HUD.
- [x] **15H — audio, performance, and final parity:** sound, batched population rendering, profiling, large-population stress tests, workflow polish, and parity audit.

## 15A object contract

- `sailboat.race-start`: position and rotation define spawn position/heading; direction length is editable.
- `sailboat.finish-line`: position, rotation, and length define the finish gate.
- `sailboat.waypoint`: position, rotation, line length, reach radius, and order define an intermediate mark.
- `sailboat.environment`: world size, wind direction/speed, and water-animation preference.
- `sailboat.training-settings`: population, iteration duration, elite ratio, simulation speed-up, rudder speed, seed offset, and async-training preference.

## 15G editor workflow

- The editor toolbar creates ordered marks and replaces the directed start or finish with two right-clicks: position, then direction/length.
- `CLEAR` removes race marks as one undoable edit. Start, finish, and mark changes use the same document history as inspector edits.
- `SAVE RACE` and `LOAD RACE` use `Assets/Races/waypoints_save.pfrace`. Race files contain only course geometry, so sailing-environment and training settings remain untouched.
- The runtime HUD shows wind, generation time, best training result, the current best boat, and its NEAT network in both editor and simulation workspaces.
- Runtime overlay clicks are isolated from world selection and camera input.

## 15H performance and workflow

- Pezza's `bubble_2.wav` now plays at the original 30% default volume when the leading synchronous boat advances to a new mark. `MARK SOUND` and `AUDIO VOLUME` are editable on the sailing environment.
- Population geometry remains one batched textured draw call and is now culled to the camera before vertex submission. The Metrics overlay reports candidate/visible boats, submitted vertices, simulation time, and render time.
- `SailBoatPerformanceRegression` exercises 10,000 boats for 60 synchronous updates, verifies bounded trajectory storage, and guards against catastrophic performance regressions. The reference Apple Silicon Release run processed about 12.7 million agent-updates per second.
- Workflow shortcuts mirror Pezza where they fit PipeFrame: `S` toggles maximum speed, `F` follows the leading boat, `B` toggles best-only rendering, `D` toggles world rendering, `U` toggles the project HUD, and `R` restarts the exploration.

## Final parity audit

- Milestone 17H corrected training targets to Pezzza's first mark endpoint and
  restored identical neutral genomes for generation zero. Exact score fixtures,
  varied-course/wind generalization, exploration reseeding, async timing,
  periodic saves, and deterministic resume cover the full logic lifecycle.
- Training time is now a persistent top metric. The live activation graph uses
  an independent right-edge drawer and remains available regardless of the
  selected main dashboard tab.

- Simulation, course editing, NEAT training, checkpoints/history, Pezza-derived visuals and water, mark audio, runtime HUDs, and large-population rendering are covered.
- PipeFrame intentionally keeps its global controls: `P` is play/pause, `.` is single-step, and `Tab` changes workspace mode. Pezza used Space and Tab for pause and camera reset, but those keys conflict with PipeFrame's viewport navigation and workspace model.
- Pezza's floating drawers are represented as integrated PipeFrame inspector/HUD panels so authoring controls remain available without hiding editor functionality.

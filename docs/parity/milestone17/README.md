# Milestone 17 parity baseline

This directory is the closure ledger for Milestone 17. Every Ant and SailBoat
reference behavior found in the supplied source trees, project documentation,
or four reference videos has a classification and a later milestone owner.

- [Ant matrix](ANT_MATRIX.md)
- [SailBoat matrix](SAILBOAT_MATRIX.md)
- [Video references and reproducible captures](references/README.md)
- [17I comparison board](COMPARISON_BOARD.md)
- [Temporary SFML migration adapters](SFML_MIGRATION_ALLOWLIST.txt)

## Classification rules

`matched` means the behavior is reachable in the current application, consumes
live state, and has a focused automated check. `incorrect` means an implementation
exists but differs from the reference. `missing` means no usable implementation
was found. `intentional difference` is reserved for a documented PipeFrame host
constraint. No row is left unknown. A source class by itself is not evidence of
a match.

The matrices use these evidence codes:

- `S`: Pezzza source or README inspection.
- `V`: timecoded video observation in the reference index.
- `P`: current PipeFrame source, test, or generated capture.
- `R`: a reproducible runtime check.

## Audit environment and source versions

The baseline was recorded on 2026-09-09 on macOS/Apple Silicon with AppleClang
21 and CMake 4.3. The supplied source directories contain no revision metadata,
so their exact upstream commits cannot be established. They are treated as the
authoritative code snapshot. Videos are authoritative for presentation and for
behavior visible at runtime. When they differ, the matrix says so explicitly.

Both original applications declare SFML through `FetchContent`: Ant pins 3.1.0;
SailBoat tracks `3.0.x`. Clean configure attempts were made in
`/tmp/pipeframe-m17-ant-reference-build` and
`/tmp/pipeframe-m17-sail-reference-build`. Both stopped while cloning SFML
because this environment cannot resolve `github.com`. This is the recorded
platform blocker; no original runtime behavior is inferred from a failed build.
The bundled assets and all source were still audited locally.

## Shared numerical conventions

Both references use single-precision world coordinates, radians internally,
seconds for `dt`, and SFML's screen convention where positive Y points down.
Their main loops pass variable frame time into simulation updates. Ant's old
single-thread path seeds its shared Mersenne Twister with 8; the current Ant
README warns that parallel access makes runs nondeterministic. SailBoat seeds an
exploration with `iteration_exploration + seed_offset`, multiplies frame time by
`simulation_speed_up`, and evaluates until `max_iteration_time`.

Ant defaults include a 384x216 cell world, speed 2, energy 600, follower/explorer
sample counts 64/8, marker spacing 3, marker decay 0.035, 1,000 ants per colony,
and explorer probability 0.1. SailBoat defaults include four inputs, one output,
2,000 agents in code (the supplied config overrides this to 1,000), 900 seconds,
elite ratio 0.2, speed-up 10, seed offset 1, and a 1,600x1,600 configured world.

## Reproduction

Current PipeFrame comparison captures are checked into `current/`:

```sh
cmake-build-debug/examples/SailBoatSimulation/SimulationDashboardTests \
  docs/parity/milestone17/current/ant-1280x720.png ant 1280 3 720
cmake-build-debug/examples/SailBoatSimulation/SimulationDashboardTests \
  docs/parity/milestone17/current/sailboat-1280x720.png boat 1280 4 720
```

The same command produces the retained 1080p, 1440p, and narrow-width states by
changing the width/height arguments. Every run asserts the dashboard contracts
before saving its capture. The pre-17I baseline files remain under `references/`.
Reference-video frames remain
reproducible from the exact timestamp links; browser policy prevented lossless
pixel export during this audit, so the index records the crop requirements and
does not pretend the absent files exist.

## Closure rule

Phases 17B-17I may change a row only with code evidence and the focused check
named in that phase. 17J closes this ledger only when every `missing` and
`incorrect` row becomes `matched`, or a deliberate difference is documented and
approved. This baseline completes the 17A inventory; it does not claim product
parity.

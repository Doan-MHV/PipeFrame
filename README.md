# PipeFrame

PipeFrame is a C++20 engine and desktop editor for 2D simulations. It provides
scenes, entities, component schemas, fixed-step systems, rendering, collision
helpers, and project UI. SFML supplies the native backend.

The Workbench supports scene editing, component inspection, tilemap painting,
undo/redo, play/pause/step controls, debug overlays, and **Build & Reload** for
project runtime libraries.

## Included simulations

| Project | Features |
| --- | --- |
| **AntSimulation** | Colony foraging, pheromone trails, food collection, ant collisions, custom rendering, live environment brushes, and colony/ant inspection. |
| **RoverLab** | Ultrasonic and LiDAR exploration, discovered maps, destination routing, infrared Microbot maze solving, benchmark comparisons, and CSV export. |

RoverLab includes **Ultrasonic**, **LiDAR**, **Microbot**, **MicrobotMedium**, and
**MicrobotHard** scenes. The room scenes explore before navigating to destinations
selected in the scene. Microbot discovers the yellow goal, returns to its start,
and can solve using the learned maze.

These projects use simulated sensing and motion. They are examples for studying
simulation and navigation, not hardware-validated robot controllers or a full
SLAM implementation.

## Build

Development and testing currently target macOS. Install a C++20 compiler,
CMake 3.25 or newer, Ninja, and [vcpkg](https://github.com/microsoft/vcpkg).
Set `VCPKG_ROOT` to your vcpkg checkout. The repository's `vcpkg.json` declares
SFML and nativefiledialog-extended.

From the repository root:

```sh
cmake --preset release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DPIPEFRAME_ANT_REWORK_ONLY=ON
cmake --build --preset release --target SimulationWorkbench RoverLabRuntime
```

The `PIPEFRAME_ANT_REWORK_ONLY` option excludes the older SailBoat example; it
still includes the Workbench, AntSimulation, and RoverLab targets. On macOS,
CMake selects dynamic vcpkg libraries so the editor and its runtime plugins share
the native backend.

## Run a project

Open AntSimulation:

```sh
./cmake-build-release/apps/SimulationWorkbench/SimulationWorkbench \
  --project examples/AntSimulation/project.pipeframe
```

Or open RoverLab:

```sh
./cmake-build-release/apps/SimulationWorkbench/SimulationWorkbench \
  --project examples/RoverLab/project.pipeframe
```

You can also launch the Workbench without arguments and use **Open** to select a
`project.pipeframe` file. Use **Build & Reload** after changing project code,
choose a scene with **Scenes**, then press **Play**. **Pause**, **Step**, and
**Reset** control the simulation; Reset restores the authored starting state.

In AntSimulation, open **FOOD / TOOLS** for live painting and **SELECTED ANT** to
inspect or follow an ant. In RoverLab, use **ROVER MISSION** for exploration and
navigation, **DISCOVERED MAP** for pan/zoom/follow, and **BENCHMARK RESULTS** to
load benchmark journals and export comparisons.

## Source layout

```text
engine/                         Shared engine and native backend
apps/SimulationWorkbench/       Desktop editor and project host
examples/AntSimulation/         Ant project, assets, scenes, and tests
examples/RoverLab/              Rover project, assets, scenes, and tests
benchmarks/AntBenchmark/        Ant benchmark executable
```

Within a simulation project:

```text
Source/
  Components/          Data and Inspector schemas
  Entities/            Entity composition
  Editor/              Project panels, brushes, and interaction tools
  World/
    Simulation/        Simulation state and systems
    Physics/           Movement and collision handling
    Rendering/         Geometry, resources, and render layers
  Integration/         Type registration, plugin entry points, and lifecycle
```

Ant's `AntWorld` and RoverLab's `ProjectWorld` coordinate their world modules.
Components expose schemas through `Component<T>`; systems register with
`AddSystem`, and project panels use `ViewPanel` with `ProjectDefinition::UI`.
Integration adapters connect those modules to the Workbench. Ant retains a
specialized runtime for its colony lifecycle and rendering; RoverLab uses the
shared scene runtime.

## Selected regression checks

Build the test executables before running them:

```sh
cmake --build --preset release --target \
  AntDashboardTests RoverRuntimeTests RoverMapPanelTests RoverResultsTests
ctest --test-dir cmake-build-release --output-on-failure \
  -R '^(AntDashboardAcceptance|RoverRuntimeRegression|RoverMapPanelRegression|RoverResultsRegression)$'
```

These checks cover Ant's panels and input handling, Rover runtime behavior,
minimap controls, and result/CSV handling. Additional engine, editor, simulation,
and benchmark tests are declared in the CMake files. Native UI and rendering
tests require a graphical desktop session.

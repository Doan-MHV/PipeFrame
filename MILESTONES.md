# PipeFrame Milestones

PipeFrame is moving toward a C++ simulation engine and editor. The current 2D editor/runtime is the foundation for authoring repeatable scenarios, running disposable Play-mode simulations, and later experimenting with physics and reinforcement-learning style agents.

This file tracks larger architecture milestones. It should describe where the engine is going without pretending every piece exists today.

## Milestone 1: Reliable Editor Authoring

Goal: make the editor trustworthy as the source of truth.

- Keep Edit mode state safe.
- Make Play mode disposable.
- Restore authored entities, tilemap, terrain, and component data after Play mode.
- Save/load authored entities through `.entities.json`.
- Keep runtime-only entities out of saved content.
- Improve inspector editing for identity, tags, groups, sprites, colliders, animation, rigid body, and attributes.

## Milestone 2: Native Simulation Behaviors

Goal: make C++ gameplay/simulation modules useful for scenario prototyping.

- Keep scenario behavior in native systems and engine services.
- Expose query helpers so systems can find nearby entities.
- Expose public entity attributes for shared simulation facts.
- Expose named scalar fields through simulation field services.
- Add terrain-aware C++ A* pathfinding for agents.
- Add read-only runtime debugging for selected entities.
- Add viewport path visualization for selected agents.
- Add a first-pass path cache so repeated agent requests can reuse C++ A* results.
- Make Play-mode update order explicit through named simulation phases.
- Keep world changes behind clear system/service boundaries.
- Keep shared scalar field behavior behind an engine service instead of burying it inside one behavior system.
- Keep debug overlays in the editor/view layer.
- Keep simulation tuning values in prefab data, attributes, or project data.

Current direction:

- Use `attributes` for public facts stored on entities.
- Use named fields for shared world signals such as food, home, danger, heat, reward, or congestion.
- Move heavier simulation work into C++ services.

## Milestone 3: Project Root and Asset Path Architecture

Goal: stop depending on `cmake-build-debug/assets` or the current working directory.

The app/editor should pass a project configuration into the engine:

```cpp
struct ProjectConfig
{
    std::filesystem::path projectRoot;
    std::filesystem::path assetsRoot;
    std::filesystem::path assetManifestPath;
    std::filesystem::path startupLevelPath;
};
```

Possible project file:

```json
{
  "name": "JungleDemo",
  "assets_root": "assets",
  "asset_manifest": "assets/AssetManifest.json",
  "startup_level": "assets/levels/Level1.json"
}
```

Rules:

- Paths inside the project file should resolve relative to the project root.
- The editor executable should accept a project file path as its first argument.
- The editor executable may copy assets for development, but the engine library should not rely on copied build-folder assets.
- `AssetStore` and `LevelLoader` should receive resolved paths instead of guessing where files live.

This milestone is important because PipeFrame should eventually behave like an engine that can open different simulation projects, not only one demo executable.

## Milestone 4: Library-Oriented Engine Split

Goal: make the codebase easier to reuse as an engine/editor instead of one large application target.

Possible long-term structure:

```text
PipeFrame/
  engine/
    CMakeLists.txt
    src/
      ECS/
      Components/
      Systems/
      Simulation/
      AssetStore/
      Map/
      Game/

  editor/
    CMakeLists.txt
    src/
      Editor/
        Inspector/

  apps/
    PipeFrameEditor/
      CMakeLists.txt
      Main.cpp

  projects/
    JungleDemo/
      PipeFrameProject.json
      assets/
```

Possible CMake target direction:

```text
PipeFrameEngine      -> runtime/simulation library
PipeFrameEditor      -> editor UI library
PipeFrame            -> editor executable
```

Do not split every small folder into its own CMake target too early. `Inspector` can stay part of the editor target. Split only when there is a real ownership boundary.

## Milestone 5: Physics Simulation Foundation

Goal: add physics as a first-class simulation system.

- Add physics-oriented components such as mass, velocity, acceleration, force, friction, restitution, and body type.
- Decide what should be custom physics versus external library support.
- Add fixed-step simulation that is separate from render frame rate.
- Add debug visualization for velocity, forces, bounds, collision normals, and contacts.
- Create simple repeatable scenarios with boxes, ramps, moving platforms, vehicles, rockets, or aircraft.

## Milestone 6: Scenario and Experiment Tools

Goal: make simulation scenarios easy to run, reset, inspect, and tune.

- Add scenario metadata for physics settings and deterministic seeds.
- Add spawn/reset definitions.
- Add scenario templates for cars, planes, rockets, boxes, and agent groups.
- Add debug panels for simulation state.
- Add logging for simulation runs.
- Keep authored parameters visible and editable.

## Milestone 7: Reinforcement Learning Direction

Goal: prepare for learning experiments after the simulation loop is reliable.

- Define an environment API shape: reset, step, observation, action, reward, done.
- Start with tiny experiments such as target seeking, balance, obstacle avoidance, or ant-like movement.
- Log observations, actions, rewards, and episode results.
- Keep training integration outside the core editor until the simulation foundation is stable.

## Current Priority

The next architecture priorities are:

- keep the inspector refactor small and domain-based
- protect Play/Edit state separation
- validate authored component and attribute data before runtime
- introduce project-root based asset loading
- grow native simulations from per-agent A* toward shared field and flow-field navigation
- grow toward physics and repeatable simulation scenarios

The chopper/gameplay content is useful test content, but it should not define the final purpose of PipeFrame.

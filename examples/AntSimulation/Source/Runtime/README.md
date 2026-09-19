# Project runtime integration

AntSimulationRuntime connects authored scenes, registration, lifecycle phases and neutral
presentation to ProjectRuntime. AntRegistration consumes component-owned schemas and
registers the entity recipes/factories. GeneratedRegistration is consumed when present;
use the editor's source generator and Build & Reload rather than editing that file.

AntWorld owns the composed worlds. Its AntRuntimeWorld owns the one BehaviourScene;
AntQuery/AntView/ColonyView live under `../World/Runtime` and borrow its typed storage.
AntPhysicsWorld and AntRenderingWorld register their smaller systems/layers. No
AntSimulationPipeline, AntStore or aggregate Ant/Colony owner remains.

../Editor/AntUIHost.cpp is a neutral SimulationDashboard composition boundary, not an SFML adapter.
Native embedding belongs to the engine/backend host. See the project README for the full
source map, edit/play policy and developer walkthroughs.

## Why two Runtime folders?

This outer folder is **editor integration**, not simulation logic. It translates
PipeFrame project lifecycle and authoring operations into AntWorld calls.
`World/Runtime/` is where simulation logic belongs. New physics/render/runtime
systems should be owned by their matching World module.

The shared editor PHYSICS/MESH controls call `CollectWorldDebug` here, which
forwards to AntPhysicsWorld and AntRenderingWorld. Actual debug geometry lives
with those worlds; it does not advance simulation or modify authored data.

## World delegation

AntWorld::FromScene (implemented in World/Runtime/AntWorldAuthoring.cpp) validates
and constructs a candidate world from scene data, loads its environment, applies
food sources and restores registered entities. The adapter swaps in the result
only after success. Its factory callback preserves project-defined entity types.

AntRenderingWorld owns renderer asset loading, render options, geometry preparation,
material/terrain/beacon drawing, layer order and render statistics. Render forwards
the editor selection overlay as a callback so its original layer order is retained.
AntEnvironment owns radius-based food queries. The adapter keeps host lifecycle,
authoring transactions, input routing, registration and selected-object state.
UI dashboard hosting now lives in Editor/AntUIHost.cpp alongside AntDashboard.cpp.

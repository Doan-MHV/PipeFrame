# Project runtime integration

AntSimulationRuntime connects authored scenes, registration, lifecycle phases and neutral
presentation to ProjectRuntime. AntRegistration consumes component-owned schemas and
registers the entity recipes/factories. GeneratedRegistration is consumed when present;
use the editor's source generator and Build & Reload rather than editing that file.

AntWorld owns the composed worlds. Its AntRuntimeWorld owns the one BehaviourScene;
AntQuery/AntView/ColonyView live under `../World/Runtime` and borrow its typed storage.
AntPhysicsWorld and AntRenderingWorld register their smaller systems/layers. No
AntSimulationPipeline, AntStore or aggregate Ant/Colony owner remains.

AntUIHost.cpp is a neutral SimulationDashboard composition boundary, not an SFML adapter.
Native embedding belongs to the engine/backend host. See the project README for the full
source map, edit/play policy and developer walkthroughs.

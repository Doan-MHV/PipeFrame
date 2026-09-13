# Entity recipes

AntEntity and ColonyEntity derive from PipeFrame EntityArchetype and add typed
components to the scene. Instantiate them through BehaviourScene::Instantiate
so initialization and rollback run consistently.

```cpp
pipeframe::BehaviourScene scene;
const auto object = scene.Instantiate(
    ant_simulation::AntEntity(colonyId, ant_simulation::AntRole::Follower,
                              {10, 12}, 0, 0, configuration));
auto &energy = *object.GetComponent<pipeframe::EnergyComponent>();
energy.Consume(0.5f);
```

Recipes assemble data; registered systems and attached behaviours execute it.
Creating a recipe does not create a second Ant/Colony state owner. The editor generates/registers source and runs Build & Reload; recipes remain separate from scene instances.

## Signal Beacon

Create through the editor hierarchy **+ → SIGNAL BEACON**. `SignalBeaconEntity.h` composes Transform and SignalBeaconComponent and attaches SignalBeaconBehaviour. Read the component header for exposed fields and validation, the behaviour header for fixed-step rotation, and World/Rendering/SignalBeaconGeometry.h for neutral geometry. Play rotates the marker; pause stops it; reset restores authored settings. It has no ant combat or foraging behavior.

## Registration and creation

`Runtime/AntRegistration.h::AntEntityTypes()` registers AntEntity (runtime-only), ColonyEntity, SignalBeaconEntity and generic component recipes for Food Source and Simulation Settings. AntQuery and ColonyLifecycleSystem spawn typed recipes through this same registry. `Runtime/AntRegistration.cpp::BindAntFactories()` binds Colony's environment-dependent authored factory and post-restoration hook. The editor menu comes from the registry, not a parallel component-name array.

New generated Entity classes are added through **+ → NEW ENTITY / COMPONENT / BEHAVIOUR**. Choose Build & Reload; compiler output and failure recovery are handled by the editor. GeneratedRegistration.h is consumed automatically when present. Optional components/behaviours can be attached from the Inspector. New custom rendering still needs a renderer; registering an entity alone does not give it visible geometry.

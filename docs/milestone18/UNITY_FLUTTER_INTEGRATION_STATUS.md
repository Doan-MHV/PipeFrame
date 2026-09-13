> Scope update (2026-09-11): [Engine rework — Ant plan](../ENGINE_REWORK_ANT_PLAN.md) governs remaining engine/editor work. Ant is the sole reference project; SailBoat implementation and tests are excluded. Earlier completion statements do not establish completion of this rework.

# Unity and Flutter integration: implementation status

This is an implementation audit, not a declaration that Milestone 18 is complete.

## Authoring roles

- `Source/Entities/`: object recipes derived from `pipeframe::EntityArchetype`. The engine allocates identity and rolls back partial component construction on failure. Ant spawning now uses `Entities/AntEntity.h`.
- `Source/Components/`: stored state and property schemas. Components do not need virtual base classes to participate in ECS queries. Ant remains a large compatibility component; its internal state has not yet been split into independent motion, energy, and rendering components.
- `Source/Behaviours/`: attached scripts derived from `pipeframe::Behaviour`, with Start, Update, FixedUpdate, enable/disable, and destruction callbacks. Ant's colony script is here.
- `Source/Systems/`: reusable operations over runtime state. Ant movement/behavior/cleanup and SailBoat movement are implemented systems, not just IDs.
- `Source/Runtime/`: plugin runtime orchestration and storage.
- `Source/Editor/`: view descriptions and editor extensions.

`pipeframe::ecs::Entity` is declared in `ECS/Entity.h`; it is an identity, not a game-specific class hierarchy. The `Entities` folder is the composition entry point, not an additional copy of component state.

SailBoat's Boat, BoatEnvironment, PolarTable and simulation schemas now live in Components; BoatMovementSystem lives in Systems; the runtime and dashboard live in Runtime and Editor respectively. SailBoat still owns boats through its training implementation, rather than using the new entity archetype path. This is a remaining migration, not completed ECS parity.

## Implemented framework behavior

`EntityArchetype::Instantiate(World&)` allocates an entity and calls Build to attach components; failures remove every attached component. AntStore uses this path.

BehaviourScene dispatches lifecycle hooks and defers destruction. Destruction callbacks can queue other objects for destruction without invalidating script storage. Destroying objects reject new attachments. Callback exceptions are propagated by explicit operations after teardown finishes; the scene destructor cannot propagate them.

`UI/View.h` provides backend-neutral Row, Column, Text, Button and controlled Toggle descriptions. A complete tree is validated for stable unique sibling keys and finite nonnegative layout values before rendering. The retained adapter reconciles by key. Ant selection actions use Toggle descriptions.

`UI/StatefulView.h` owns state plus a Build function. SetState invalidates the cached description. Build validates and recomputes after a change. The UI host must call Build during its refresh; this is not an automatic frame scheduler. Callbacks and their captured objects must remain alive.

ProjectScaffolder supports Entity and Behaviour templates in their standard folders and records them in the module manifest. This API does **not** yet implement an editor button that compiles, registers and attaches arbitrary user scripts.

## Remaining completion gates

1. Decompose Ant and Boat compatibility state into shared components and query systems without changing simulation behavior.
2. Migrate SailBoat construction to the same entity composition and lifecycle ownership as Ant.
3. Connect generated behaviours and member-bound component schemas to editor creation, compilation, reload, attachment, serialization and live editing.
4. Remove SFML from public project/editor authoring surfaces. Existing dashboard/render adapters still expose SFML; this pass does not claim backend isolation.
5. Migrate complete dashboards to the declarative view API, including inputs, scrolling, charts, and responsive layout, with visual and interaction verification.
6. Integrate collision, trigger, visibility, and coroutine lifecycle services with actual physics/render scheduling.
7. Test the complete robot-project authoring path through the editor.

The new APIs make progress toward these gates; declarations, folders and passing existing tests do not replace end-to-end verification.

## Minimal authoring examples

```cpp
struct Energy { float amount = 10; }; // Source/Components/Energy.h

class RobotEntity : public pipeframe::EntityArchetype { // Source/Entities/
    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity id) const override {
        world.Add<Energy>(id);
    }
};
class DrainEnergy : public pipeframe::Behaviour { // Source/Behaviours/
    void FixedUpdate(float dt) override {
        if (auto *energy = GetComponent<Energy>()) energy->amount -= dt;
    }
};
pipeframe::BehaviourScene scene;
auto id = RobotEntity{}.Instantiate(scene.Components());
scene.Attach<DrainEnergy>(id);
scene.FixedUpdate(1.0f / 60.0f);
```

This demonstrates composition and attached execution. It does not expose Energy in the Inspector automatically; member-bound property schema registration is a separate requirement.

```cpp
using namespace pipeframe::ui;
StatefulView<int> counter(0, [](const int &value) {
    return views::Column("counter", {views::Text("value", std::to_string(value))});
});
counter.SetState([](int &value) { ++value; });
// During the owning UI refresh:
dashboard.RenderView(parent, counter.Build());
```

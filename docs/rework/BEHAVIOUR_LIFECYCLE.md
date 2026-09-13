# Behaviour lifecycle, tasks and service callbacks

PipeFrame's C++ authoring contract lives in
[Scene.h](../../engine/include/PipeFrame/ECS/Scene.h). Attach a registered Behaviour to
an entity; the owning BehaviourScene dispatches it. Batch Ant rules still run through
systems, without allocating one Behaviour or coroutine per ant.

## Order and ownership

On the first eligible scene update: `OnEnable`, `Start`, then `Update` or `FixedUpdate`.
Start runs once. FixedUpdate advances that Behaviour's coroutines after its own callback.
Disabled behaviours and inactive objects do not run callbacks or advance their task clock.
Re-enabling resumes their retained tasks. Destroy/reset/unload destroys the script and its
suspended coroutine frames. Destruction requested during a callback is deferred until
scene dispatch ends; later callbacks/tasks on a pending-destruction object are skipped.

Tasks created while tasks are being resumed start on the next eligible fixed tick. Each
task resumes at most once per tick. Waits use simulation time, not wall time; a zero wait
means the next tick. Pause/speed/single-step therefore follow the simulation scheduler.
Exceptions propagate to the caller with scene dispatch restored; scripts should handle
recoverable failures themselves. This is cooperative scheduling, not background threading.

```cpp
class PulseBehaviour final : public pipeframe::Behaviour {
public:
    void Start() override { task = StartCoroutine(Pulse()); }
    void OnTriggerEnter(const pipeframe::CollisionEvent2D &event) override {
        StopCoroutine(task);
        // event.other is the other scene entity; validate handles before later use.
    }
private:
    CoroutineId task{};
    pipeframe::Coroutine Pulse() {
        for (;;) {
            if (auto* transform = GetComponent<pipeframe::Transform2DComponent>())
                transform->rotation += 0.1f; // Runtime rotations are radians.
            co_yield pipeframe::WaitForSeconds{0.5f};
        }
    }
};
```

Include `PipeFrame/Components/Transform2DComponent.h` for the component in this example.
`StopCoroutine(id)` / `StopAllCoroutines()` cancel future continuation. Cancelled storage
is released by the next task sweep or script destruction; cancelling from within a task
never destroys its currently executing stack.

## Physics and rendering deliver events

The shared [SceneProjectRuntime](../../engine/include/PipeFrame/Project/SceneProjectRuntime.h)
collects contacts after kinematic movement, then dispatches `OnCollisionEnter/Exit` and
`OnTriggerEnter/Exit`. Solid tilemap/box/segment contacts use the existing environment
queries. An `EnvironmentCollider2DComponent` with `trigger = true` detects circle-body
intersection/crossing without blocking motion. Each pair transitions once per category;
both eligible receivers are notified, with opposite normals. A through-crossing is an
enter on that tick and an exit on the following clear tick. If the other entity has been
destroyed, a surviving receiver's exit carries `ecs::InvalidEntity`.

This is the current 2D kinematic/environment service, not a general rigid-body collision
engine. Trigger sweeps cover body translation against the sampled trigger shape. Rotating,
scaling or rapidly moving triggers do not have continuous trigger detection. No Stay
callback is promised. Contact state is sampled per fixed step; arbitrary transient contacts
inside a custom solver require that solver's event integration.

After drawing, the runtime compares visible environment shape/playground/tilemap bounds
against its current axis-aligned camera and emits `OnBecameVisible/Invisible`. Events begin
after the script has started and are delivered only to active, enabled scripts. Bounds
visibility is a coarse camera test, not GPU occlusion or pixel coverage. Multiple-camera
aggregation and arbitrary custom-renderer bounds are outside this service's contract.

A custom World/physics/rendering implementation owns its domain event semantics and calls
`BehaviourScene::NotifyContact` / `NotifyVisibility` after traversal. These entrypoints
preserve safe dispatch and destruction. Ant's specialized population solvers are not
silently replaced or forced to emit a virtual callback for every ant contact.

## Verification and compatibility

[BehaviourServicesTests](../../engine/tests/BehaviourServicesTests.cpp) exercises real
kinematic contact/trigger movement, camera transitions, reciprocal pair deduplication,
coroutine timing/cancellation, inactive task clocks and destruction during callbacks.
[FoundationHeadlessTests](../../engine/tests/FoundationHeadlessTests.cpp) checks scene
identity, activation and structural changes in both Debug and Release.

Plugin ABI is **13**. Rebuild project plugins because Behaviour's layout and virtual
interface changed. The editor rejects old plugin binaries instead of loading mismatched
objects. Existing source behaviours need no new overrides; all new hooks default to no-op.

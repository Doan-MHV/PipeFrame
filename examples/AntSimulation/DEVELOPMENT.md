# Ant development guide

Open `project.pipeframe` in SimulationWorkbench and choose **Build & Reload** for the
current host (ABI 14). R6 introduced editor-authored environments and the complete
build/reload path. The older imperative/native host APIs are not project authoring APIs.

## Where to put a change

| Responsibility | Current source | Engine contract |
| --- | --- | --- |
| Assemble an object | `Source/Entities/AntEntity.h`, `ColonyEntity.h`, `SignalBeaconEntity.h` | EntityArchetype; registered recipe |
| State and exposed fields | `Source/Components/*Component.h` | component-owned Schema; ECS storage |
| Attached object logic | `Source/Behaviours/` | Behaviour lifecycle |
| World ownership/phase order | `Source/World/AntWorld.h/.cpp` | World; one AntRuntimeWorld owns BehaviourScene |
| Batch domain rules | `Source/World/Runtime/Systems/` | FixedUpdateSystem; colony/foraging/cleanup |
| Movement and contacts | `Source/World/Physics/` | PhysicsWorld; shared steering, body storage, contacts and grid queries |
| Simulation fields and map import | `Source/World/Runtime/Environment/` | shared grids/tilemaps; Ant food/marker policy |
| Borrow actual ECS data | `Source/World/Runtime/AntView.h`, `ColonyView.h`, `AntQuery.h` | ComponentView and SceneViewCache; no second population owner |
| Render simulation geometry | `Source/World/Rendering/` | RenderingWorld and RenderLayer, neutral Canvas |
| Build UI and custom brushes | `Source/Editor/` | View/StatefulView/SchemaBrush |
| Connect the project to the host | `Source/Runtime/AntSimulationRuntime.*`, `Source/Editor/AntUIHost.cpp` | ProjectRuntime and neutral SimulationDashboard |
| Register types/factories | `Source/Runtime/AntRegistration.*` | ComponentRegistry and EntityRegistry |

This World hierarchy is the later user-approved organization, replacing the original
flat Systems/Rendering/Physics proposal. A new rule belongs under its owning subworld,
not in a miscellaneous folder. The subworld registers its ordered work once; a component
contains data and schema, not an Update dispatcher. Plain data needs no virtual base.

## Author and inspect

Hierarchy **+** lists registered authored recipes. AntEntity is runtime-only and is spawned
by colony logic; a simulation population is not serialized as thousands of scene objects.
Colony, Food Source, Simulation Settings, Playground and Signal Beacon are authored types.
The Beacon is a real second entity example with attached behaviour; no soldier combat AI
is implied by a type name or role enum.

Create source through **+ → NEW ENTITY / COMPONENT / BEHAVIOUR**, then **Build & Reload**.
Prefer role suffixes (`SensorComponent`, `SensorBehaviour`, `SensorEntity`). The generator
preserves the explicit class name you type and maintains GeneratedRegistration; it does
not automatically execute arbitrary files you add by hand. Attach registered components
in the Inspector. Systems are scheduled explicitly by their owning world.

Fields live in `Component::Schema()`: Editable for authored values, ReadOnly for runtime
telemetry, omitted for internal state. Edit colony Transform/Settings through the shared
Inspector and gizmos; validation, undo, redo, save and reopen use the same real data.
Read-only Colony State/History and individual-ant telemetry remain read-only after Pause
or Reset. Individual runtime ants can be picked/inspected/followed; they are recreated
from colony settings on Reset, not saved as per-ant authored records.

For a map: select/create Playground, **Assets → Maps → New Map**, paint terrain and food,
Save Map, Save scene and Reset. Use Make Unique before changing only one instance of a
shared map. [The environment workflow](../../docs/rework/R6_ANT_ENVIRONMENT_AUTHORING.md)
explains Ant's unit-cell and origin constraints and density-only food authoring.

## Learn and verify

- [Blank project, exposed fields and query-driven behaviour](../../docs/tutorials/blank-project/README.md)
- [Independent scene + stateful UI](../../docs/tutorials/independent-ui/README.md)
- [Behaviour tasks and service events](../../docs/rework/BEHAVIOUR_LIFECYCLE.md)
- [Schema convention](../../docs/rework/COMPONENT_SCHEMA_CONVENTION.md)
- [Custom brush API](../../docs/rework/R6_CUSTOM_BRUSHES.md)
- [Final performance evidence](../../docs/rework/R6_FINAL_PERFORMANCE.md)
- [Reference/parity limits](PARITY.md)

Use the Ant-only CMake configuration `PIPEFRAME_ANT_REWORK_ONLY=ON`. SailBoat and its
tests are outside this audit. Core/Scene and native widgets are explicitly backend host
implementation; new project code uses ECS/Scene and the public declarative UI API.

World construction from authored scenes is implemented by `AntWorld::FromScene`
in `Source/World/Runtime/AntWorldAuthoring.cpp`. Rendering assets, options, frame
preparation and draw order belong to `AntRenderingWorld.cpp`. The outer runtime
is the editor adapter; see `Source/Runtime/README.md` for its responsibilities.

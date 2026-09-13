# R2 — typed components and live editor properties

Status: the documented R2 colony-editing gate is implemented and verified (2026-09-12).

Gate: edit actual ColonyEntity Transform and ColonySettings data through the Inspector; validate, undo/redo, save/reopen and play/reset those edits. Adding an exposed field uses its schema without a project-specific Inspector or runtime property-name dispatcher.

## Data flow

1. A recipe attaches real typed components to an engine SceneObject.
2. `ComponentRegistry::Register(schema)` binds the C++ type to its stable component ID, metadata, presence query, snapshot reader and validated writer.
3. A runtime provides `GetComponentRegistry()` and `ResolveSceneObject(authoredId)`. Engine `InspectObjectComponents` returns only registered components actually attached to that entity.
4. Workbench asks the runtime for those components when refreshing selection. Inspector owns a copy of the displayed snapshot and builds fields from registered descriptors. It does not import Ant types.
5. A field commit enters `ProjectSession::SetSelectedComponentProperty`. It prepares a candidate scene document and validates the live batch before recording history or changing the document.
6. Ant applies property-only edits to the live typed components. Undo and redo detect the same property-only change and use that path; structural scene changes still rebuild the simulation.
7. The existing scene serializer persists authored component maps. Live telemetry is displayed separately and never copied into the authored document by inspection.

`ComponentRegistry` validates all mutations before applying any of them. Unknown fields, wrong types, read-only edits, invalid ranges, nonfinite values and destroyed entity handles fail without partially applying the batch. The registry reacquires components through checked handles rather than caching pointers into relocatable ECS pools. Restore mode can construct missing registered components from schema defaults and authored values.

Accessor conversions belong in the schema. Transform storage uses radians; the common schema reads/writes degrees for scene/editor values. Position and scale use the existing vector types. The document's older Transform mirror is maintained by engine conversion helpers, not an Ant-specific property switch.

## Registering another component or field

```cpp
struct SensorComponent {
    double range{5};
    std::string label{"front"};
    float reading{};
    static auto Schema() {
        using namespace pipeframe;
        return ComponentSchema<SensorComponent>("robot.sensor", "Sensor")
            .Editable({.key="range", .displayName="Range", .kind=PropertyKind::Number,
                       .defaultValue=5.0, .minimum=0, .maximum=100}, &SensorComponent::range)
            .Editable({.key="label", .displayName="Label", .kind=PropertyKind::String,
                       .defaultValue=std::string{"front"}}, &SensorComponent::label)
            .ReadOnly({.key="reading", .displayName="Reading", .kind=PropertyKind::Number,
                       .defaultValue=0.0}, &SensorComponent::reading);
    }
};
registry.Register(SensorComponent::Schema());
// Recipes attach data; registration consumes the component-owned schema.
```

The field declaration supplies Inspector metadata, validation and serialization together. Ordinary fields do not need a new base class or property-name switch. `Accessor` supports representation conversion and `ReadOnly` supports computed telemetry. Component accessors must operate on the supplied value; scene mutation belongs in systems/commands.

This is explicit C++ schema registration, not automatic reflection over every C++ member. Only declared fields are exposed. Source generation and automatic registration/build wiring remain R6.

## Ant integration and semantics

Ant registers its actual Identity, Pose, Foraging, Encounter, ColonySettings, ColonyState and ColonyHistory component types, together with common Transform, Motion, Energy and RandomState components. Components without exposed fields still appear as attached sections. Internal telemetry is read-only.

The authored colony selection is wired end to end into Workbench. Ant's transient individual-ant panel still awaits the full R5 panel conversion; its component schemas and generic ECS inspection are now available for that work. Food/environment settings keep their existing authored configuration path. R2 does not claim every legacy object is already an ECS entity.

- Each colony restores its own settings instead of inheriting the first colony's settings as its effective data.
- Movement reads the colony's speed setting. Collision search accounts for actual body speeds.
- Radius and color edits update colony state; moving/resizing a colony refreshes its persistent home markers.
- Each colony owns an engine RandomState component. Seeds initialize that colony's stream on initialization/reset.
- While stopped, initial population and seed edits update initialization state.
- During a running or paused preview, initial population and seed edits are authored for the next reset. They do not retroactively change population/reserve or reseed the current stream.
- Property edits and undo/redo during preview preserve the current population and reserve.
- Reset reconstructs preview state from the edited authored scene. Save/reopen restores authored values, not an arbitrary snapshot of running simulation state.

General transform hierarchy propagation and complete renderer/editor transform behavior remain R1/R5 work. The registry does not turn metadata-only physics/rendering descriptors into attached, functioning ECS components. Those broader authoring/runtime integrations must be verified in their own stages.

## Inspector fixes

The existing Inspector is the active R2 consumer. It discovers attached components from live snapshots, displays read-only telemetry, and grows its row storage as needed instead of silently stopping at 64 rows. Integer and object-reference input uses exact text parsing instead of conversion through float. Long field captions are placed above their inputs using reusable engine labeled-field layout, avoiding the overlap visible in the first acceptance render.

The full declarative Ant/editor redesign is still R5. R4's schema parser is reused here; this stage does not claim the imperative Inspector implementation has already been replaced with mounted views.

## Compatibility

The project runtime interface is now plugin ABI **3**. Plugins must be rebuilt against the current engine; incompatible old binaries are rejected rather than used with a mismatched interface. Active Ant-only build targets and infrastructure examples were rebuilt. No SailBoat code or runtime tests were added to this work.

## Evidence

- `ComponentRegistryAcceptance`: unrelated Sensor component, new exposed field, real ECS writes, presence discovery, default construction, multi-entity rejection, read-only/unknown/nonfinite rejection, degree/radian conversion, relocation and destroyed selection.
- `AntComponentEditingAcceptance`: loads the real Ant runtime in an isolated temporary project; sends actual pointer/keyboard events to Inspector fields; verifies independent colonies, live component discovery absent from the scene file, full integer seed precision, Transform writes, read-only/invalid edits, required-component protection, undo/redo without population reset, preview/reset semantics, and save/reopen.
- Final build passed; **64/64 tests passed** with `PIPEFRAME_ANT_REWORK_ONLY=ON`.
- [Saved complete test output](R2_ACCEPTANCE_RESULTS.txt).
- [Saved real Inspector render](evidence/r2-live-inspector.png). Lower fields remain accessible through the Inspector's scroll view.

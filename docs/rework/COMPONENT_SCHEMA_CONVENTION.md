# Component-owned schemas

Required convention for the active Ant rework and generated components, September 12, 2026.

Open a component header to find its data and its static `Schema()`. Registration calls `registry.Register(Component::Schema())` and contains no field declarations. Ant's live component schemas, FoodSource/SimulationSettings authoring schemas, shared Transform/Motion/Energy/Random schemas and the editor's component generator follow this convention. SailBoat remains outside the active rework scope.

## Exposure and edit permissions

- `.Editable(descriptor, &Component::member)` exposes an editable member.
- `.ReadOnly(descriptor, &Component::member)` exposes a member but denies editor writes.
- `.ReadOnly(descriptor, getter)` exposes a computed value with no restore setter.
- `.Accessor(descriptor, getter, setter)` handles representation conversion; set `.editable=true` or `false` explicitly in its descriptor. Transform converts radians to displayed degrees this way.
- A member omitted from `Schema()` is hidden from the Inspector and absent from schema serialization. This is not a hidden-but-persisted field feature. Document intentionally omitted runtime data beside its declaration.
- `.Required()` prevents component removal; it does not control field editability.

Use named descriptor members to avoid positional boolean arguments. Type, default, units, minimum, maximum and step stay beside the member binding. Read-only labels currently appear as LIVE. Components without exposed properties show an explanatory empty-state message.

## Validation example

```cpp
struct SensorComponent {
    double minimumDistance{0.0};
    double maximumDistance{10.0};
    float measuredDistance{};
    int cachedHitIndex{-1}; // Runtime-only: omitted from Schema().

    static auto Schema() {
        using namespace pipeframe;
        return ComponentSchema<SensorComponent>("project.sensor", "Sensor")
            .Editable({.key="minimum", .displayName="Minimum distance",
                       .kind=PropertyKind::Number, .defaultValue=0.0,
                       .unit="m", .minimum=0.0},
                      &SensorComponent::minimumDistance)
            .Editable({.key="maximum", .displayName="Maximum distance",
                       .kind=PropertyKind::Number, .defaultValue=10.0,
                       .unit="m", .maximum=100.0},
                      &SensorComponent::maximumDistance)
            .ReadOnly({.key="measured", .displayName="Measured distance",
                       .kind=PropertyKind::Number, .defaultValue=0.0, .unit="m"},
                      &SensorComponent::measuredDistance)
            .Validate("Minimum distance must not exceed maximum distance",
                      [](const auto &candidate) {
                          return candidate.minimumDistance <= candidate.maximumDistance;
                      });
    }
};

registry.Register(SensorComponent::Schema());
```

Built-in type, finite-number, enum and range validation runs first. `Validate(message, predicate)` then checks the complete candidate component after all proposed field assignments. Multiple rules can be chained; predicates should be pure and must not mutate the world. A false result or standard exception returns the rule's message without publishing the candidate. Rules can express required strings, dependent limits and project-specific constraints. They also run during schema restoration, so defaults and saved values must satisfy them.

ComponentRegistry applies editor read-only rules before schema writes, and prepares all mutations before committing a batch. `ComponentSchema::Apply` is also used for internal restoration: member-backed read-only fields can be restored there. Read-only is an editor permission, not C++ immutability. Direct component assignments in simulation code do not invoke schema validation automatically.

## Boundaries and compatibility

Existing `Field` and `PF_COMPONENT` APIs remain available to legacy callers; new generated components and active Ant examples use the convention above. `EnergySchema()` and `Transform2DSchema()` remain forwarding compatibility functions, with no independent field definitions. CommonComponentSchemas and AntRegistration only register owned schemas.

FoodSource and SimulationSettings now declare typed data/schema in Components instead of anonymous metadata structs inside AntRegistration. Their current runtime still consumes the authored property maps; this change does not claim complete ECS attachment migration for those objects. Full generated registration/build/reload/attachment remains R6; generating a header alone does not wire it into a runtime.

Colony State remains read-only; this cleanup does not introduce paused runtime editing. Colony History and Random State still intentionally expose no fields. The change makes that policy visible in their headers, not a claim that new history charts or generator diagnostics were implemented.

## Verification

The build passed. All 64 registered Ant-only checks are passing: 63 passed in the full run; the obsolete macro-location conformance assertion was updated to enforce this convention, and its focused rerun passed. [Full results and correction record](COMPONENT_SCHEMA_ACCEPTANCE.txt). Validation tests cover cross-field failures, required strings, multi-field atomicity, existing numeric limits and hidden-member rejection. Generated-header and schema-location checks prevent the old convention from returning.

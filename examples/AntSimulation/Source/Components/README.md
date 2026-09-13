# Components

These are scene-owned ECS data, assembled by `../Entities/AntEntity.h` and
`../Entities/ColonyEntity.h`:

- AntIdentityComponent: colony membership, role, display identity and physics link.
- AntPoseComponent: body/head/tail tracking and six AntLegPose records.
- ForagingComponent: food-search state, target, trail timers and delivery count.
- AntEncounterComponent: enemy-alert timer and optional opponent identity.
- ColonyStateComponent: colony identity, resources, radius and membership count.
- ColonyHistoryComponent: collection-rate samples and name attribution.
- ColonySettingsComponent: typed authoring settings and schema.

Shared transform, motion and energy components live in PipeFrame. Components do
not inherit an ID-only class: the engine owns entity identity and typed storage.
AntView/ColonyView are borrowed access facades under World/Runtime, not components.
Copying a view aliases the entity; copy its component data for a snapshot.

Every registered component declares a static `Schema()` in its own header. Use
`Editable` or `ReadOnly` for exposed members, named descriptors for constraints,
and `Validate` for whole-component rules. Unlisted members remain runtime-only.
`Runtime/AntRegistration.cpp` only registers these schemas. FoodSourceComponent
and SimulationSettingsComponent also own their authoring schema here.

See [the schema and validation convention](../../../../docs/rework/COMPONENT_SCHEMA_CONVENTION.md).
The Inspector already supports live colony editing with undo/persistence; runtime
telemetry marked ReadOnly remains non-editable even while paused or reset.

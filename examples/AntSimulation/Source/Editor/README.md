# Ant editor and simulation UI

`AntDashboard.cpp` describes the six simulation panels and timer using the public
PipeFrame `View` API. `AntSelectionView.h` composes the selected-ant actions.
There are no native widget subclasses or widget-pointer synchronization loops in
these view builders.

- Change simulation/model data in callbacks, then publish it at the UI update boundary.
- Reuse engine `Wrap`, `Scroll`, `Card`, `Chart`, `Progress`, `Mesh` and field views.
- Use the component registry and `SchemaInspector` for discoverable component data.
- Keep world tools/model logic in their existing editor tool/inspector classes.

`Source/Runtime/AntUIHost.cpp` composes the neutral SimulationDashboard. Native
embedding lives in the engine/backend host and is isolated by compile checks. The detailed migration, limitations and engine/editor
examples are in [R5 UI migration](../../../../docs/rework/R5_UI_MIGRATION.md).

## Authored environment brushes

`AntFoodBrush.h` is the reusable authoring example: it derives from
`pipeframe::SchemaBrush<AntFoodBrush>`, owns its settings schema, and supplies only
a numeric food-density effect. The runtime registers it once; the existing plugin
registry exposes it in Assets → Maps. Shared gestures, bounds, preview, erase, undo
and persistence are engine-owned. Save and reset to load the authored density into
Ant. `AntEditorTool` remains the transient live-simulation interaction.

See [custom brush authoring and verification](../../../../docs/rework/R6_CUSTOM_BRUSHES.md).

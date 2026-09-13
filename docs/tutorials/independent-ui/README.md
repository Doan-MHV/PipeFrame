# One independent scene, Behaviour and stateful UI

[CounterProject.h](CounterProject.h) is the complete backend-neutral project code.
It does not include or link Ant. The executable
[IndependentAuthoringTests](../../../engine/tests/IndependentAuthoringTests.cpp) compiles
that exact header and exercises a real mounted UI through the engine's desktop host.
A separate compile target proves the project header works without native include paths.

1. `CounterRuntime` registers a component-owned schema and an entity recipe.
2. `CounterEntity` composes Transform and Counter components and attaches `CounterBehaviour`.
3. A fixed update changes the actual Transform using the Counter rate.
4. `CounterPanel` derives from ViewPanel and returns Scroll → Column → StatefulView →
   Card/Button/Text. The project specifies content and layout constraints, not pixels.
5. The host mounts `panel.DescribeView()`. Clicking Increase rate changes the component
   and updates the nested UI state. The next fixed step consumes the changed rate.
6. Resize preserves the keyed button and state. Unmount disposes the retained source.
   Reset invalidates the old SceneObject and restores authored settings.

The sample's button is a **runtime control**, not an editor authoring transaction. It
clamps its direct write to the supported range. Workbench authoring should instead use
ProjectSession/schema transactions for validation, undo and scene persistence. Likewise,
stateful UI state is not automatically a serialized component. These are distinct owners.
Recreate the panel with the new scene object after Reset; a retained callback checks the
old handle before using it.

Project developers use ViewPanel, View and StatefulView. The test host intentionally
uses BackendSFML/UIManager to supply font metrics, mount bounds and native events;
those imports are not needed by CounterProject.h. A host is necessary to display any
project's UI; it is not a second project-facing UI framework.

To reproduce, build PipeFrame and run CTest with `-R IndependentAuthoringAcceptance`.
For source generation, assets, prefab persistence and Build & Reload, continue with
[the blank-project lesson](../blank-project/README.md).

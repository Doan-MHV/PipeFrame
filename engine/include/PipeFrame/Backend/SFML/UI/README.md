# Native UI implementation

These are the SFML widget implementation and legacy imperative host APIs. They are not
the project-facing UI contract. Normal project and editor-panel code uses
`PipeFrame/UI/View.h`, `ViewPanel.h`, `StatefulView.h` and `SimulationDashboard.h`.

Only native hosts/adapters and their tests should include this directory. Such targets
must opt into `PipeFrame::BackendSFML`; `PipeFrame::Engine` does not export that dependency.
Retired `PipeFrame/UI/Widget.h` and other imperative paths intentionally have no forwarding
headers, which would reintroduce native types into project authoring.

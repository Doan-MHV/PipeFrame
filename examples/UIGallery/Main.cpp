// Project-authored UI: no native widgets, fonts, event loop or SFML.
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/UI/AlertDialog.h>
namespace {
struct GalleryWorld {};
class GalleryPanel final : public pipeframe::ui::ViewPanel {
public:
    explicit GalleryPanel(pipeframe::ui::ProjectUIHost& ui) : ui(ui) {}

protected:
    pipeframe::ui::View BuildView() override {
        using namespace pipeframe::ui;
        return views::Column(
                   "gallery",
                   {views::Text("title", "DECLARATIVE PROJECT UI"),
                    views::Text("description", "Write a ViewPanel, then register it with project.UI<GalleryPanel>().")
                        .FitHeight(),
                    views::Row("counter",
                               {views::Text("count", std::to_string(count)), views::Button("increment", "INCREMENT",
                                                                                           [this] {
                                                                                               ++count;
                                                                                               RefreshView();
                                                                                           })}),
                    views::Button(
                        "dialog", "SHOW DIALOG",
                        [this] {
                            ui.ShowDialog(AlertDialog(
                                "example", "Managed dialog",
                                views::Text(
                                    "body",
                                    "The engine blocks input behind this dialog and cleans up its widgets on close.")
                                    .FitHeight(),
                                {views::Button("close", "CLOSE", [this] { ui.CloseDialog(); })}));
                        })})
            .Padding(16)
            .Spacing(12);
    }

private:
    pipeframe::ui::ProjectUIHost& ui;
    int count{};
};
class GalleryRuntime final : public pipeframe::SceneProjectRuntime {
public:
    GalleryRuntime() : SceneProjectRuntime("Declarative UI Gallery") {
        pipeframe::ProjectDefinition<GalleryWorld> project;
        using namespace pipeframe::ui;
        project.UI<GalleryPanel>({.size = {500, 280}, .placement = PanelPlacement::AtPercent(0, 0).Offset(16, 16)});
        project.UI<GalleryPanel>(
            {.title = "EXTRA PANEL",
             .size = {320, 280},
             .placement = PanelPlacement::AtPercent(100, 70).Anchor(PanelAnchor::CenterRight).Offset(-16, 0),
             .drawer = true});
        RegisterProject(project, world);
    }

private:
    GalleryWorld world;
};
}  // namespace
std::unique_ptr<pipeframe::ProjectRuntime> CreateGalleryProject() {
    return std::make_unique<GalleryRuntime>();
}

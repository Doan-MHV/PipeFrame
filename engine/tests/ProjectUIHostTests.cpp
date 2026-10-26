#include <PipeFrame/Backend/SFML/DashboardAccess.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/UI/AlertDialog.h>

#include <iostream>
using namespace pipeframe;
using namespace pipeframe::ui;
namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct World {
    int clicks{}, builds{}, updates{}, destroyed{};
    ViewPanel* panel{};
};
struct Panel final : ViewPanel {
    World& world;
    ProjectUIHost& host;
    Panel(World& world, ProjectUIHost& host) : world(world), host(host) { world.panel = this; }
    ~Panel() {
        ++world.destroyed;
        world.panel = nullptr;
    }
    void OnUpdate(float) override { ++world.updates; }
    View BuildView() override {
        ++world.builds;
        return views::Column("test-panel",
                             {views::Text("value", std::to_string(world.clicks)),
                              views::Button("increment", "INCREMENT",
                                            [this] {
                                                ++world.clicks;
                                                RefreshView();
                                            }),
                              views::TextField("input", "Input", "", [](const std::string&) {}),
                              views::Button("dialog", "DIALOG",
                                            [this] {
                                                host.ShowDialog(AlertDialog(
                                                    "dialog", "Example", views::Text("body", "Modal"),
                                                    {views::Button("close", "CLOSE", [this] { host.CloseDialog(); })}));
                                            })})
            .Padding(12)
            .Spacing(6);
    }
};
struct PlacedPanel final : ViewPanel {
    std::string key;
    explicit PlacedPanel(std::string key) : key(std::move(key)) {}
    View BuildView() override {
        return views::Column(key, {views::Button(key + "-hide", "HIDE", [this] { Hide(); })});
    }
};
struct Runtime final : SceneProjectRuntime {
    World world;
    Runtime() : SceneProjectRuntime("UI test") {
        ProjectDefinition<World> project;
        project.UI<Panel>({.size = {300, 250}});
        RegisterProject(project, world);
    }
    ~Runtime() { Unload(); }
};
Widget* Find(Widget& parent, std::string_view key) {
    if (parent.GetKey() == key) return &parent;
    for (std::size_t i = 0; i < parent.GetChildCount(); ++i)
        if (auto* found = Find(*parent.GetChild(i), key)) return found;
    return nullptr;
}
}  // namespace
int main() {
    try {
        sf::RenderTexture target({800, 600});
        auto context = backend::sfml::MakeRenderContext(target);
        Runtime runtime;
        std::string error;
        Check(runtime.Load({}, error), error.c_str());
        auto render = [&] { runtime.RenderScreen(context); };
        render();
        render();
        render();
        Check(runtime.world.updates == 3, "OnUpdate runs once per visible rendered frame");
        const int builds = runtime.world.builds;
        render();
        Check(runtime.world.builds == builds, "Unchanged panels should not rebuild every frame");
        runtime.world.panel->RefreshView();
        render();
        Check(runtime.world.builds > builds, "RefreshView reaches the mounted view");
        auto& root = backend::sfml::DashboardAccess::Host(*runtime.UI().Dashboard()).Root();
        auto click = [&](std::string_view key) {
            auto* widget = Find(root, key);
            Check(widget, "Missing panel control");
            const auto p = widget->GetScreenPosition() + widget->GetSize() * .5f;
            Vector2i point{int(p.x), int(p.y)};
            Check(runtime.ConsumesPointerAt(point, context), "Interactive panel should own pointer");
            runtime.HandleUIEvent({InputEventType::PointerMoved, PointerMoveInput{point}}, context);
            Check(runtime.HandleUIEvent({InputEventType::PointerPressed, PointerInput{PointerButton::Left, point}},
                                        context),
                  "Panel press consumed");
            render();
            Check(runtime.HandleUIEvent({InputEventType::PointerReleased, PointerInput{PointerButton::Left, point}},
                                        context),
                  "Panel release consumed");
            render();
        };
        click("increment");
        Check(runtime.world.clicks == 1, "Registered panel button callback runs exactly once");
        click("dialog");
        Check(runtime.UI().HasDialog() && runtime.HasUIFocus(), "Dialog owns keyboard focus");
        Check(runtime.HandleUIEvent({InputEventType::PointerPressed, PointerInput{PointerButton::Left, {2, 590}}},
                                    context),
              "Modal blocks clicks outside content");
        Check(runtime.world.clicks == 1, "Modal click cannot reach the background");
        runtime.HandleUIEvent({InputEventType::KeyPressed, KeyInput{InputKey::Escape}}, context);
        Check(!runtime.UI().HasDialog(), "Escape closes the modal without forwarding shortcuts");
        runtime.UI().ShowDialog(
            views::Button("modal-close", "CLOSE", [&] { runtime.UI().CloseDialog(); }).FillHeight());
        render();
        render();
        runtime.HandleUIEvent({InputEventType::PointerMoved, PointerMoveInput{{400, 300}}}, context);
        runtime.HandleUIEvent({InputEventType::PointerPressed, PointerInput{PointerButton::Left, {400, 300}}}, context);
        runtime.HandleUIEvent({InputEventType::PointerReleased, PointerInput{PointerButton::Left, {400, 300}}},
                              context);
        Check(!runtime.UI().HasDialog(), "Dialog action can close itself safely during dispatch");
        click("input");
        Check(runtime.HasUIFocus(), "Text field focus is forwarded automatically");
        runtime.SetViewMode(ProjectRuntimeViewMode::Zen);
        Check(!runtime.HasUIFocus() && !runtime.ConsumesPointerAt({400, 50}, context),
              "Hidden UI releases input and focus");
        runtime.SetViewMode(ProjectRuntimeViewMode::Editor);
        render();
        runtime.UI().ShowDialog(views::Text("pending", "Pending"));
        runtime.Unload();
        Check(runtime.world.destroyed == 1 && runtime.UI().PanelCount() == 0 && !runtime.UI().HasDialog(),
              "Unload destroys panels and pending modal callbacks");
        Check(runtime.Load({}, error), "Reload registration");
        render();
        Check(runtime.world.panel && runtime.UI().PanelCount() == 1, "Reload creates one fresh panel");
        runtime.Unload();
        Check(runtime.world.destroyed == 2, "Reloaded panel is destroyed exactly once");
        ProjectUIHost positioned;
        auto& pixel = positioned.Add(std::make_unique<PlacedPanel>("pixel"),
                                     {.size = {120, 100}, .placement = PanelPlacement::AtPixels(100, 80)});
        positioned.Add(std::make_unique<PlacedPanel>("percent"),
                       {.size = {200, 100},
                        .placement = PanelPlacement::AtPercent(100, 0).Anchor(PanelAnchor::TopRight).Offset(-16, 16)});
        std::vector<ViewPanel*> drawers;
        for (int i = 0; i < 4; ++i)
            drawers.push_back(&positioned.Add(
                std::make_unique<PlacedPanel>("drawer" + std::to_string(i)),
                {.title = "Drawer",
                 .size = {200, 120},
                 .placement = PanelPlacement::AtPercent(100, 50).Anchor(PanelAnchor::CenterRight).Offset(-16, 0),
                 .drawer = true}));
        positioned.Render(context);
        positioned.Render(context);
        auto& native = backend::sfml::DashboardAccess::Host(*positioned.Dashboard());
        Check(Find(native.Root(), "pixel")->GetScreenPosition() == sf::Vector2f{100, 80}, "Absolute pixel placement");
        Check(Find(native.Root(), "percent")->GetScreenPosition() == sf::Vector2f{584, 16},
              "Percentage placement with anchor and inset");
        auto& first = native.GetIndependentDrawer(0);
        Check(first.GetEdge() == DrawerEdge::Right, "Drawer infers closest edge");
        for (std::size_t i = 0; i < 4; ++i) {
            const auto a = native.GetIndependentDrawer(i).GetHandle().GetBounds();
            Check(a.position.x >= 0 && a.position.y >= 0 && a.position.x + a.size.x <= 800 &&
                      a.position.y + a.size.y <= 600,
                  "Collapsed handles stay in viewport");
            for (std::size_t j = 0; j < i; ++j)
                Check(!a.findIntersection(native.GetIndependentDrawer(j).GetHandle().GetBounds()),
                      "Same-edge handles do not overlap");
        }
        drawers[0]->Show();
        drawers[1]->Show();
        positioned.Render(context);
        Check(first.IsOpen() && native.GetIndependentDrawer(1).IsOpen(), "Positioned drawers open independently");
        Check(positioned.ContainsPoint({110, 90}), "Floating panel remains interactive with open drawers");
        drawers[0]->Hide();
        positioned.Render(context);
        Check(!first.IsOpen() && native.GetIndependentDrawer(1).IsOpen() && first.GetHandle().IsVisible(),
              "Panel-local Hide only collapses its own drawer");
        pixel.Hide();
        positioned.Render(context);
        Check(!positioned.ContainsPoint({110, 90}), "Floating Hide releases pointer area");
        pixel.Show();
        positioned.Render(context);
        Check(positioned.ContainsPoint({110, 90}), "Floating Show restores panel");
        sf::RenderTexture resized({1000, 700});
        auto resizedContext = backend::sfml::MakeRenderContext(resized);
        positioned.Render(resizedContext);
        Check(Find(native.Root(), "pixel")->GetScreenPosition() == sf::Vector2f{100, 80},
              "Pixel placement survives resize");
        Check(Find(native.Root(), "percent")->GetScreenPosition() == sf::Vector2f{784, 16},
              "Percentage placement follows resize");
        resizedContext.GetCamera().SetViewport({{.1f, .1f}, {.8f, .8f}});
        positioned.Render(resizedContext);
        Check(Find(native.Root(), "pixel")->GetScreenPosition() == sf::Vector2f{200, 150},
              "Pixel placement is relative to the scene viewport, not the window");
        Check(Find(native.Root(), "percent")->GetScreenPosition() == sf::Vector2f{684, 86},
              "Percentage placement uses the scene viewport dimensions");
        ProjectUIHost edges;
        const PanelPlacement placements[] = {PanelPlacement::AtPercent(0, 50).Anchor(PanelAnchor::CenterLeft),
                                             PanelPlacement::AtPercent(50, 0).Anchor(PanelAnchor::TopCenter),
                                             PanelPlacement::AtPercent(50, 100).Anchor(PanelAnchor::BottomCenter)};
        const DrawerEdge expected[] = {DrawerEdge::Left, DrawerEdge::Top, DrawerEdge::Bottom};
        for (int i = 0; i < 3; ++i)
            edges.Add(std::make_unique<PlacedPanel>("edge" + std::to_string(i)),
                      {.size = {200, 120}, .placement = placements[i], .drawer = true});
        edges.Add(std::make_unique<PlacedPanel>("override"), {.size = {200, 120},
                                                              .placement = PanelPlacement::AtPixels(100, 80),
                                                              .drawer = true,
                                                              .drawerEdge = DrawerEdge::Bottom});
        edges.Render(context);
        edges.Render(context);
        auto& edgeNative = backend::sfml::DashboardAccess::Host(*edges.Dashboard());
        for (int i = 0; i < 3; ++i)
            Check(edgeNative.GetIndependentDrawer(i).GetEdge() == expected[i], "Infer each viewport edge");
        Check(edgeNative.GetIndependentDrawer(3).GetEdge() == DrawerEdge::Bottom, "Explicit drawer edge override");
        // Click a collapsed handle, then the panel's own Hide button.
        auto sendClick = [&](Widget& widget) {
            auto p = widget.GetScreenPosition() + widget.GetSize() * .5f;
            Vector2i point{int(p.x), int(p.y)};
            edges.HandleEvent({InputEventType::PointerMoved, PointerMoveInput{point}}, context);
            edges.HandleEvent({InputEventType::PointerPressed, PointerInput{PointerButton::Left, point}}, context);
            edges.HandleEvent({InputEventType::PointerReleased, PointerInput{PointerButton::Left, point}}, context);
            edges.Render(context);
        };
        sendClick(edgeNative.GetIndependentDrawer(0).GetHandle());
        Check(edgeNative.GetIndependentDrawer(0).IsOpen(), "Collapsed handle reopens drawer through input routing");
        sendClick(*Find(edgeNative.Root(), "edge0-hide"));
        Check(!edgeNative.GetIndependentDrawer(0).IsOpen(),
              "Hide button collapses its own drawer through input routing");
        bool rejected = false;
        try {
            ProjectUIHost invalid;
            invalid.Add(std::make_unique<PlacedPanel>("bad"), {.placement = PanelPlacement::AtPercent(101, 0)});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected, "Invalid percentages rejected");
        std::cout << "Project UI registration, revisions, input, modal blocking, visibility and teardown passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

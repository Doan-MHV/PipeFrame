#include "Runtime/AntSimulationRuntime.h"
#include "Components/AntSimulationTypes.h"
#include "Runtime/SailBoatSimulationRuntime.h"
#include "Components/SailBoatSimulationTypes.h"
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <cstdlib>
#include <iostream>

void Require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
pipeframe::InputEvent ToInput(const sf::Event &event) {
    return *pipeframe::backend::sfml::FromBackend(event);
}
TextButton *Find(Widget &root, const std::string &caption) {
    if (auto *button = dynamic_cast<TextButton *>(&root))
        for (std::size_t i = 0; i < button->GetChildCount(); ++i)
            if (auto *label = dynamic_cast<Label *>(button->GetChild(i)); label && label->GetText() == caption)
                return button;
    for (std::size_t i = 0; i < root.GetChildCount(); ++i)
        if (auto *found = Find(*root.GetChild(i), caption))
            return found;
    return nullptr;
}
template <typename WidgetType>
WidgetType *FindWidget(Widget &root) {
    if (auto *match = dynamic_cast<WidgetType *>(&root))
        return match;
    for (std::size_t i = 0; i < root.GetChildCount(); ++i)
        if (auto *found = FindWidget<WidgetType>(*root.GetChild(i)))
            return found;
    return nullptr;
}
int main(int argc, char **argv) {
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Simulation dashboards");
    window.setVisible(false);
    RenderContext context(window);
    ant_simulation::AntSimulationRuntime ant;
    sailboat_simulation::SailBoatSimulationRuntime boat;
    std::string error;
    const auto scratch =
        std::filesystem::temp_directory_path() /
        ("pipeframe-16g-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(scratch);
    // Keep all user-facing save actions inside a disposable test project.
    const auto boatProject = scratch / "SailBoat";
    std::filesystem::create_directories(boatProject / "Assets" / "Races");
    for (const auto *folder : {"Textures", "Fonts", "Audio"})
        std::filesystem::create_directory_symlink(std::filesystem::path(PIPEFRAME_EXAMPLES) / "SailBoatSimulation" /
                                                      "Assets" / folder,
                                                  boatProject / "Assets" / folder);
    Require(ant.Load({std::filesystem::path(PIPEFRAME_EXAMPLES) / "AntSimulation"}, error), error);
    auto colony = ant.CreateDefaultObject(ant_simulation::ColonyTypeId);
    colony.id = 1;
    colony.properties[ant_simulation::InitialPopulationKey] = std::int64_t{4};
    colony.transform.position = {30, 30};
    ant.SynchronizeScene(std::array{colony});
    ant.Start();
    ant.FixedUpdate(0.016f);
    Require(boat.Load({boatProject}, error), error);
    auto start = boat.CreateDefaultObject(sailboat_simulation::RaceStartTypeId);
    start.id = 1;
    start.transform.position = {50, 50};
    auto finish = boat.CreateDefaultObject(sailboat_simulation::FinishLineTypeId);
    finish.id = 2;
    finish.transform.position = {500, 500};
    auto settings = boat.CreateDefaultObject(sailboat_simulation::TrainingSettingsTypeId);
    settings.id = 3;
    settings.properties[sailboat_simulation::PopulationSizeKey] = std::int64_t{8};
    settings.properties[sailboat_simulation::MaximumIterationTimeKey] = 0.1;
    boat.SynchronizeScene(std::array{start, finish, settings});
    boat.Start();
    boat.FixedUpdate(0.016f);
    for (int tick = 0; tick < 20; ++tick) {
        ant.FixedUpdate(0.016f);
        boat.FixedUpdate(0.1f);
    }
    boat.FixedUpdate(0.001f);
    Require(boat.GetPopulationTrainer().IsInitialized(), "Small training population must initialize");
    Require(boat.GetPopulationTrainer().GetGenerationTime() > 0.0f,
            "Playback must expose a non-zero live generation timer between rollovers");
    auto render = [&](pipeframe::ProjectRuntime &runtime) {
        context.BeginScreen();
        runtime.RenderScreen(context);
    };
    render(ant);
    render(boat);
    std::size_t clickCount = 0;
    auto click = [&](pipeframe::ProjectRuntime &runtime, Widget &widget) {
        ++clickCount;
        auto pixel = sf::Vector2i(widget.GetScreenPosition() + widget.GetSize() * 0.5f);
        Require(runtime.ConsumesPointerAt(pipeframe::backend::sfml::FromBackend(pixel), context),
                "Dashboard must claim control click " + std::to_string(clickCount));
        Require(runtime.HandleUIEvent(ToInput(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, pixel}), context),
                "UI press must be consumed for click " + std::to_string(clickCount));
        render(runtime); // A live data refresh between press/release must preserve capture.
        Require(runtime.HandleUIEvent(ToInput(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, pixel}), context),
                "UI release must be consumed for click " + std::to_string(clickCount));
    };
    auto action = [&](pipeframe::ProjectRuntime &runtime, SimulationDashboard &dashboard, const std::string &caption) {
        auto *button = Find(dashboard.Root(), caption);
        Require(button, "Missing action " + caption);
        EdgeDrawer *targetDrawer = nullptr;
        for (Widget *parent = button->GetParent(); parent; parent = parent->GetParent())
            if (auto *drawer = dynamic_cast<EdgeDrawer *>(parent)) {
                targetDrawer = drawer;
                break;
            }
        for (std::size_t index = 0; index < dashboard.GetIndependentDrawerCount(); ++index)
            if (&dashboard.GetIndependentDrawer(index) != targetDrawer)
                dashboard.GetIndependentDrawer(index).Close(false);
        if (targetDrawer)
            targetDrawer->Open(false);
        render(runtime);
        click(runtime, *button);
    };
    auto &antUI = *ant.GetDashboard();
    auto &boatUI = *boat.GetDashboard();
    Require(boatUI.GetPersistentMetricCount() == 2,
            "Wind and training time must remain visible outside drawers");
    Require(!boatUI.GetPersistentMetric(1).GetValueLabel().GetText().empty(),
            "The persistent training timer must show live elapsed/limit data");
    Require(boatUI.GetIndependentDrawerCount() == 8,
            "Every SailBoat workflow surface must have an independent drawer");
    auto &networkDrawer = boatUI.GetIndependentDrawer(4);
    networkDrawer.Open(false);
    render(boat);
    Require(!networkDrawer.GetHandle().IsVisible(),
            "An open popup must hide its collapsed white handle");
    for (std::size_t index = 0; index < boatUI.GetIndependentDrawerCount(); ++index)
        Require(!boatUI.GetIndependentDrawer(index).GetHandle().IsVisible(),
                "Collapsed drawer buttons must stay hidden behind a complete popup");
    auto *networkClose = Find(networkDrawer, "CLOSE");
    Require(networkClose, "Every complete popup must expose a close action");
    click(boat, *networkClose);
    Require(!networkDrawer.IsOpen(), "The popup close action must collapse its drawer");
    for (std::size_t index = 0; index < boatUI.GetIndependentDrawerCount(); ++index)
        Require(boatUI.GetIndependentDrawer(index).GetHandle().IsVisible(),
                "Closing a popup must restore every white drawer button");
    for (std::size_t first = 0; first < boatUI.GetIndependentDrawerCount(); ++first)
        for (std::size_t second = first + 1; second < boatUI.GetIndependentDrawerCount(); ++second) {
            const auto &a = boatUI.GetIndependentDrawer(first);
            const auto &b = boatUI.GetIndependentDrawer(second);
            if (a.GetEdge() == b.GetEdge())
                Require(!a.GetHandle().GetBounds().findIntersection(b.GetHandle().GetBounds()),
                        "Collapsed buttons on one edge must occupy distinct click targets");
        }
    auto &boatDrawer = boatUI.GetIndependentDrawer(3);
    click(boat, boatDrawer.GetHandle());
    Require(boatDrawer.IsOpen(),
            "A restored white drawer button must open through the runtime input path");
    boatUI.GetIndependentDrawer(3).Open(false);
    Require(!networkDrawer.IsOpen() && boatUI.GetIndependentDrawer(3).IsOpen(),
            "Opening a popup must close the previous popup before panels can overlap");
    networkDrawer.Open(false);
    render(boat);
    auto *liveNetwork = FindWidget<NetworkView>(networkDrawer);
    Require(liveNetwork && liveNetwork->GetNodes().size() >= 5,
            "The independent graph must expose four inputs, an output, and live values");
    action(boat, boatUI, "FOLLOW BEST OFF");
    render(boat);
    Require(Find(boatUI.Root(), "FOLLOW BEST ON") != nullptr,
            "Follow-best playback must remain connected to the dashboard action");
    action(boat, boatUI, "BEST ONLY OFF");
    Require(boat.GetConfiguration().drawBestOnly,
            "Best-only playback must update the live render configuration");
    action(ant, antUI, ant_simulation::AntEditorTool::GetModeName(ant_simulation::AntEditorToolMode::AddFood));
    Require(ant.GetEditorTool().GetMode() == ant_simulation::AntEditorToolMode::AddFood && ant.UsesRightClickTool(),
            "Ant brush command must stay connected during playback");
    action(boat, boatUI, "ADD WAYPOINT");
    Require(boat.UsesRightClickTool(), "Race tool must claim right clicks during playback");
    boat.HandleEvent(ToInput(sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {700, 500}}), context);
    Require(boat.ConsumeSceneEdits().size() == 1, "Race placement must emit one authored edit while playing");
    action(boat, boatUI, "CANCEL TOOL");
    Require(!boat.UsesRightClickTool(), "Cancel must restore Workbench context-menu routing");
    action(boat, boatUI, "SAVE CHECKPOINT");
    auto *modelTable = FindWidget<TableView>(boatUI.GetIndependentDrawer(6));
    Require(modelTable && !modelTable->GetRows().empty(), "Saved checkpoints must populate the model selector");
    const auto saved = modelTable->GetRows().front().id;
    Require(std::filesystem::exists(std::filesystem::path(saved) / "checkpoint.pftrain"),
            "Checkpoint action must persist model metadata");
    const auto row = sf::Vector2i(modelTable->GetScreenPosition() + sf::Vector2f{20, 52});
    boat.HandleUIEvent(ToInput(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, row}), context);
    render(boat);
    boat.HandleUIEvent(ToInput(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, row}), context);
    Require(modelTable->GetSelectedRow() == saved, "Live model table must preserve row clicks across frames");
    action(boat, boatUI, "LOAD SELECTED CHECKPOINT");
    Require(boat.GetPopulationTrainer().GetAgents().size() == 8,
            "Loading the selected checkpoint must retain its population");
    ant.HandleEvent(ToInput(sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {700, 500}}), context);
    Require(ant.HasWorldPointerCapture(), "Ant brush must capture its world stroke");
    ant.HandleEvent(ToInput(sf::Event::FocusLost{}), context);
    Require(!ant.HasWorldPointerCapture(), "Focus loss must cancel Ant brush capture");
    action(ant, antUI, "SELECT");
    const auto ants = ant.GetSimulationWorld()->GetAntStore().GetAnts();
    Require(!ants.empty(), "Ant selection fixture needs a live ant");
    const auto antPoint = context.WorldToScreen(
        pipeframe::backend::sfml::ToBackend(ants.front().GetPosition()));
    ant.HandleEvent(ToInput(sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, antPoint}), context);
    Require(ant.GetAntInspectorData().available, "Domain right click must select a live ant");
    action(ant, antUI, "FOLLOW OFF");
    ant.FixedUpdate(0.016f);
    Require(ant.GetAntInspectorData().follow, "Shared follow button must update the Ant inspector");
    auto &selectedAntDrawer = antUI.GetIndependentDrawer(3);
    if (selectedAntDrawer.IsOpen()) {
        auto *selectedClose = Find(selectedAntDrawer, "CLOSE");
        Require(selectedClose, "The selected-ant popup must expose a close action");
        click(ant, *selectedClose);
    }
    auto &antEditorDrawer = antUI.GetIndependentDrawer(0);
    click(ant, antEditorDrawer.GetHandle());
    Require(antEditorDrawer.IsOpen(), "The restored Ant editor button must open its popup");
    render(ant);
    auto *editorClose = Find(antEditorDrawer, "CLOSE");
    Require(editorClose, "The Ant editor popup must expose a close action");
    click(ant, *editorClose);
    antUI.Update(0.3f);
    Require(!ant.ConsumesPointerAt({150, 200}, context), "Closing the drawer must return its old bounds to the world");
    antEditorDrawer.Open(false);
    for (auto *runtime : std::array<pipeframe::ProjectRuntime *, 2>{&ant, &boat}) {
        runtime->SetViewMode(pipeframe::ProjectRuntimeViewMode::Zen);
        render(*runtime);
        Require(!runtime->ConsumesPointerAt({20, 20}, context), "Zen must release dashboard input");
        runtime->SetViewMode(pipeframe::ProjectRuntimeViewMode::Simulation);
    }
    for (const auto size : {sf::Vector2u{640, 720}, sf::Vector2u{1280, 720},
                            sf::Vector2u{1920, 1080}, sf::Vector2u{2560, 1440}}) {
        window.setSize(size);
        context.SetScreenSize(size);
        for (auto *runtime : std::array<pipeframe::ProjectRuntime *, 2>{&ant, &boat})
            render(*runtime);
        const sf::FloatRect viewport{{}, sf::Vector2f(size)};
        for (auto *dashboard : std::array<SimulationDashboard *, 2>{&antUI, &boatUI})
            for (std::size_t index = 0; index < dashboard->GetIndependentDrawerCount(); ++index)
                Require(dashboard->GetIndependentDrawer(index).GetHandle().GetBounds().findIntersection(viewport).has_value(),
                        "Every compact drawer handle must remain reachable after resize");
    }
    if (argc > 1) {
        const bool showBoat = argc < 3 || std::string(argv[2]) != "ant";
        auto &runtime =
            showBoat ? static_cast<pipeframe::ProjectRuntime &>(boat) : static_cast<pipeframe::ProjectRuntime &>(ant);
        auto &dashboard = showBoat ? boatUI : antUI;
        const unsigned width = argc > 3 ? std::stoul(argv[3]) : 1000;
        const unsigned height = argc > 5 ? std::stoul(argv[5]) : 800;
        window.setSize({width, height});
        context.SetScreenSize({width, height});
        const auto drawerIndex = argc > 4 ? std::stoul(argv[4]) : 0;
        for (std::size_t index = 0; index < dashboard.GetIndependentDrawerCount(); ++index)
            dashboard.GetIndependentDrawer(index).Close(false);
        if (drawerIndex < dashboard.GetIndependentDrawerCount()) {
            dashboard.GetIndependentDrawer(drawerIndex).Open(false);
        }
        dashboard.Update(0.3f);
        render(runtime);
        sf::RenderTexture target({width, height});
        target.clear(sf::Color(18, 20, 24));
        dashboard.Render(target);
        target.display();
        Require(target.getTexture().copyToImage().saveToFile(argv[1]), "Snapshot save");
    }
    boat.Unload();
    ant.Unload();
    std::filesystem::remove_all(scratch);
    std::cout << "Both simulation dashboards passed.\n";
}

#include "Runtime/SailBoatSimulationRuntime.h"
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Render/RenderContext.h>
#include <SFML/Graphics/Vertex.hpp>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>

namespace sailboat_simulation {
namespace {
std::string Number(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}
const UITheme &SailBoatTheme() {
    static const UITheme theme = [] {
        UITheme value;
        value.glassSurface = {66, 89, 105, 205};
        value.floatingSurface = {69, 93, 109, 218};
        value.elevatedSurface = {77, 99, 113, 220};
        value.border = {250, 250, 244, 180};
        value.subtleBorder = {232, 238, 238, 110};
        value.textPrimary = {252, 251, 244};
        value.textSecondary = {210, 218, 216};
        value.accent = {245, 202, 77};
        value.accentHovered = {255, 220, 103};
        value.accentPressed = {218, 172, 54};
        value.controlSelected = {201, 157, 48};
        value.radiusMedium = 12.0f;
        value.radiusLarge = 20.0f;
        value.shadow = {20, 37, 47, 115};
        value.shadowOffsetSmall = {0.0f, 5.0f};
        return value;
    }();
    return theme;
}
} // namespace
namespace {
// Wind geometry is project-specific; the surrounding card and layout are shared.
class WindIndicator final : public Widget {
  public:
    explicit WindIndicator(const SailBoatConfiguration &configuration) : configuration(configuration) {
        SetSize({0, 60});
        SetHitTestVisible(false);
    }

  protected:
    void OnRender(sf::RenderTarget &target) const override {
        const auto center = GetScreenPosition() + GetSize() * 0.5f;
        const float length = std::hypot(configuration.wind.x, configuration.wind.y);
        if (length <= 0)
            return;
        const sf::Vector2f direction{configuration.wind.x / length, configuration.wind.y / length};
        const sf::Vector2f normal{-direction.y, direction.x};
        const auto tip = center + direction * 24.f;
        const auto color = UITheme::Dark().accent;
        const sf::Vertex shaft[] = {{center - direction * 24.f, color}, {tip, color}};
        const sf::Vertex head[] = {{tip, color},
                                   {tip - direction * 10.f + normal * 6.f, color},
                                   {tip - direction * 10.f - normal * 6.f, color}};
        target.draw(shaft, 2, sf::PrimitiveType::Lines);
        target.draw(head, 3, sf::PrimitiveType::Triangles);
    }

  private:
    const SailBoatConfiguration &configuration;
};
} // namespace
void SailBoatSimulationRuntime::BuildDashboard() {
    const auto &font=*pipeframe::backend::sfml::GraphicsResourceAccess::Font(uiResources,hudFont);
    dashboard=std::make_unique<SimulationDashboard>(font,"SAILBOAT SIMULATION",SailBoatTheme());
    dashboard->SetTabbedDrawerVisible(false);
    windCard = &dashboard->AddFloatingMetric("WIND", 190.0f, 96.0f,
                                              DrawerAnchor::Start, DrawerAnchor::Start);
    auto &race = dashboard->AddIndependentDrawer(DrawerEdge::Left, "EDITOR", 290.0f, 500.0f,
                                                  DrawerAnchor::End, false);
    raceStatus = &dashboard->Text(race, "", 70);
    const std::array<const char *, 6> names{"ADD WAYPOINT",    "SET START", "SET FINISH",
                                            "CLEAR WAYPOINTS", "SAVE RACE", "LOAD RACE"};
    for (std::size_t i = 0; i < names.size(); ++i)
        raceActions[i] = &dashboard->Action(race, names[i], [this, i] { ExecuteRaceAction(i); });
    dashboard->Action(race, "CANCEL TOOL", [this] {
        raceEditorTool = RaceEditorTool::None;
        raceEditorPointerValid = false;
        raceEditorStatus = "Choose a tool, then right-click the water.";
    });
    auto &settings = dashboard->AddIndependentDrawer(DrawerEdge::Left, "SETTINGS", 270.0f, 260.0f,
                                                      DrawerAnchor::Center, false);
    workflowActions[0] = &dashboard->Action(settings, "FOLLOW BEST", [this] { followBestBoat = !followBestBoat; });
    workflowActions[1] = &dashboard->Action(
        settings, "BEST ONLY", [this] { configuration.drawBestOnly = !configuration.drawBestOnly; });
    workflowActions[2] = &dashboard->Action(settings, "WORLD", [this] { worldVisible = !worldVisible; });
    timerCard = &dashboard->AddFloatingMetric("ITERATION", 184.0f, 106.0f,
                                               DrawerAnchor::Start, DrawerAnchor::Center);
    auto &training = dashboard->AddIndependentDrawer(DrawerEdge::Right, "TRAINING RESULT", 300.0f, 240.0f,
                                                      DrawerAnchor::Start, true);
    resultCard = &dashboard->Metric(training, "TRAINING RESULTS");
    dashboard->Action(training, "NEW EXPLORATION", [this] {
        if (populationTrainer.IsInitialized()) {
            populationTrainer.StartNewExploration();
            renderer.ResetRaceProgress();
            renderer.ResetWaterSimulation();
            lastAudibleTarget = 0;
        }
    });
    performanceDetails = &dashboard->Text(training, "", 50);
    auto &boat = dashboard->AddIndependentDrawer(DrawerEdge::Right, "SELECTED BOAT", 300.0f, 210.0f,
                                                  DrawerAnchor::Center, true);
    boatCard = &dashboard->Metric(boat, "SELECTED BOAT");
    boatDetails = &dashboard->Text(boat, "", 90);
    auto &network = dashboard->AddIndependentDrawer(DrawerEdge::Right, "NETWORK", 720.0f, 280.0f,
                                                     DrawerAnchor::End, false);
    dashboard->Text(network, "LIVE NETWORK");
    networkView = &network.CreateChild<NetworkView>(font, SailBoatTheme());
    networkView->SetSize({0, 130});
    networkDetails = &dashboard->Text(network, "", 36);
    auto &history = dashboard->AddIndependentDrawer(DrawerEdge::Right, "HISTORY", 330.0f, 680.0f,
                                                     DrawerAnchor::Center, false);
    dashboard->Text(history, "BEST / AVERAGE SCORE");
    scoreChart = &history.CreateChild<TimeSeriesChart>();
    scoreChart->SetSize({0, 130});
    dashboard->Text(history, "COMPLETION RATE");
    completionChart = &history.CreateChild<TimeSeriesChart>();
    completionChart->SetSize({0, 110});
    completionChart->SetVerticalRange(ChartRange{0, 1});
    historyTable=&history.CreateChild<TableView>(font);
    historyTable->SetSize({0, 180});
    historyDetails = &dashboard->Text(history, "Select a generation to inspect its results.", 100);
    historyTable->SetOnSelectionChanged([this](auto id) {
        if (!id)
            return;
        const auto samples = populationTrainer.GetHistory().Samples();
        const auto index = std::stoull(*id);
        if (index >= samples.size())
            return;
        const auto &s = samples[index];
        historyDetails->SetText("Exploration " + std::to_string(s.run) + " | generation " +
                                std::to_string(s.iteration) + "\nDistance " + Number(s.Metric("distance")) + " | time " +
                                Number(s.Metric("race_time", -1.0)) + "\nCompletion " + Number(s.progress * 100) +
                                "%\nNodes " + std::to_string(s.modelNodeCount) + " | connections " +
                                std::to_string(s.modelConnectionCount));
    });
    auto &models = dashboard->AddIndependentDrawer(DrawerEdge::Left, "MODELS", 320.0f, 580.0f,
                                                    DrawerAnchor::Start, false);
    modelTable=&models.CreateChild<TableView>(font);
    modelTable->SetSize({0, 200});
    modelTable->SetOnSelectionChanged(
        [this](auto id) { selectedCheckpoint = id ? std::optional<std::filesystem::path>{*id} : std::nullopt; });
    dashboard->Action(models, "REFRESH SAVED RUNS", [this] { RefreshCheckpoints(); });
    dashboard->Action(models, "SAVE CHECKPOINT", [this] {
        std::string error;
        if (populationTrainer.SaveCheckpoint(trainingRunDirectory, error))
            checkpointStatus = "Checkpoint saved.";
        else
            checkpointStatus = error;
        RefreshCheckpoints();
    });
    dashboard->Action(models, "LOAD SELECTED CHECKPOINT", [this] {
        std::string error;
        if (!selectedCheckpoint) {
            checkpointStatus = "Select a saved run first.";
            return;
        }
        if (populationTrainer.LoadCheckpoint(*selectedCheckpoint, error)) {
            checkpointStatus = "Checkpoint loaded.";
            renderer.ResetRaceProgress();
            lastAudibleTarget = 0;
        } else
            checkpointStatus = error;
    });
    dashboard->Action(models, "EXPORT RUN ARTIFACTS", [this] {
        std::string error;
        checkpointStatus = populationTrainer.SaveRunArtifacts(error) ? "Run artifacts saved." : error;
    });
    modelStatus = &dashboard->Text(models, "", 100);
    auto &control = dashboard->AddIndependentDrawer(DrawerEdge::Bottom, "CONTROL", 120.0f, 230.0f,
                                                     DrawerAnchor::Center, false);
    dashboard->Action(control, "PLAY / PAUSE", [this] { playing = !playing; });
    RefreshCheckpoints();
}
void SailBoatSimulationRuntime::RefreshCheckpoints() {
    std::vector<TableRow> rows;
    std::error_code error;
    const auto root = projectDirectory / "Training" / "Runs";
    if (std::filesystem::exists(root, error))
        for (const auto &entry : std::filesystem::directory_iterator(root, error)) {
            if (entry.is_directory(error))
                rows.push_back({entry.path().string(), {entry.path().filename().string()}});
        }
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) { return a.id > b.id; });
    modelTable->SetData({{"Saved runs", 1}}, std::move(rows));
    selectedCheckpoint.reset();
}
void SailBoatSimulationRuntime::RefreshDashboard() {
    windCard->SetValueText(Number(std::hypot(configuration.wind.x, configuration.wind.y)) + " m/s");
    windCard->SetDetail(
        Number(std::atan2(configuration.wind.y, configuration.wind.x) * 180.f / std::numbers::pi_v<float>) +
        " degrees");
    raceStatus->SetText(raceEditorStatus);
    workflowActions[0]->SetText(followBestBoat ? "FOLLOW BEST ON" : "FOLLOW BEST OFF");
    workflowActions[1]->SetText(configuration.drawBestOnly ? "BEST ONLY ON" : "BEST ONLY OFF");
    workflowActions[2]->SetText(worldVisible ? "WORLD ON" : "WORLD OFF");
    timerCard->SetValueText(Number(populationTrainer.GetGenerationTime()) + " / " +
                            Number(configuration.maximumIterationTime) + " s");
    timerCard->SetDetail("Generation " + std::to_string(populationTrainer.GetGeneration() + 1) +
                         (playing ? " | PLAYING" : " | PAUSED"));
    const auto samples = populationTrainer.GetHistory().Samples();
    resultCard->SetValueText(Number(populationTrainer.GetBestScore()) + " score");
    if (!samples.empty()) {
        const auto &s = samples.back();
        resultCard->SetValueText(Number(s.bestScore) + " score");
        const double raceTime = s.Metric("race_time", -1.0);
        resultCard->SetDetail(Number(s.Metric("distance")) + " m | " +
                              (raceTime < 0 ? "NA" : Number(raceTime) + " s") + " | " +
                              Number(s.progress * 100) + "%");
    } else
        resultCard->SetDetail("No completed generations");
    ChartSeries score, average, completion;
    average.color = UITheme::Dark().textSecondary;
    std::vector<TableRow> rows;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto &s = samples[i];
        score.samples.push_back({static_cast<float>(i), s.bestScore});
        average.samples.push_back({static_cast<float>(i), s.averageScore});
        completion.samples.push_back({static_cast<float>(i), s.progress});
        rows.push_back({std::to_string(i),
                        {std::to_string(s.run) + "/" + std::to_string(s.iteration), Number(s.bestScore),
                         Number(s.progress * 100)}});
    }
    scoreChart->SetSeries({std::move(score), std::move(average)});
    completionChart->SetSeries({std::move(completion)});
    historyTable->SetData({{"Run/gen", 1}, {"Score", 1}, {"Done %", 1}}, std::move(rows));
    const auto *best = populationTrainer.GetBestAgent();
    if (best) {
        boatCard->SetValueText("Boat #" + std::to_string(best->GetId()));
        const auto &boat = best->GetBoat();
        const auto &task = best->GetTask();
        boatCard->SetDetail(boat.HasCrashed() ? "CRASHED" : task.HasFinished() ? "FINISHED" : "SAILING");
        boatDetails->SetText("Speed " + Number(boat.GetSpeed()) + " m/s\nDistance " + Number(task.GetRaceDistance()) +
                             " m\nScore " + Number(task.GetScore()) + "\nTarget " +
                             std::to_string(task.GetTargetIndex() + 1));
        const auto snapshot = best->GetNetwork().GetInferenceSnapshot();
        std::vector<NetworkNode> nodes;
        std::vector<NetworkEdge> edges;
        std::size_t input = 0, hidden = 0, output = 0;
        for (const auto &inferenceNode : snapshot.nodes) {
            sf::Vector2f position;
            if (inferenceNode.role == pipeframe::learning::NetworkNodeRole::Input)
                position = {0.1f, 0.15f + 0.2f * input++};
            else if (inferenceNode.role == pipeframe::learning::NetworkNodeRole::Output)
                position = {0.9f, 0.5f + 0.1f * output++};
            else
                position = {0.5f, 0.1f + 0.8f * hidden++ /
                    std::max<std::size_t>(1, snapshot.nodes.size() - input - output)};
            const float intensity = std::clamp(std::abs(inferenceNode.value), 0.15f, 1.0f);
            sf::Color color = UITheme::Dark().accent;
            color.a = static_cast<std::uint8_t>(intensity * 255.0f);
            std::string label;
            if (inferenceNode.role == pipeframe::learning::NetworkNodeRole::Input) {
                static const std::array<const char *, 4> labels{
                    "Projection Y", "Projection X", "Rel wind X", "Rel wind Y"};
                label = labels[std::min<std::size_t>(input - 1, labels.size() - 1)];
            } else if (inferenceNode.role == pipeframe::learning::NetworkNodeRole::Output) {
                label = "Angular velocity";
            } else {
                label = "Hidden " + std::to_string(inferenceNode.index);
            }
            nodes.push_back({inferenceNode.index, position, color, std::move(label), Number(inferenceNode.value)});
        }
        for (const auto &connection : snapshot.edges)
            if (connection.source < nodes.size() && connection.target < nodes.size() && connection.source != connection.target)
                edges.push_back({connection.source, connection.target,
                                 !connection.enabled ? UITheme::Dark().textDisabled :
                                 connection.value >= 0 ? sf::Color{85, 215, 180} : sf::Color{238, 108, 77}, true,
                                 std::clamp(std::abs(connection.value), 0.5f, 4.0f)});
        networkView->SetGraph(std::move(nodes), std::move(edges));
        networkDetails->SetText(std::to_string(snapshot.nodes.size()) + " nodes | " +
                                std::to_string(snapshot.edges.size()) + " connections | live activations");
    } else {
        boatCard->SetValueText("No trained boat");
        boatCard->SetDetail("");
        boatDetails->SetText("");
        networkView->SetGraph({}, {});
        networkDetails->SetText("Start training to inspect a network.");
    }
    performanceDetails->SetText("Simulation " + Number(lastSimulationTimeMs) + " ms\nRender " +
                                Number(lastRenderTimeMs) + " ms");
    modelStatus->SetText(checkpointStatus.empty() ? "Select a saved run, then load its checkpoint." : checkpointStatus);
}
void SailBoatSimulationRuntime::RenderScreen(RenderContext &context) {
    if (!loaded || !dashboard)
        return;
    dashboard->SetVisible(hudVisible && viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
    if (!dashboard->IsVisible())
        return;
    RefreshDashboard();
    dashboard->Layout(sf::FloatRect(context.GetWorldViewportBounds()));
    dashboard->Render(context.GetWindow());
}
bool SailBoatSimulationRuntime::ConsumesPointerAt(pipeframe::Vector2i point, const RenderContext &) const {
    const sf::Vector2i backendPoint = pipeframe::backend::sfml::ToBackend(point);
    return dashboard && hudVisible && viewMode != pipeframe::ProjectRuntimeViewMode::Zen &&
           dashboard->Contains(sf::Vector2f(backendPoint));
}
bool SailBoatSimulationRuntime::HandleUIEvent(const pipeframe::InputEvent &inputEvent, RenderContext &context) {
    const auto translated = pipeframe::backend::sfml::ToBackend(inputEvent);
    if (!translated) return false;
    const sf::Event &event = *translated;
    if (event.is<sf::Event::FocusLost>() || event.is<sf::Event::MouseLeft>())
        raceEditorPointerValid = false;
    if (const auto *key = event.getIf<sf::Event::KeyPressed>();
        key && key->code == sf::Keyboard::Key::Escape && raceEditorTool != RaceEditorTool::None) {
        raceEditorTool = RaceEditorTool::None;
        raceEditorPointerValid = false;
        raceEditorStatus = "Race tool cancelled.";
        return true;
    }
    if (!dashboard)
        return false;
    dashboard->SetVisible(hudVisible && viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
    dashboard->Layout(sf::FloatRect(context.GetWorldViewportBounds()));
    return dashboard->HandleEvent(event);
}
} // namespace sailboat_simulation

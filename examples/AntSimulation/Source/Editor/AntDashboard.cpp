#include <cmath>
#include "Editor/AntSelectionView.h"
#include <PipeFrame/Components/PlaygroundComponent.h>
#include "Runtime/AntSimulationRuntime.h"
#include <PipeFrame/UI/SchemaInspector.h>
#include <PipeFrame/UI/SimulationDashboard.h>
#include <PipeFrame/UI/View.h>
#include <iomanip>
#include <sstream>
namespace ant_simulation {
namespace {
std::string Number(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}
} // namespace
pipeframe::ui::View AntSimulationRuntime::BuildDashboardView(std::size_t panel) {
    using namespace pipeframe::ui;
    std::vector<View> content;
    if (panel == 6)
        return views::Card("time", "SIMULATION TIME", Number(simulationElapsedTime) + " s",
                           playing ? "PLAYING" : "PAUSED");
    if (!simulationWorld)
        return views::Text("unavailable", "Simulation unavailable");
    if (panel == 0) {
        content.push_back(views::Text("help", "LIVE TOOLS: right-drag to add food or edit the running world. No save or restart required. SELECT restores ant selection.").FitHeight());
        for (const auto mode : {AntEditorToolMode::None, AntEditorToolMode::Erase, AntEditorToolMode::AddFood,
                                AntEditorToolMode::AddWall})
            content.push_back(
                views::Button("mode:" + std::to_string(static_cast<int>(mode)),
                              mode == AntEditorToolMode::None ? "SELECT" : AntEditorTool::GetModeName(mode),
                              [this, mode] {
                                  if (simulationWorld)
                                      editorTool.CancelStroke(simulationWorld->GetEnvironment());
                                  editorTool.SetMode(mode);
                              })
                    .Selected(editorTool.GetMode() == mode));
        content.push_back(views::Text("radius-label", std::string(AntEditorTool::GetModeName(editorTool.GetMode())) +
                                                          " | radius " + Number(editorTool.GetRadius()))
                              .FitHeight());
        content.push_back(views::Slider("radius", editorTool.GetRadius(), AntEditorTool::MinimumRadius,
                                        AntEditorTool::MaximumRadius,
                                        [this](float value) { editorTool.SetRadius(value); }));
        content.push_back(views::NumberField("food-quantity","Live food per cell",float(editorTool.GetFoodQuantity()),[this](float value){
            if(std::isfinite(value)&&value>=1&&value<=10000&&value==std::floor(value))editorTool.SetFoodQuantity(std::size_t(value));
        }));
        content.push_back(views::Text("persistence","Live edits last until Reset. Use Assets / Maps for saved starting terrain and food.").FitHeight());
    } else if (panel == 1) {
        const auto option = [&](const char *key, const char *label, bool RenderOptions::*member) {
            content.push_back(views::Toggle(key, label, renderOptions.*member, [this, member](bool value) {
                renderOptions.*member = value;
                ApplyRenderOptions();
            }));
        };
        option("grid", "GRID", &RenderOptions::showGrid);
        option("markers", "MARKERS", &RenderOptions::showMarkers);
        option("shadows", "SHADOWS", &RenderOptions::showShadows);
        option("targets", "TARGETS", &RenderOptions::showTargets);
        option("physics", "PHYSICS", &RenderOptions::showPhysicsDebug);
        option("ants", "ANTS", &RenderOptions::showAnts);
        option("colors", "DYNAMIC COLORS", &RenderOptions::dynamicAntColors);
        content.push_back(views::Text("intensity-label", "Marker intensity").FitHeight());
        auto intensity = views::Slider("intensity", float(renderOptions.markerIntensity), 1, 20, [this](float value) {
            renderOptions.markerIntensity = int(value);
            ApplyRenderOptions();
        });
        intensity.step = 1;
        content.push_back(std::move(intensity));
    } else if (panel == 2) {
        const auto colonies = simulationWorld->GetColonyLifecycleSystem().GetColonies();
        const ColonyView *selected = nullptr;
        for (const auto &colony : colonies)
            if (selectedColony == colony.GetId())
                selected = &colony;
        if (!selected && !colonies.empty())
            selected = &colonies.front();
        for (const auto &colony : colonies)
            content.push_back(views::Button("colony:" + std::to_string(colony.GetId()),
                                            "Colony " + std::to_string(colony.GetId()) + " | " +
                                                std::to_string(colony.GetAntCount()) + " ants",
                                            [this, id = colony.GetId()] {
                                                selectedColony = id;
                                                colonyInspector.SetSelectedColony(id);
                                            })
                                  .Selected(selected && selected->GetId() == colony.GetId()));
        if (selected) {
            content.push_back(views::Card(
                "colony", "COLONY " + std::to_string(selected->GetId()), Number(selected->GetFoodQuantity()) + " food",
                "Rate " + Number(selected->GetCollectionRate()) + " | reserve " + Number(selected->GetReserve()) +
                    " | r " + Number(selected->GetRadius())));
            View::Series population, collection;
            population.color = selected->GetColor();
            if (auto history = histories.find(selected->GetId()); history != histories.end())
                for (const auto &sample : history->second.GetSamples()) {
                    population.samples.push_back({sample.elapsedTime, float(sample.antCount)});
                    collection.samples.push_back({sample.elapsedTime, sample.collectionRate});
                }
            content.push_back(views::Text("population-title", "POPULATION HISTORY").FitHeight());
            content.push_back(views::Chart("population", {std::move(population)}));
            content.push_back(views::Text("collection-title", "COLLECTION RATE / SECOND").FitHeight());
            content.push_back(views::Chart("collection", {std::move(collection)}));
        } else
            content.push_back(views::Text("empty", "No colony"));
    } else if (panel == 3) {
        const auto &ant = antInspector.GetData();
        content.push_back(views::Card("ant", "SELECTED ANT", ant.available ? ant.displayName : "No ant selected",
                                      ant.available ? ant.role + " | " + ant.state : ""));
        if (const auto *selected = antInspector.FindSelectedAnt(simulationWorld->GetAntQuery().GetAnts())) {
            previewGeometry.UpdateDetailed(*selected, 0, configuration);
            std::vector<View::MeshLayer> layers;
            const auto layer = [&](auto vertices, pipeframe::TextureHandle texture) {
                View::MeshLayer item;
                item.texture = texture;
                for (auto vertex : vertices) {
                    vertex.position = (vertex.position - selected->GetPosition()) * 0.42f;
                    item.vertices.push_back(vertex);
                }
                layers.push_back(std::move(item));
            };
            layer(previewGeometry.GetLegVertices(), previewLeg);
            layer(previewGeometry.GetFoodVertices(), previewFood);
            layer(previewGeometry.GetBodyVertices(), previewBody);
            content.push_back(views::Mesh("preview", std::move(layers), previewResources));
            content.push_back(views::Toggle("live-components", "LIVE COMPONENTS", showAntComponents,
                                            [this](bool value) { showAntComponents = value; }));
            if (showAntComponents)
                for (const auto &component : componentRegistry.Inspect(selected->GetObject())) {
                    const auto descriptors = componentRegistry.Describe();
                    const auto found = std::ranges::find(descriptors, component.typeId,
                                                         &pipeframe::SceneComponentTypeDescriptor::typeId);
                    if (found == descriptors.end())
                        continue;
                    auto schema = *found;
                    // Runtime ants are transient simulation instances; this panel exposes
                    // their actual storage as telemetry, never writes authoring history.
                    for (auto &property : schema.properties)
                        property.editable = false;
                    content.push_back(SchemaInspector("live:" + component.typeId, schema, component.properties, {}));
                }
        }
        content.push_back(AntSelectionView(antInspector));
        std::ostringstream out;
        if (ant.available)
            out << "Colony " << ant.colonyId << "\nPosition " << Number(ant.position.x) << ", "
                << Number(ant.position.y) << "\nTarget " << Number(ant.target.x) << ", " << Number(ant.target.y)
                << "\nEnergy " << Number(ant.energy) << "\nSpeed " << Number(ant.speed) << "\nTarget distance "
                << Number(ant.distanceToTarget) << "\nTravel " << Number(ant.totalTravelDistance) << "\nFood collected "
                << ant.collectedFood << "\nCarrying " << (ant.carryingFood ? "yes" : "no") << " | encounter "
                << (ant.inEncounter ? "yes" : "no") << "\n"
                << (ant.dead ? "DEAD" : "ALIVE");
        else
            out << "Right-click an ant with SELECT active.";
        content.push_back(views::Text("details", out.str()).FitHeight());
        content.push_back(views::Text("energy-label", "ENERGY").FitHeight());
        content.push_back(views::Progress("energy", ant.energyRatio));
        content.push_back(views::Text("speed-label", "SPEED").FitHeight());
        content.push_back(views::Progress("speed", ant.speedRatio));
        content.push_back(views::Text("blocked-label", "BLOCKED").FitHeight());
        content.push_back(views::Progress("blocked", ant.blockedRatio));
    } else if (panel == 4) {
        const auto &s = simulationWorld->GetStatistics();
        const auto &u = s.lastAntUpdate;
        std::ostringstream stats;
        stats << "Frame " << Number(dashboard->GetFrameTimeMs()) << " ms | FPS "
              << Number(1000.f / std::max(0.001f, dashboard->GetFrameTimeMs())) << "\nAnts " << s.antCount
              << " | colonies " << s.colonyCount << "\nBodies " << s.physicsBodyCount << "\nFood entities "
              << s.foodEntityCount << " | walls " << s.wallCount << "\nContacts " << u.antContacts
              << "\nWall constraints " << u.wallConstraints << "\nFuture collisions " << u.futureCollisions
              << "\nVisible " << renderStatistics.visible << " | vertices " << renderStatistics.vertices
              << "\nSimulation " << Number(renderStatistics.movementTimeMs) << " ms\nPreparation "
              << Number(u.preparationTimeMs) << " ms\nAvoidance " << Number(u.avoidanceTimeMs) << " ms\nPhysics "
              << Number(u.physicsTimeMs) << " ms\nBehavior " << Number(u.behaviorTimeMs) << " ms\nCleanup "
              << Number(u.cleanupTimeMs) << " ms\nGeometry " << Number(renderStatistics.geometryTimeMs) << " ms";

        content.push_back(views::Text("statistics", stats.str()).FitHeight());
    } else if (panel == 5)
        content.push_back(views::Button("play", playing ? "PAUSE" : "PLAY", [this] { playing = !playing; }));
    return views::Scroll("panel", views::Column("content", std::move(content)).Spacing(8).Padding(4)).FillHeight();
}
} // namespace ant_simulation

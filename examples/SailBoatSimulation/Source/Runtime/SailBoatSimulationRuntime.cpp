#include "Runtime/SailBoatSimulationRuntime.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <utility>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <PipeFrame/Backend/SFML/GeometryRenderer.h>
#include <PipeFrame/Render/RenderServices2D.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

#include "Components/SailBoatSimulationTypes.h"
#include "Systems/BoatMovementSystem.h"
#include "World/RaceCoursePersistence.h"
#include "Components/SailBoatComponentIds.h"
#include "Systems/SailBoatSystemIds.h"
#include "Runtime/SailBoatPluginDescriptor.h"
#include "Editor/SailBoatEditorExtensionIds.h"

namespace sailboat_simulation {
namespace {

constexpr sf::Color SeaColor{33, 150, 243};
constexpr sf::Color GridColor{255, 255, 255, 24};
constexpr sf::Color StartColor{6, 214, 160};
constexpr sf::Color FinishColor{238, 108, 77};
constexpr sf::Color WaypointColor{255, 209, 102};
constexpr sf::Color SelectionColor{255, 255, 255};

float DegreesToRadians(const float degrees) {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

float Distance(const pipeframe::Vector2f left, const pipeframe::Vector2f right) {
    const pipeframe::Vector2f difference = left - right;
    return std::sqrt(difference.x * difference.x + difference.y * difference.y);
}

} // namespace

const char *SailBoatSimulationRuntime::GetName() const { return "SailBoat Simulation"; }

pipeframe::ProjectPluginDescriptor SailBoatSimulationRuntime::GetPluginDescriptor() const {
    return contract::Descriptor();
}

bool SailBoatSimulationRuntime::RegisterPlugin(pipeframe::PluginRegistrar &registrar,
                                               std::string &error) {
    const std::array extensions{
        pipeframe::ExtensionDescriptor{contract::Dashboard, {}, "SailBoat Dashboard", pipeframe::ExtensionPoint::Drawer, "simulation"},
        pipeframe::ExtensionDescriptor{contract::RaceEditor, {}, "Race Editor", pipeframe::ExtensionPoint::Tool, "scene"},
        pipeframe::ExtensionDescriptor{contract::Network, {}, "Network View", pipeframe::ExtensionPoint::Panel, "simulation"},
        pipeframe::ExtensionDescriptor{contract::Telemetry, {}, "Race Telemetry", pipeframe::ExtensionPoint::TelemetryStream, "simulation"},
        pipeframe::ExtensionDescriptor{contract::RaceValidator, {}, "Race Validator", pipeframe::ExtensionPoint::ValidationRule, "project"}};
    for (auto extension : extensions) {
        extension.invoke=[this](const auto &){RefreshDashboard();RefreshCheckpoints();};
        if (!registrar.Extension(std::move(extension), &error)) return false;
    }
    if (!registrar.Action({"sailboat.toggle-dashboard", {}, "Toggle SailBoat Dashboard", "SailBoat",
                           {"simulation"}, "U", [this](const auto &) { hudVisible = !hudVisible; }}, &error)) return false;
    if (!registrar.Action({"sailboat.toggle-best", {}, "Toggle Best Boat Only", "SailBoat",
                           {"simulation"}, "B", [this](const auto &) { configuration.drawBestOnly = !configuration.drawBestOnly; }}, &error)) return false;
    pipeframe::ProjectSystemDescriptor sensing{contract::SensingSystem, {}, pipeframe::SystemPhase::FixedPrePhysics, {},
        {EnvironmentTypeId}, {EnvironmentTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {
            if(context.components)context.components->With(EnvironmentTypeId);
            if(auto *spatial=context.services->Find<pipeframe::UniformSpatialIndex<pipeframe::SceneObjectId>>())
                (void)spatial->QueryRadius({},512.0f);
            AdvancePreview(context.deltaTime);context.events->Publish({"sailboat.sensed",std::to_string(context.tick)});
            if(context.tick==1)context.log->Write("sailboat","scheduled sensing started");
        }};
    sensing.requiredServices={std::string(pipeframe::SpatialServiceId),std::string(pipeframe::EventServiceId),
        std::string(pipeframe::LogServiceId),std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(sensing),&error)) return false;
    pipeframe::ProjectSystemDescriptor training{contract::TrainingSystem, {}, pipeframe::SystemPhase::FixedBehavior,
        {contract::SensingSystem}, {TrainingSettingsTypeId}, {TrainingSettingsTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {
            (void)context.random->Stream(context.tick);
            context.jobs->Submit(context.tick,[this,delta=context.deltaTime]{AdvanceTraining(delta);});
        }};
    training.requiredServices={std::string(pipeframe::RandomServiceId),std::string(pipeframe::JobServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(training),&error)) return false;
    pipeframe::ProjectSystemDescriptor presentation{contract::PresentationSystem, {}, pipeframe::SystemPhase::FixedCleanup,
        {contract::TrainingSystem}, {TrainingSettingsTypeId}, {},false,false,false,
        [this](pipeframe::SystemContext &context) {AdvancePresentation(context.deltaTime);}};
    presentation.requiredServices={std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(presentation),&error)) return false;
    pipeframe::ProjectSystemDescriptor render{contract::RenderSystem, {}, pipeframe::SystemPhase::Render,
        {contract::PresentationSystem}, {EnvironmentTypeId}, {},false,false,false,
        [this](pipeframe::SystemContext &context) {
            (void)context.services->Find<pipeframe::GraphicsResourceService>();
            (void)context.services->Find<pipeframe::SurfaceRegistry>();
            if(auto *target=context.services->Find<RenderContext>())Render(*target);
        }};
    render.requiredServices={std::string(pipeframe::RenderServiceId),std::string(pipeframe::ResourceServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    return registrar.System(std::move(render), &error);
}

bool SailBoatSimulationRuntime::Load(const pipeframe::ProjectRuntimeContext &context, std::string &errorMessage) {
    errorMessage.clear();
    dashboard.reset();
    projectDirectory = context.projectDirectory;
    const auto runId = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    trainingRunDirectory = projectDirectory / "Training" / "Runs" /
                           ("run_" + std::to_string(runId));
    raceSavePath = projectDirectory / "Assets" / "Races" / "waypoints_save.pfrace";
    authoredObjects.clear();
    selectedObjectId.reset();
    configuration = {};
    raceCourse.Clear();
    loaded = true;
    playing = false;
    cameraInitialized = false;
    previewBoatInitialized = false;
    elapsedSimulationTime = 0.0f;
    std::string ignoredAssetError;
    renderer.LoadAssets(projectDirectory / "Assets", ignoredAssetError);
    hudFont=uiResources.LoadFont((projectDirectory/"Assets"/"Fonts"/"roboto_regular.ttf").string(),&ignoredAssetError);
    hudFontLoaded=uiResources.State(hudFont)==pipeframe::ResourceState::Ready;
    markSound=audio.LoadClip((projectDirectory/"Assets"/"Audio"/"bubble_2.wav").string(),&ignoredAssetError);
    markSoundLoaded=audio.State(markSound)==pipeframe::ResourceState::Ready;
    audio.SetMasterVolume(configuration.audioVolume/100.0f);
    if(hudFontLoaded) BuildDashboard();
    hudVisible = true;
    worldVisible = true;
    followBestBoat = false;
    lastAudibleTarget = 0;
    lastSimulationTimeMs = 0.0f;
    lastRenderTimeMs = 0.0f;
    return true;
}

std::span<const pipeframe::SceneObjectTypeDescriptor> SailBoatSimulationRuntime::GetSceneObjectTypes() const {
    return objectTypes;
}

std::span<const pipeframe::SceneComponentTypeDescriptor> SailBoatSimulationRuntime::GetSceneComponentTypes() const {
    static const auto types=CreateComponentTypes(CreateObjectTypes());
    return types;
}

pipeframe::SceneObjectData
SailBoatSimulationRuntime::CreateDefaultObject(const pipeframe::SceneObjectTypeId &typeId) const {
    pipeframe::SceneObjectData object = ProjectRuntime::CreateDefaultObject(typeId);

    if (typeId == RaceStartTypeId) {
        object.name = "RACE START";
        object.transform.position = {500.0f, 800.0f};
        object.transform.rotation = -90.0f;
    } else if (typeId == FinishLineTypeId) {
        object.name = "FINISH LINE";
        object.transform.position = {1100.0f, 800.0f};
        object.transform.rotation = 90.0f;
    } else if (typeId == WaypointTypeId) {
        object.name = "WAYPOINT";
        object.transform.position = {800.0f, 500.0f};
    } else if (typeId == EnvironmentTypeId) {
        object.name = "SAILING ENVIRONMENT";
    } else if (typeId == TrainingSettingsTypeId) {
        object.name = "TRAINING SETTINGS";
    }

    if(auto component=std::ranges::find(object.components,typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()){
        for(const auto &[key,value]:object.properties)component->properties.insert_or_assign(key,value);
        object.properties.clear();
    }
    return object;
}

void SailBoatSimulationRuntime::SynchronizeScene(std::span<const pipeframe::SceneObjectData> objects) {
    authoredObjects.assign(objects.begin(), objects.end());
    RebuildDomainState();
    cameraInitialized = false;
}

void SailBoatSimulationRuntime::SetSelectedObject(const std::optional<pipeframe::SceneObjectId> objectId) {
    selectedObjectId = objectId;
}

void SailBoatSimulationRuntime::SetViewMode(const pipeframe::ProjectRuntimeViewMode mode) { viewMode = mode; }

std::optional<pipeframe::SceneObjectId>
SailBoatSimulationRuntime::HitTest(const pipeframe::Vector2f worldPosition) const {
    for (auto iterator = authoredObjects.rbegin(); iterator != authoredObjects.rend(); ++iterator) {
        if (iterator->typeId == RaceStartTypeId || iterator->typeId == FinishLineTypeId) {
            if (MakeSegment(*iterator, 80.0f).DistanceTo(worldPosition, false) <= 12.0f) {
                return iterator->id;
            }
        } else if (iterator->typeId == WaypointTypeId) {
            const float radius = std::max(6.0f, static_cast<float>(ReadNumber(*iterator, WaypointRadiusKey, 10.0)));
            if (Distance(iterator->transform.position, worldPosition) <= radius + 6.0f) {
                return iterator->id;
            }
        }
    }

    return std::nullopt;
}

void SailBoatSimulationRuntime::Start() { playing = true; }

void SailBoatSimulationRuntime::FixedUpdate(const float fixedDeltaTime) {
    if (!playing) {
        return;
    }
    const auto updateStart = std::chrono::steady_clock::now();
    AdvancePreview(fixedDeltaTime);
    AdvanceTraining(fixedDeltaTime);
    AdvancePresentation(fixedDeltaTime);
    lastSimulationTimeMs = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - updateStart).count();
}

void SailBoatSimulationRuntime::AdvancePreview(const float fixedDeltaTime) {
    if (!playing || fixedDeltaTime <= 0.0f) return;
    const auto started=std::chrono::steady_clock::now();
    lastSimulationTimeMs=0.0f;
    elapsedSimulationTime += fixedDeltaTime;
    if (previewBoatInitialized) {
        const SailBoatRaceTask::NeuralInputs inputs =
            previewRaceTask.BuildNeuralInputs(previewBoat, boatEnvironment, raceCourse);
        const float previewRudderCommand = std::clamp(inputs[1] * 2.0f, -1.0f, 1.0f);
        previewRaceTask.Update(previewBoat, boatEnvironment, raceCourse,
                               previewRudderCommand, configuration.angularSpeedDegrees,
                               fixedDeltaTime);
    }
    lastSimulationTimeMs+=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-started).count();
}

void SailBoatSimulationRuntime::AdvanceTraining(const float fixedDeltaTime) {
    if (!playing || fixedDeltaTime <= 0.0f) return;
    const auto started=std::chrono::steady_clock::now();
    populationTrainer.Update(fixedDeltaTime);
    lastSimulationTimeMs+=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-started).count();
}

void SailBoatSimulationRuntime::AdvancePresentation(const float fixedDeltaTime) {
    if (!playing || fixedDeltaTime <= 0.0f) return;
    const auto started=std::chrono::steady_clock::now();
    if (const SailBoatAgent *bestAgent = populationTrainer.GetBestAgent(); bestAgent != nullptr) {
        const std::size_t targetIndex = bestAgent->GetTask().GetTargetIndex();
        renderer.SetRaceProgress(targetIndex);
        UpdateMarkAudio(targetIndex);
    }
    renderer.Advance(fixedDeltaTime);
    lastSimulationTimeMs+=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-started).count();
}

void SailBoatSimulationRuntime::Render(RenderContext &context) {
    if (!loaded) {
        return;
    }

    const auto renderStart = std::chrono::steady_clock::now();
    if (!cameraInitialized) {
        FitCamera(context);
    }

    sf::RenderTarget &target = context.GetWindow();
    const std::span<const SailBoatAgent> agents = populationTrainer.GetAgents();
    const SailBoatAgent *bestAgent = populationTrainer.GetBestAgent();
    if (followBestBoat && playing && bestAgent != nullptr) {
        context.GetCamera().SetCenter(pipeframe::backend::sfml::ToBackend(bestAgent->GetBoat().GetPosition()));
        context.BeginWorld();
    }

    if (!worldVisible) {
        lastRenderTimeMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - renderStart).count();
        return;
    }

    renderer.DrawWater(target, pipeframe::backend::sfml::ToBackend(configuration.worldSize),
                       pipeframe::backend::sfml::ToBackend(configuration.wind),
                       configuration.waterAnimation, agents,
                       populationTrainer.GetBestAgentIndex(),
                       configuration.drawGhostBoats || configuration.drawBestOnly,
                       playing);

    constexpr float GridSpacing = 100.0f;
    for (float x = 0.0f; x <= configuration.worldSize.x; x += GridSpacing) {
        const sf::Vertex line[] = {{{x, 0.0f}, GridColor}, {{x, configuration.worldSize.y}, GridColor}};
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
    for (float y = 0.0f; y <= configuration.worldSize.y; y += GridSpacing) {
        const sf::Vertex line[] = {{{0.0f, y}, GridColor}, {{configuration.worldSize.x, y}, GridColor}};
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }

    const SailBoatRaceTask &displayTask = bestAgent != nullptr ? bestAgent->GetTask() : previewRaceTask;
    renderer.DrawCourse(target, raceCourse, displayTask, configuration.drawWaypointLabels);
    DrawRaceEditorPreview(target);
    if (viewMode == pipeframe::ProjectRuntimeViewMode::Editor && selectedObjectId.has_value()) {
        if (const pipeframe::SceneObjectData *selected = FindObject(*selectedObjectId); selected != nullptr) {
            DrawRaceObject(target, *selected);
        }
    }

    if (!agents.empty()) {
        if (bestAgent != nullptr) {
            if (configuration.drawBestTrajectory) {
                renderer.DrawTrajectory(target, bestAgent->GetBoat());
            }
            if (configuration.drawBestOnly) {
                renderer.DrawPopulation(target, std::span<const SailBoatAgent>{bestAgent, 1},
                                        0, false, configuration.highlightBest);
            } else {
                renderer.DrawPopulation(target, agents, populationTrainer.GetBestAgentIndex(),
                                        configuration.drawGhostBoats,
                                        configuration.highlightBest);
            }
        }
    } else {
        DrawPreviewTrajectory(target);
        DrawPreviewBoat(target);
    }
    if (configuration.showTargetGuide && bestAgent != nullptr) {
        renderer.DrawNextTarget(target, bestAgent->GetBoat(), bestAgent->GetTask(), raceCourse);
    } else if (configuration.showTargetGuide && previewBoatInitialized) {
        renderer.DrawNextTarget(target, previewBoat, previewRaceTask, raceCourse);
    }
    lastRenderTimeMs = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - renderStart).count();
}

void SailBoatSimulationRuntime::HandleEvent(const pipeframe::InputEvent &inputEvent, RenderContext &context) {
    if (!loaded) {
        return;
    }
    if (inputEvent.type == pipeframe::InputEventType::KeyPressed) {
        const auto *key = inputEvent.GetIf<pipeframe::KeyInput>();
        if (!key) return;
        HandleShortcut(*key);
        return;
    }
    if (viewMode == pipeframe::ProjectRuntimeViewMode::Zen) {
        return;
    }
    if (inputEvent.type == pipeframe::InputEventType::PointerMoved) {
        const auto *moved = inputEvent.GetIf<pipeframe::PointerMoveInput>();
        if (!moved) return;
        raceEditorPointerWorld = pipeframe::backend::sfml::FromBackend(
            context.ScreenToWorld(pipeframe::backend::sfml::ToBackend(moved->position)));
        raceEditorPointerValid = true;
    } else if (inputEvent.type == pipeframe::InputEventType::PointerPressed) {
        const auto *pressed = inputEvent.GetIf<pipeframe::PointerInput>();
        if (pressed && pressed->button == pipeframe::PointerButton::Right) {
            HandleRaceEditorRightClick(pipeframe::backend::sfml::FromBackend(
                context.ScreenToWorld(pipeframe::backend::sfml::ToBackend(pressed->position))));
        }
    }
}

void SailBoatSimulationRuntime::HandleShortcut(const pipeframe::KeyInput &event) {
    if (event.alt || event.control || event.system) {
        return;
    }
    if (event.key == pipeframe::InputKey::U) {
        hudVisible = !hudVisible;
    } else if (event.key == pipeframe::InputKey::B) {
        configuration.drawBestOnly = !configuration.drawBestOnly;
    } else if (event.key == pipeframe::InputKey::D) {
        worldVisible = !worldVisible;
    } else if (event.key == pipeframe::InputKey::F) {
        followBestBoat = !followBestBoat;
    } else if (event.key == pipeframe::InputKey::R && playing && populationTrainer.IsInitialized()) {
        populationTrainer.StartNewExploration();
        renderer.ResetRaceProgress();
        renderer.ResetWaterSimulation();
        lastAudibleTarget = 0;
    }
}

void SailBoatSimulationRuntime::UpdateMarkAudio(const std::size_t targetIndex) {
    if (targetIndex < lastAudibleTarget) {
        lastAudibleTarget = targetIndex;
        return;
    }
    if (targetIndex > lastAudibleTarget && !configuration.asyncTraining &&
        configuration.audioEnabled && markSoundLoaded) {
        audio.SetMasterVolume(configuration.audioVolume/100.0f);
        audio.Play(markSound);
    }
    lastAudibleTarget = targetIndex;
}

void SailBoatSimulationRuntime::DrawRaceEditorPreview(sf::RenderTarget &target) const {
    if (viewMode == pipeframe::ProjectRuntimeViewMode::Zen || !raceEditorPointerValid ||
        raceEditorTool == RaceEditorTool::None) {
        return;
    }

    sf::Color color = WaypointColor;
    sf::Vector2f origin = pipeframe::backend::sfml::ToBackend(raceEditorPointerWorld);
    bool drawDirection = false;
    if (raceEditorTool == RaceEditorTool::SetStartFirst || raceEditorTool == RaceEditorTool::SetStartDirection) {
        color = StartColor;
    } else if (raceEditorTool == RaceEditorTool::SetFinishFirst ||
               raceEditorTool == RaceEditorTool::SetFinishDirection) {
        color = FinishColor;
    }
    if (raceEditorTool == RaceEditorTool::SetStartDirection ||
        raceEditorTool == RaceEditorTool::SetFinishDirection) {
        origin = pipeframe::backend::sfml::ToBackend(raceEditorFirstPoint);
        drawDirection = true;
    }

    if (drawDirection) {
        const sf::Vertex line[] = {{{origin}, sf::Color{color.r, color.g, color.b, 180}},
                                   {pipeframe::backend::sfml::ToBackend(raceEditorPointerWorld), color}};
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
    sf::CircleShape point(8.0f);
    point.setOrigin({8.0f, 8.0f});
    point.setPosition(drawDirection ? origin : pipeframe::backend::sfml::ToBackend(raceEditorPointerWorld));
    point.setFillColor(sf::Color{color.r, color.g, color.b, 110});
    point.setOutlineColor(color);
    point.setOutlineThickness(2.0f);
    target.draw(point);
}

std::vector<pipeframe::ProjectRuntimeSceneEdit> SailBoatSimulationRuntime::ConsumeSceneEdits() {
    std::vector<pipeframe::ProjectRuntimeSceneEdit> edits = std::move(pendingSceneEdits);
    pendingSceneEdits.clear();
    return edits;
}

pipeframe::ProjectRuntimeStatistics SailBoatSimulationRuntime::GetStatistics() const {
    const SailBoatRenderStatistics &render = renderer.GetStatistics();
    const std::size_t agentCount = populationTrainer.GetAgents().size();
    const bool hasRenderedPopulation = worldVisible && agentCount != 0;
    return {
        .available = loaded,
        .usingQuads = true,
        .candidateObjectCount = agentCount,
        .visibleObjectCount = hasRenderedPopulation ? render.visibleBoats : 0,
        .vertexCount = hasRenderedPopulation ? render.boatVertices + render.trajectoryVertices : 0,
        .movementTimeMs = lastSimulationTimeMs,
        .spatialGridTimeMs = 0.0f,
        .geometryTimeMs = lastRenderTimeMs,
    };
}

void SailBoatSimulationRuntime::ExecuteRaceAction(const std::size_t index) {
    if (index == 0) {
        raceEditorTool = RaceEditorTool::AddWaypoint;
        raceEditorStatus = "Right-click to place an ordered mark.";
    } else if (index == 1) {
        raceEditorTool = RaceEditorTool::SetStartFirst;
        raceEditorStatus = "Right-click start position, then direction.";
    } else if (index == 2) {
        raceEditorTool = RaceEditorTool::SetFinishFirst;
        raceEditorStatus = "Right-click finish point, then gate direction.";
    } else if (index == 3) {
        pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::RemoveObjectType,
                                     WaypointTypeId, {}});
        raceEditorTool = RaceEditorTool::None;
        raceEditorStatus = "All waypoint marks cleared. Undo is available.";
    } else if (index == 4) {
        std::string error;
        if (RaceCoursePersistence::Save(raceSavePath, authoredObjects, error)) {
            raceEditorStatus = "Race saved to Assets/Races/waypoints_save.pfrace.";
        } else {
            raceEditorStatus = error;
        }
    } else if (index == 5) {
        std::vector<pipeframe::SceneObjectData> objects;
        std::string error;
        if (RaceCoursePersistence::Load(raceSavePath, objects, error)) {
            QueueLoadedRace(std::move(objects));
            raceEditorStatus = "Saved race loaded. Undo restores the current race.";
        } else {
            raceEditorStatus = error;
        }
    }
}

void SailBoatSimulationRuntime::HandleRaceEditorRightClick(const pipeframe::Vector2f worldPosition) {
    if (raceEditorTool == RaceEditorTool::AddWaypoint) {
        pipeframe::SceneObjectData object = CreateDefaultObject(WaypointTypeId);
        object.transform.position = worldPosition;
        const std::int64_t order = static_cast<std::int64_t>(raceCourse.GetWaypoints().size() + 1);
        object.name = "WAYPOINT " + std::to_string(order);
        object.properties.insert_or_assign(WaypointOrderKey, order);
        pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::CreateObject, {}, std::move(object)});
        raceEditorStatus = "Waypoint " + std::to_string(order) + " placed.";
    } else if (raceEditorTool == RaceEditorTool::SetStartFirst) {
        raceEditorFirstPoint = worldPosition;
        raceEditorTool = RaceEditorTool::SetStartDirection;
        raceEditorStatus = "Right-click again to set the start direction.";
    } else if (raceEditorTool == RaceEditorTool::SetStartDirection) {
        QueueDirectedObject(RaceStartTypeId, raceEditorFirstPoint, worldPosition);
        raceEditorTool = RaceEditorTool::None;
        raceEditorStatus = "Race start replaced. Undo is available.";
    } else if (raceEditorTool == RaceEditorTool::SetFinishFirst) {
        raceEditorFirstPoint = worldPosition;
        raceEditorTool = RaceEditorTool::SetFinishDirection;
        raceEditorStatus = "Right-click again to set the finish gate direction.";
    } else if (raceEditorTool == RaceEditorTool::SetFinishDirection) {
        QueueDirectedObject(FinishLineTypeId, raceEditorFirstPoint, worldPosition);
        raceEditorTool = RaceEditorTool::None;
        raceEditorStatus = "Finish line replaced. Undo is available.";
    }
}

void SailBoatSimulationRuntime::QueueDirectedObject(const pipeframe::SceneObjectTypeId &typeId,
                                                    const pipeframe::Vector2f first, const pipeframe::Vector2f second) {
    pipeframe::SceneObjectData object = CreateDefaultObject(typeId);
    const pipeframe::Vector2f difference = second - first;
    const float length = std::max(1.0f, std::sqrt(difference.x * difference.x + difference.y * difference.y));
    object.transform.position = first;
    object.transform.rotation = std::atan2(difference.y, difference.x) * 180.0f / std::numbers::pi_v<float>;
    object.properties.insert_or_assign(LineLengthKey, static_cast<double>(length));
    pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::ReplaceObjectType,
                                 typeId, std::move(object)});
}

void SailBoatSimulationRuntime::QueueLoadedRace(std::vector<pipeframe::SceneObjectData> objects) {
    pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::RemoveObjectType, RaceStartTypeId, {}});
    pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::RemoveObjectType, FinishLineTypeId, {}});
    pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::RemoveObjectType, WaypointTypeId, {}});
    for (pipeframe::SceneObjectData &object : objects) {
        pendingSceneEdits.push_back({pipeframe::ProjectRuntimeSceneEditKind::CreateObject, {}, std::move(object)});
    }
}

void SailBoatSimulationRuntime::Reset() {
    elapsedSimulationTime = 0.0f;
    playing = false;
    lastAudibleTarget = 0;
    audio.StopAll();
    RebuildDomainState();
}

void SailBoatSimulationRuntime::Stop() {
    playing = false;
    audio.StopAll();
    renderer.ResetWaterSimulation();
}

void SailBoatSimulationRuntime::Unload() {
    populationTrainer.Clear();
    renderer.UnloadAssets();
    audio.StopAll();
    markSound={};
    authoredObjects.clear();
    selectedObjectId.reset();
    projectDirectory.clear();
    trainingRunDirectory.clear();
    raceSavePath.clear();
    pendingSceneEdits.clear();
    raceEditorTool = RaceEditorTool::None;
    raceCourse.Clear();
    loaded = false;
    dashboard.reset();
    playing = false;
    cameraInitialized = false;
    previewBoatInitialized = false;
    hudFontLoaded = false;
    markSoundLoaded = false;
    hudVisible = true;
    worldVisible = true;
    followBestBoat = false;
    lastAudibleTarget = 0;
    lastSimulationTimeMs = 0.0f;
    lastRenderTimeMs = 0.0f;
    elapsedSimulationTime = 0.0f;
}

const SailBoatConfiguration &SailBoatSimulationRuntime::GetConfiguration() const { return configuration; }
const RaceCourse &SailBoatSimulationRuntime::GetRaceCourse() const { return raceCourse; }
const Boat &SailBoatSimulationRuntime::GetPreviewBoat() const { return previewBoat; }
const SailBoatRaceTask &SailBoatSimulationRuntime::GetPreviewRaceTask() const { return previewRaceTask; }
const SailBoatPopulationTrainer &SailBoatSimulationRuntime::GetPopulationTrainer() const {
    return populationTrainer;
}
bool SailBoatSimulationRuntime::HasPreviewBoat() const { return previewBoatInitialized; }
bool SailBoatSimulationRuntime::IsPlaying() const { return playing; }

std::vector<pipeframe::SceneObjectTypeDescriptor> SailBoatSimulationRuntime::CreateObjectTypes() {
    pipeframe::SceneObjectTypeDescriptor start;
    start.typeId = RaceStartTypeId;
    start.displayName = "RACE START";
    start.properties.push_back({LineLengthKey, "DIRECTION LENGTH", pipeframe::PropertyKind::Number, 80.0, true});

    pipeframe::SceneObjectTypeDescriptor finish;
    finish.typeId = FinishLineTypeId;
    finish.displayName = "FINISH LINE";
    finish.properties.push_back({LineLengthKey, "LINE LENGTH", pipeframe::PropertyKind::Number, 100.0, true});

    pipeframe::SceneObjectTypeDescriptor waypoint;
    waypoint.typeId = WaypointTypeId;
    waypoint.displayName = "WAYPOINT";
    waypoint.properties.push_back({LineLengthKey, "DIRECTION LENGTH", pipeframe::PropertyKind::Number, 60.0, true});
    waypoint.properties.push_back({WaypointRadiusKey, "REACH RADIUS", pipeframe::PropertyKind::Number, 10.0, true});
    waypoint.properties.push_back(
        {WaypointOrderKey, "RACE ORDER", pipeframe::PropertyKind::Integer, std::int64_t{1}, true});

    pipeframe::SceneObjectTypeDescriptor environment;
    environment.typeId = EnvironmentTypeId;
    environment.displayName = "SAILING ENVIRONMENT";
    environment.properties.push_back(
        {WorldSizeKey, "WORLD SIZE", pipeframe::PropertyKind::Vector2, pipeframe::Vector2f{1600.0f, 1600.0f}, true});
    environment.properties.push_back(
        {WindDirectionKey, "WIND DIRECTION", pipeframe::PropertyKind::Number, 0.0, true});
    environment.properties.push_back({WindSpeedKey, "WIND SPEED", pipeframe::PropertyKind::Number, 1.0, true});
    environment.properties.push_back(
        {WaterAnimationKey, "WATER ANIMATION", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {DrawBestOnlyKey, "BEST BOAT ONLY", pipeframe::PropertyKind::Boolean, false, true});
    environment.properties.push_back(
        {DrawGhostBoatsKey, "GHOST BOATS", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {HighlightBestKey, "HIGHLIGHT BEST", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {DrawBestTrajectoryKey, "BEST TRAJECTORY", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {DrawWaypointLabelsKey, "WAYPOINT LABELS", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {ShowTargetGuideKey, "TARGET GUIDE", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {AudioEnabledKey, "MARK SOUND", pipeframe::PropertyKind::Boolean, true, true});
    environment.properties.push_back(
        {AudioVolumeKey, "AUDIO VOLUME", pipeframe::PropertyKind::Number, 30.0, true});

    pipeframe::SceneObjectTypeDescriptor training;
    training.typeId = TrainingSettingsTypeId;
    training.displayName = "TRAINING SETTINGS";
    training.properties.push_back(
        {PopulationSizeKey, "POPULATION SIZE", pipeframe::PropertyKind::Integer, std::int64_t{1000}, true});
    training.properties.push_back(
        {MaximumIterationTimeKey, "ITERATION SECONDS", pipeframe::PropertyKind::Number, 900.0, true});
    training.properties.push_back({EliteRatioKey, "ELITE RATIO", pipeframe::PropertyKind::Number, 0.2, true});
    training.properties.push_back(
        {SimulationSpeedUpKey, "TRAINING SPEED", pipeframe::PropertyKind::Number, 10.0, true});
    training.properties.push_back(
        {AngularSpeedKey, "RUDDER SPEED", pipeframe::PropertyKind::Number, 10.0, true});
    training.properties.push_back(
        {SeedOffsetKey, "SEED OFFSET", pipeframe::PropertyKind::Integer, std::int64_t{1}, true});
    training.properties.push_back(
        {AsyncTrainingKey, "ASYNC TRAINING", pipeframe::PropertyKind::Boolean, false, true});

    std::vector result{std::move(start),std::move(finish),std::move(waypoint),std::move(environment),std::move(training)};
    for(auto &type:result)type.componentTypeIds.push_back(type.typeId);
    return result;
}

std::vector<pipeframe::SceneComponentTypeDescriptor> SailBoatSimulationRuntime::CreateComponentTypes(
    const std::span<const pipeframe::SceneObjectTypeDescriptor> types) {
    std::vector<pipeframe::SceneComponentTypeDescriptor> result;
    result.reserve(types.size());
    for(const auto &type:types)
        result.push_back({type.typeId,type.displayName+" Settings",1,false,false,type.properties});
    return result;
}

void SailBoatSimulationRuntime::RebuildDomainState() {
    configuration = {};

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId == EnvironmentTypeId) {
            configuration.worldSize = ReadVector(object, WorldSizeKey, configuration.worldSize);
            const float direction = DegreesToRadians(
                static_cast<float>(ReadNumber(object, WindDirectionKey, 0.0)));
            const float speed = std::max(0.0f, static_cast<float>(ReadNumber(object, WindSpeedKey, 1.0)));
            configuration.wind = {std::cos(direction) * speed, std::sin(direction) * speed};
            configuration.waterAnimation = ReadBoolean(object, WaterAnimationKey, true);
            configuration.drawBestOnly = ReadBoolean(object, DrawBestOnlyKey, false);
            configuration.drawGhostBoats = ReadBoolean(object, DrawGhostBoatsKey, true);
            configuration.highlightBest = ReadBoolean(object, HighlightBestKey, true);
            configuration.drawBestTrajectory = ReadBoolean(object, DrawBestTrajectoryKey, true);
            configuration.drawWaypointLabels = ReadBoolean(object, DrawWaypointLabelsKey, true);
            configuration.showTargetGuide = ReadBoolean(object, ShowTargetGuideKey, true);
            configuration.audioEnabled = ReadBoolean(object, AudioEnabledKey, true);
            configuration.audioVolume = std::clamp(
                static_cast<float>(ReadNumber(object, AudioVolumeKey, 30.0)), 0.0f, 100.0f);
        } else if (object.typeId == TrainingSettingsTypeId) {
            configuration.populationSize = static_cast<std::uint32_t>(std::clamp<std::int64_t>(
                ReadInteger(object, PopulationSizeKey, 1000), 1, 1'000'000));
            configuration.maximumIterationTime = std::max(
                0.01f, static_cast<float>(ReadNumber(object, MaximumIterationTimeKey, 900.0)));
            configuration.eliteRatio = std::clamp(
                static_cast<float>(ReadNumber(object, EliteRatioKey, 0.2)), 0.0f, 1.0f);
            configuration.simulationSpeedUp = std::max(
                0.01f, static_cast<float>(ReadNumber(object, SimulationSpeedUpKey, 10.0)));
            configuration.angularSpeedDegrees = std::max(
                0.01f, static_cast<float>(ReadNumber(object, AngularSpeedKey, 10.0)));
            configuration.seedOffset = static_cast<std::uint32_t>(std::max<std::int64_t>(
                0, ReadInteger(object, SeedOffsetKey, 1)));
            configuration.asyncTraining = ReadBoolean(object, AsyncTrainingKey, false);
        }
    }

    raceCourse.Clear();
    raceCourse.SetWorldSize(configuration.worldSize);

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId == RaceStartTypeId) {
            raceCourse.SetStart(MakeSegment(object, 80.0f));
        } else if (object.typeId == FinishLineTypeId) {
            raceCourse.SetFinish(MakeSegment(object, 100.0f));
        } else if (object.typeId == WaypointTypeId && raceCourse.GetWaypoints().size() < configuration.maximumWaypointCount) {
            raceCourse.AddWaypoint({
                .segment = MakeSegment(object, 60.0f),
                .radius = std::max(1.0f, static_cast<float>(ReadNumber(object, WaypointRadiusKey, 10.0))),
                .order = ReadInteger(object, WaypointOrderKey, 1),
            });
        }
    }

    raceCourse.SortWaypoints();

    boatEnvironment.size = configuration.worldSize;
    boatEnvironment.wind = configuration.wind;
    audio.SetMasterVolume(configuration.audioVolume/100.0f);
    ResetPreviewBoat();
}

void SailBoatSimulationRuntime::ResetPreviewBoat() {
    renderer.ResetRaceProgress();
    renderer.ResetWaterSimulation();
    lastAudibleTarget = 0;
    populationTrainer.Clear();
    previewRaceTask.Reset();
    previewBoatInitialized = raceCourse.GetStart().IsValid();
    if (!previewBoatInitialized) {
        previewBoat = {};
        return;
    }

    previewBoat.Reset(raceCourse.GetStart().GetFirstPoint(),
                      DegreesToRadians(raceCourse.GetStart().GetAngleDegrees()));

    std::string trainingError;
    if (populationTrainer.Initialize(configuration, raceCourse, boatEnvironment, trainingError)) {
        populationTrainer.SetRunDirectory(trainingRunDirectory);
    }
}

void SailBoatSimulationRuntime::FitCamera(RenderContext &context) {
    Camera2D &camera = context.GetCamera();
    const sf::IntRect viewport = context.GetWorldViewportBounds();
    const float width = static_cast<float>(std::max(1, viewport.size.x));
    const float height = static_cast<float>(std::max(1, viewport.size.y));
    const float aspect = width / height;

    sf::Vector2f cameraSize = pipeframe::backend::sfml::ToBackend(configuration.worldSize * 1.1f);
    if (cameraSize.x / cameraSize.y > aspect) {
        cameraSize.y = cameraSize.x / aspect;
    } else {
        cameraSize.x = cameraSize.y * aspect;
    }

    camera.SetCenter(pipeframe::backend::sfml::ToBackend(configuration.worldSize * 0.5f));
    camera.SetSize(cameraSize);
    camera.SetZoom(1.0f);
    cameraInitialized = true;
}

void SailBoatSimulationRuntime::DrawRaceObject(sf::RenderTarget &target,
                                               const pipeframe::SceneObjectData &object) const {
    sf::Color color;
    float fallbackLength = 0.0f;

    if (object.typeId == RaceStartTypeId) {
        color = StartColor;
        fallbackLength = 80.0f;
    } else if (object.typeId == FinishLineTypeId) {
        color = FinishColor;
        fallbackLength = 100.0f;
    } else if (object.typeId == WaypointTypeId) {
        color = WaypointColor;
        fallbackLength = 60.0f;
    } else {
        return;
    }

    const RaceSegment segment = MakeSegment(object, fallbackLength);
    const sf::Vertex line[] = {{pipeframe::backend::sfml::ToBackend(segment.GetFirstPoint()), color},
                               {pipeframe::backend::sfml::ToBackend(segment.GetSecondPoint()), color}};
    target.draw(line, 2, sf::PrimitiveType::Lines);

    const float pointRadius = object.typeId == WaypointTypeId
                                  ? std::max(4.0f, static_cast<float>(ReadNumber(object, WaypointRadiusKey, 10.0)))
                                  : 7.0f;
    sf::CircleShape point(pointRadius);
    point.setOrigin({pointRadius, pointRadius});
    point.setPosition(pipeframe::backend::sfml::ToBackend(segment.GetFirstPoint()));
    point.setFillColor(color);

    if (selectedObjectId.has_value() && *selectedObjectId == object.id) {
        point.setOutlineColor(SelectionColor);
        point.setOutlineThickness(3.0f);
    }

    target.draw(point);
}

void SailBoatSimulationRuntime::DrawPreviewTrajectory(sf::RenderTarget &target) const {
    if (!previewBoatInitialized || previewBoat.GetTrajectory().size() < 2) {
        return;
    }

    pipeframe::Path2D trajectory;trajectory.color=pipeframe::backend::sfml::FromBackend(WaypointColor);
    for(const auto &point:previewBoat.GetTrajectory())trajectory.points.push_back(point.position);
    pipeframe::backend::sfml::DrawGeometry(target,pipeframe::BuildPathCommand(trajectory));
}

void SailBoatSimulationRuntime::DrawPreviewBoat(sf::RenderTarget &target) const {
    if (!previewBoatInitialized) {
        return;
    }

    sf::ConvexShape boat(4);
    boat.setPoint(0, {16.0f, 0.0f});
    boat.setPoint(1, {-8.0f, -7.0f});
    boat.setPoint(2, {-13.0f, 0.0f});
    boat.setPoint(3, {-8.0f, 7.0f});
    boat.setFillColor(previewBoat.HasCrashed() ? FinishColor
                                              : previewBoat.HasFinished() ? StartColor
                                                                          : sf::Color::White);
    boat.setOutlineColor(sf::Color{20, 55, 80});
    boat.setOutlineThickness(2.0f);
    boat.setPosition(pipeframe::backend::sfml::ToBackend(previewBoat.GetPosition()));
    boat.setRotation(sf::radians(previewBoat.GetAngleRadians()));
    target.draw(boat);
}

const pipeframe::SceneObjectData *SailBoatSimulationRuntime::FindObject(const pipeframe::SceneObjectId objectId) const {
    const auto iterator = std::find_if(authoredObjects.begin(), authoredObjects.end(),
                                       [objectId](const pipeframe::SceneObjectData &object) {
                                           return object.id == objectId;
                                       });
    return iterator == authoredObjects.end() ? nullptr : &*iterator;
}

double SailBoatSimulationRuntime::ReadNumber(const pipeframe::SceneObjectData &object, const std::string &key,
                                             const double fallback) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);
    if (iterator == properties->end()) {
        return fallback;
    }
    if (const auto *number = std::get_if<double>(&iterator->second)) {
        return *number;
    }
    if (const auto *integer = std::get_if<std::int64_t>(&iterator->second)) {
        return static_cast<double>(*integer);
    }
    return fallback;
}

std::int64_t SailBoatSimulationRuntime::ReadInteger(const pipeframe::SceneObjectData &object, const std::string &key,
                                                    const std::int64_t fallback) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);
    if (iterator == properties->end()) {
        return fallback;
    }
    if (const auto *integer = std::get_if<std::int64_t>(&iterator->second)) {
        return *integer;
    }
    if (const auto *number = std::get_if<double>(&iterator->second)) {
        return static_cast<std::int64_t>(*number);
    }
    return fallback;
}

bool SailBoatSimulationRuntime::ReadBoolean(const pipeframe::SceneObjectData &object, const std::string &key,
                                            const bool fallback) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);
    if (iterator == properties->end()) {
        return fallback;
    }
    if (const auto *value = std::get_if<bool>(&iterator->second)) {
        return *value;
    }
    return fallback;
}

pipeframe::Vector2f SailBoatSimulationRuntime::ReadVector(const pipeframe::SceneObjectData &object, const std::string &key,
                                                          const pipeframe::Vector2f fallback) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);
    if (iterator == properties->end()) {
        return fallback;
    }
    if (const auto *value = std::get_if<pipeframe::Vector2f>(&iterator->second)) {
        return *value;
    }
    return fallback;
}

RaceSegment SailBoatSimulationRuntime::MakeSegment(const pipeframe::SceneObjectData &object,
                                                   const float fallbackLength) {
    const float length = std::max(1.0f, static_cast<float>(ReadNumber(object, LineLengthKey, fallbackLength)));
    const float radians = DegreesToRadians(object.transform.rotation);
    const pipeframe::Vector2f direction{std::cos(radians), std::sin(radians)};
    const pipeframe::Vector2f position = object.transform.position;
    return {position, position + direction * length};
}

} // namespace sailboat_simulation

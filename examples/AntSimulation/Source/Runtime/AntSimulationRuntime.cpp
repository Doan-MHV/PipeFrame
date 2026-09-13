#include "Editor/AntFoodBrush.h"
#include <PipeFrame/UI/SimulationDashboard.h>
#if __has_include("Runtime/GeneratedRegistration.h")
#include "Runtime/GeneratedRegistration.h"
#define PIPEFRAME_ANT_GENERATED_REGISTRATION 1
#endif
#include "World/Rendering/SignalBeaconGeometry.h"
#include "Runtime/AntSimulationRuntime.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>
#include <variant>


#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Resources/ImageData.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>
#include <PipeFrame/Render/RenderServices2D.h>

#include "Runtime/AntTypeIds.h"
#include "World/Runtime/Environment/WorldMapLoader.h"
#include "Runtime/AntRegistration.h"
#include "Components/ColonySettingsComponent.h"
#include "World/Runtime/Systems/AntSystemIds.h"
#include "Runtime/AntPluginDescriptor.h"
#include "Editor/AntEditorExtensionIds.h"

namespace ant_simulation {

namespace {

constexpr std::int64_t MaximumAntCount{1'000'000};

constexpr std::int64_t MaximumFoodPerCell{1'000'000};

constexpr std::array<pipeframe::Color, 6> ColonyColors{
    pipeframe::Color{239, 71, 111},  pipeframe::Color{6, 214, 160},  pipeframe::Color{17, 138, 178},
    pipeframe::Color{255, 209, 102}, pipeframe::Color{244, 162, 97}, pipeframe::Color{131, 56, 236},
};

} // namespace

AntSimulationRuntime::AntSimulationRuntime()
    : renderingWorld(std::make_shared<AntRenderingWorld>(configuration)),
      environmentRenderer(renderingWorld->environment),antRenderer(renderingWorld->ants),
      shadowRenderer(renderingWorld->shadows),debugRenderer(renderingWorld->debug),editorToolRenderer(renderingWorld->editor),
      antInspector(configuration), componentTypes(CreateAntComponentTypes()) {
    RegisterBrush<AntFoodBrush>("Ant");
    RegisterAntComponents(componentRegistry);
    RegisterAntEntities(entityRegistry);
#ifdef PIPEFRAME_ANT_GENERATED_REGISTRATION
    pipeframe_generated::Register(*this);
#endif
    objectTypes=entityRegistry.Describe();
    std::ranges::sort(objectTypes,[](const auto &a,const auto &b) {
        const auto order=[](const auto &id){return id==ColonyTypeId?0:id==FoodSourceTypeId?1:id==SimulationSettingsTypeId?2:id==SignalBeaconTypeId?3:4;};
        return order(a.typeId)<order(b.typeId);
    });
    editorTool.SetEnabled(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
}

const char *AntSimulationRuntime::GetName() const { return "Ant Simulation"; }

pipeframe::ProjectPluginDescriptor AntSimulationRuntime::GetPluginDescriptor() const {
    return contract::Descriptor();
}

bool AntSimulationRuntime::RegisterPlugin(pipeframe::PluginRegistrar &registrar,
                                          std::string &error) {
    const std::array extensions{
        pipeframe::ExtensionDescriptor{contract::Dashboard, {}, "Ant Dashboard", pipeframe::ExtensionPoint::Drawer, "simulation"},
        pipeframe::ExtensionDescriptor{contract::WorldBrush, {}, "Ant World Brush", pipeframe::ExtensionPoint::EnvironmentBrush, "scene"},
        pipeframe::ExtensionDescriptor{contract::SelectionTool, {}, "Ant Selection Tool", pipeframe::ExtensionPoint::Tool, "scene"},
        pipeframe::ExtensionDescriptor{contract::DebugOverlay, {}, "Ant Debug Overlay", pipeframe::ExtensionPoint::SimulationDebugOverlay, "scene"},
        pipeframe::ExtensionDescriptor{contract::Telemetry, {}, "Ant Telemetry", pipeframe::ExtensionPoint::TelemetryStream, "simulation"},
        pipeframe::ExtensionDescriptor{contract::Settings, {}, "Ant Settings", pipeframe::ExtensionPoint::Setting, "project"}};
    for (auto extension : extensions) {
        extension.invoke=[this](const auto &){ApplyRenderOptions();RefreshDashboard();};
        if (!registrar.Extension(std::move(extension), &error)) return false;
    }
    if (!registrar.Action({"ant.toggle-markers", {}, "Toggle Pheromone Markers", "Ant",
                           {"scene", "simulation"}, "M",
                           [this](const auto &) { renderOptions.showMarkers = !renderOptions.showMarkers; ApplyRenderOptions(); }}, &error)) return false;
    if (!registrar.Action({"ant.toggle-physics", {}, "Toggle Physics Debug", "Ant",
                           {"scene", "simulation"}, "P",
                           [this](const auto &) { renderOptions.showPhysicsDebug = !renderOptions.showPhysicsDebug; ApplyRenderOptions(); }}, &error)) return false;
    pipeframe::ProjectSystemDescriptor colonies{contract::ColonyLifecycleSystem, {}, pipeframe::SystemPhase::FixedPrePhysics, {},
        {ColonyTypeId, FoodSourceTypeId}, {ColonyTypeId, FoodSourceTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {
            if(playing && simulationWorld)simulationWorld->BeginFixedStep(context.deltaTime);
            if(context.tick==1)context.log->Write("ant","scheduled systems started");
        }};
    colonies.requiredServices={std::string(pipeframe::SpatialServiceId),std::string(pipeframe::EventServiceId),
        std::string(pipeframe::RandomServiceId),std::string(pipeframe::JobServiceId),std::string(pipeframe::LogServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(colonies),&error)) return false;
    pipeframe::ProjectSystemDescriptor movement{contract::MovementSystem, {}, pipeframe::SystemPhase::FixedPhysics,
        {contract::ColonyLifecycleSystem}, {ColonyTypeId}, {ColonyTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {if(playing&&simulationWorld)simulationWorld->UpdateMovement(context.deltaTime);}};
    movement.requiredServices={std::string(pipeframe::PhysicsServiceId),std::string(pipeframe::SpatialServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(movement),&error)) return false;
    pipeframe::ProjectSystemDescriptor behavior{contract::BehaviorSystem, {}, pipeframe::SystemPhase::FixedBehavior,
        {contract::MovementSystem}, {ColonyTypeId}, {ColonyTypeId,FoodSourceTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {if(playing&&simulationWorld)simulationWorld->UpdateBehavior(context.deltaTime);}};
    behavior.requiredServices={std::string(pipeframe::RandomServiceId),std::string(pipeframe::EventServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(behavior),&error)) return false;
    pipeframe::ProjectSystemDescriptor cleanup{contract::CleanupSystem, {}, pipeframe::SystemPhase::FixedCleanup,
        {contract::BehaviorSystem}, {ColonyTypeId}, {ColonyTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {
            if(playing&&simulationWorld){simulationWorld->EndFixedStep(context.deltaTime);simulationElapsedTime+=context.deltaTime;}
            context.events->Publish({"ant.simulation-tick",std::to_string(context.tick)});
        }};
    cleanup.requiredServices={std::string(pipeframe::PhysicsServiceId),std::string(pipeframe::EventServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(cleanup),&error)) return false;
    pipeframe::ProjectSystemDescriptor live{contract::LiveStateSystem, {}, pipeframe::SystemPhase::FixedCleanup,
        {contract::CleanupSystem}, {ColonyTypeId}, {},false,false,false,
        [this](pipeframe::SystemContext &context) {RefreshSimulationState(context.deltaTime);}};
    live.requiredServices={std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(live),&error)) return false;
    pipeframe::ProjectSystemDescriptor render{contract::RenderSystem, {}, pipeframe::SystemPhase::Render,
        {contract::LiveStateSystem}, {ColonyTypeId, FoodSourceTypeId}, {},false,false,false,
        [this](pipeframe::SystemContext &context) {
            (void)context.services->Find<pipeframe::GraphicsResourceService>();
            (void)context.services->Find<pipeframe::SurfaceRegistry>();
            if(auto *target=context.services->Find<RenderContext>())Render(*target);
        }};
    render.requiredServices={std::string(pipeframe::RenderServiceId),std::string(pipeframe::ResourceServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    return registrar.System(std::move(render), &error);
}

bool AntSimulationRuntime::Load(const pipeframe::ProjectRuntimeContext &context, std::string &errorMessage) {
    dashboard.reset();
    projectDirectory = context.projectDirectory;environmentLog=context.log;
    if (!tilemapAssets.Load(context, errorMessage) || !environmentVisuals.Load(context,errorMessage)) return false;

    registeredObjects.Clear();
    authoredObjects.clear();
    selectedObjectId.reset();

    renderStatistics = {};

    playing = false;
    loaded = false;
    cameraInitialized = false;

    if (!projectDirectory.empty()) {
        const std::filesystem::path assetRoot = projectDirectory / "Assets";

        if (!LoadRendererAssets(assetRoot, errorMessage)) {
            return false;
        }
    }

    if (!RebuildSimulation(&errorMessage)) {
        return false;
    }

    editorTool.SetEnabled(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);

    loaded = true;
    errorMessage.clear();

    return true;
}

bool AntSimulationRuntime::LoadRendererAssets(const std::filesystem::path &assetRoot, std::string &errorMessage) {
    if (!environmentRenderer.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    if (!antRenderer.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    if (!shadowRenderer.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    uiFont=uiResources.LoadFont((assetRoot/"Fonts"/"roboto_regular.ttf").string(),&errorMessage);
    if(uiResources.State(uiFont)!=pipeframe::ResourceState::Ready) {
        errorMessage="Unable to load Ant dashboard font."; return false;
    }
    BuildDashboard();

    errorMessage.clear();

    return true;
}

void AntSimulationRuntime::ApplyRenderOptions() {
    environmentRenderer.SetGridEnabled(renderOptions.showGrid);

    environmentRenderer.SetMarkersEnabled(renderOptions.showMarkers);

    environmentRenderer.SetWallShadowEnabled(renderOptions.showShadows);

    shadowRenderer.SetEnabled(renderOptions.showShadows);

    debugRenderer.SetTargetVisible(renderOptions.showTargets);

    debugRenderer.SetPhysicsDebugVisible(renderOptions.showPhysicsDebug);

    configuration.dynamicAntColor = renderOptions.dynamicAntColors;

    /*
     * AntPezza exposes marker intensity as a slider from
     * 1 through 20.
     *
     * A lower power produces brighter/stronger trails.
     */
    const int clampedIntensity = std::clamp(renderOptions.markerIntensity, 1, 20);

    const float intensityRatio = static_cast<float>(clampedIntensity - 1) / 19.0f;

    constexpr float MinimumPower{0.025f};

    constexpr float MaximumPower{0.5f};

    const float markerColorPower = (1.0f - intensityRatio) * (MaximumPower - MinimumPower) + MinimumPower;

    environmentRenderer.SetMarkerColorPower(markerColorPower);
}

std::span<const pipeframe::SceneObjectTypeDescriptor> AntSimulationRuntime::GetSceneObjectTypes() const {
    return objectTypes;
}

std::span<const pipeframe::SceneComponentTypeDescriptor> AntSimulationRuntime::GetSceneComponentTypes() const {
    return componentTypes;
}

pipeframe::SceneObjectData AntSimulationRuntime::CreateDefaultObject(const pipeframe::SceneObjectTypeId &typeId) const {
    pipeframe::SceneObjectData object = ProjectRuntime::CreateDefaultObject(typeId);

    if (typeId == ColonyTypeId) {
        object.name = "ANT COLONY";

        object.transform.position = configuration.colonyPosition;

        std::size_t existingColonyCount{0};

        for (const pipeframe::SceneObjectData &existingObject : authoredObjects) {
            if (existingObject.typeId == ColonyTypeId) {
                ++existingColonyCount;
            }
        }

        const pipeframe::Color color = ColonyColors[existingColonyCount % ColonyColors.size()];

        object.properties[ColonyColorRedKey] = static_cast<std::int64_t>(color.r);

        object.properties[ColonyColorGreenKey] = static_cast<std::int64_t>(color.g);

        object.properties[ColonyColorBlueKey] = static_cast<std::int64_t>(color.b);
    } else if (typeId == FoodSourceTypeId) {
        object.name = "FOOD SOURCE";

        object.transform.position = {
            configuration.GetWorldSizeFloat().x * 0.75f,

            configuration.GetWorldSizeFloat().y * 0.5f,
        };
    } else if (typeId == SimulationSettingsTypeId) {
        object.name = "SIMULATION SETTINGS";

        object.transform.position = {
            0.0f,
            0.0f,
        };
    }

    if(typeId==pipeframe::PlaygroundEntityTypeId){
        for(auto &component:object.components)if(component.typeId==pipeframe::PlaygroundComponentTypeId){
            component.properties["cellSize"]=1.0;
            component.properties["color"]=EnvironmentRenderer::BackgroundColor;
        }
    }
    if(auto component=std::ranges::find(object.components,typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()){
        for(const auto &[key,value]:object.properties)component->properties.insert_or_assign(key,value);
        object.properties.clear();
    }
    pipeframe::SynchronizeTransformComponent(object);
    return object;
}

void AntSimulationRuntime::SynchronizeScene(const std::span<const pipeframe::SceneObjectData> sourceObjects) {
    std::vector<pipeframe::SceneObjectData> objects(sourceObjects.begin(),sourceObjects.end());
    for (auto &object : objects) NormalizeObjectComponents(object);
    if(simulationWorld && TrySynchronizeComponentValues(authoredObjects,objects,liveAuthoringState))return;
    auto previous=authoredObjects;
    authoredObjects.assign(objects.begin(), objects.end());
    std::string rebuildError;
    if(!RebuildSimulation(&rebuildError) && std::ranges::any_of(objects,[](const auto &o){return o.typeId==pipeframe::PlaygroundEntityTypeId;})){
        authoredObjects=std::move(previous);throw std::invalid_argument(rebuildError);
    }

    renderOptions = {};

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != SimulationSettingsTypeId) {
            continue;
        }

        renderOptions.showGrid = ReadBoolean(object, ShowGridKey, true);

        renderOptions.showMarkers = ReadBoolean(object, ShowMarkersKey, true);

        renderOptions.showShadows = ReadBoolean(object, ShowShadowsKey, true);

        renderOptions.showTargets = ReadBoolean(object, ShowTargetsKey, false);

        renderOptions.showPhysicsDebug = ReadBoolean(object, ShowPhysicsDebugKey, false);

        renderOptions.showAnts = ReadBoolean(object, ShowAntsKey, true);

        renderOptions.dynamicAntColors = ReadBoolean(object, DynamicAntColorsKey, false);

        renderOptions.markerIntensity = static_cast<int>(
            std::clamp(ReadInteger(object, MarkerIntensityKey, 10), std::int64_t{1}, std::int64_t{20}));

        break;
    }

    ApplyRenderOptions();
}

void AntSimulationRuntime::SetSelectedObject(const std::optional<pipeframe::SceneObjectId> objectId) {
    selectedObjectId = objectId;

    // Authored selection and live-ant inspection are independent during playback.
    if (playing) {
        return;
    }

    antInspector.SetSelectedAnt(std::nullopt);

    colonyInspector.SetSelectedColony(std::nullopt);

    if (!selectedObjectId.has_value() || simulationWorld == nullptr) {
        return;
    }

    const ColonyView *colony = simulationWorld->GetColonyLifecycleSystem().FindColony(*selectedObjectId);

    if (colony == nullptr) {
        return;
    }

    colonyInspector.SetSelectedColony(colony->GetId());

    selectedColony=colony->GetId();

    colonyInspector.Refresh(simulationWorld->GetColonyLifecycleSystem().GetColonies());
}

void AntSimulationRuntime::SetViewMode(const pipeframe::ProjectRuntimeViewMode mode) {
    if (viewMode == mode) {
        return;
    }

    if(simulationWorld && editorTool.IsStrokeActive()) editorTool.CancelStroke(simulationWorld->GetEnvironment());
    viewMode = mode;
    editorTool.SetEnabled(mode!=pipeframe::ProjectRuntimeViewMode::Zen);
    cameraInitialized = false;
}

std::optional<pipeframe::SceneObjectId> AntSimulationRuntime::HitTest(const pipeframe::Vector2f worldPosition) const {
    for (auto iterator = authoredObjects.rbegin(); iterator != authoredObjects.rend(); ++iterator) {
        if (iterator->typeId != ColonyTypeId && iterator->typeId != FoodSourceTypeId && iterator->typeId != SignalBeaconTypeId) {
            continue;
        }

        if (ContainsPoint(iterator->transform.position, GetSelectionRadius(*iterator), worldPosition)) {
            return iterator->id;
        }
    }

    for(const auto &[id,object]:registeredObjects.All()){
        const auto *ground=object.GetComponent<pipeframe::PlaygroundComponent>();
        const auto *pose=object.GetComponent<pipeframe::Transform2DComponent>();
        if(ground&&pose&&pipeframe::PlaygroundContains(worldPosition,*pose,*ground))return id;
    }
    return std::nullopt;
}

void AntSimulationRuntime::Start() {
    RefreshAuthoredEnvironment();
    liveAuthoringState=pipeframe::ProjectRuntimeAuthoringState::Playing;
    if (simulationWorld == nullptr) {
        if (!RebuildSimulation()) {
            return;
        }
    }

    if (editorTool.IsStrokeActive()) {
        editorTool.CancelStroke(simulationWorld->GetEnvironment());
    }

    editorTool.SetEnabled(viewMode!=pipeframe::ProjectRuntimeViewMode::Zen);

    playing = true;
}

void AntSimulationRuntime::FixedUpdate(const float fixedDeltaTime) {
    RefreshAuthoredEnvironment();
    if (!playing || simulationWorld == nullptr || fixedDeltaTime <= 0.0f) {
        return;
    }

    AdvanceSimulation(fixedDeltaTime);
    RefreshSimulationState(fixedDeltaTime);
}

void AntSimulationRuntime::AdvanceSimulation(const float fixedDeltaTime) {
    if (!playing || simulationWorld == nullptr || fixedDeltaTime <= 0.0f) return;
    using Clock = std::chrono::steady_clock;
    using Milliseconds = std::chrono::duration<float, std::milli>;
    const auto start = Clock::now();
    simulationWorld->FixedUpdate(fixedDeltaTime);
    simulationElapsedTime += fixedDeltaTime;
    renderStatistics.movementTimeMs = Milliseconds(Clock::now() - start).count();
    renderStatistics.spatialGridTimeMs = 0.0f;
}

void AntSimulationRuntime::RefreshSimulationState(const float fixedDeltaTime) {
    if (!playing || simulationWorld == nullptr || fixedDeltaTime <= 0.0f) return;
    antInspector.Refresh(simulationWorld->GetAntQuery().GetAnts());

    colonyInspector.Refresh(simulationWorld->GetColonyLifecycleSystem().GetColonies());
    for (const auto &colony : simulationWorld->GetColonyLifecycleSystem().GetColonies()) histories.try_emplace(colony.GetId()).first->second.Update(colony,fixedDeltaTime);
}

void AntSimulationRuntime::Render(RenderContext &context) {
    RefreshAuthoredEnvironment();
    if (!loaded || simulationWorld == nullptr || !simulationWorld->IsInitialized()) {
        return;
    }

    const AntInspectorData &inspectorData = antInspector.GetData();

    if (inspectorData.available && inspectorData.follow) {
        context.SetCameraCenter({inspectorData.position.x,inspectorData.position.y});
    }

    if (!cameraInitialized) {
        FitCamera(context);
    }

    context.BeginWorld();

    const auto viewport = GetWorldViewport(context);

    const float renderZoom = GetRenderZoom(context);

    AntEnvironment &environment = simulationWorld->GetEnvironment();

    const std::span<const ColonyView> colonies = simulationWorld->GetColonyLifecycleSystem().GetColonies();

    const std::span<const AntView> ants = simulationWorld->GetAntQuery().GetAnts();

    const std::span<const AntPhysicsBody> physicsBodies = simulationWorld->GetPhysicsBodies().GetBodies();

    using Clock = std::chrono::steady_clock;

    using Milliseconds = std::chrono::duration<float, std::milli>;

    const auto geometryStart = Clock::now();

    environmentRenderer.Update(environment, colonies, viewport);

    if (renderOptions.showAnts) {
        antRenderer.UpdateGeometry(ants, viewport, renderZoom);

        shadowRenderer.Update(antRenderer.GetGeometry(), antRenderer.GetMode());
    }

    debugRenderer.SetSelectedAnt(inspectorData.highlight ? antInspector.GetSelectedAnt() : std::nullopt);

    debugRenderer.SetTargetVisible(renderOptions.showTargets || inspectorData.showTarget);

    debugRenderer.Update(ants, physicsBodies, viewport);

    editorToolRenderer.Update(editorTool, environment, viewport);

    const auto geometryEnd = Clock::now();

    renderingWorld->ShowAnts(renderOptions.showAnts);

    environmentRenderer.SetGround(std::nullopt);environmentRenderer.SetTerrainVisible(true);
    for(const auto &[id,object]:registeredObjects.All())if(const auto *ground=object.GetComponent<pipeframe::PlaygroundComponent>()){
        if(const auto *tiles=object.GetComponent<pipeframe::TilemapComponent>())environmentRenderer.SetTerrainVisible(tiles->visible);
        auto display=*ground;display.showGrid=display.showGrid&&renderOptions.showGrid;pipeframe::RenderState state;pipeframe::Vector2f uv;
        if(const auto *material=environmentVisuals.ResolveMaterial(ground->material);material&&material->error.empty()){
            display.color=pipeframe::MultiplyTint(ground->color,material->value.tint);state=material->State();
            uv={material->textureSize.x*material->value.uvScale.x,material->textureSize.y*material->value.uvScale.y};
        }
        environmentRenderer.SetGround(display,state,uv);
    }
    renderingWorld->Render(context.GetCanvas(),0);
    for(const auto &[id,object]:registeredObjects.All()) {
        const auto *transform=object.GetComponent<pipeframe::Transform2DComponent>();
        const auto *settings=object.GetComponent<SignalBeaconComponent>();
        if(!transform || !settings)continue;
        const auto vertices=BuildSignalBeaconGeometry(*transform,*settings);
        context.GetCanvas().Draw(vertices.data(),vertices.size(),pipeframe::PrimitiveTopology::Triangles);
    }

    DrawAuthoredSelection(context);

    renderingWorld->Render(context.GetCanvas(),1);

    const AntRendererStatistics &statistics = antRenderer.GetStatistics();

    if (renderOptions.showAnts) {
        renderStatistics.candidates = statistics.candidateCount;

        renderStatistics.visible = statistics.visibleCount;

        renderStatistics.vertices = statistics.vertexCount;

        renderStatistics.usingQuads = statistics.mode != AntRenderingMode::Points;
    } else {
        renderStatistics.candidates = 0;
        renderStatistics.visible = 0;
        renderStatistics.vertices = 0;
        renderStatistics.usingQuads = false;
    }

    renderStatistics.geometryTimeMs = Milliseconds(geometryEnd - geometryStart).count();

}

void AntSimulationRuntime::HandleEvent(const pipeframe::InputEvent &inputEvent, RenderContext &context) {
    using namespace pipeframe;
    if(!simulationWorld || viewMode==ProjectRuntimeViewMode::Zen)return;
    auto &environment=simulationWorld->GetEnvironment();
    if((inputEvent.type==InputEventType::FocusChanged && inputEvent.GetIf<FocusInput>() && !inputEvent.GetIf<FocusInput>()->focused) ||
       (inputEvent.type==InputEventType::PointerPresenceChanged && inputEvent.GetIf<PointerPresenceInput>() && !inputEvent.GetIf<PointerPresenceInput>()->inside)) {
        editorTool.CancelStroke(environment);return;
    }
    if(const auto *pointer=inputEvent.GetIf<PointerInput>();pointer && pointer->button==PointerButton::Right) {
        const auto position=context.MapPixelToWorld(pointer->position);
        if(inputEvent.type==InputEventType::PointerPressed && (!editorTool.IsEnabled() || editorTool.GetMode()==AntEditorToolMode::None)) {
            const float radius=std::max(AntDebugRenderer::SelectionRadius,8.0f/std::max(.001f,GetRenderZoom(context)));
            (void)SelectAntAt(position,radius);return;
        }
        if(!editorTool.IsEnabled())return;
        editorTool.SetPosition(position);
        if(inputEvent.type==InputEventType::PointerPressed)editorTool.BeginStroke(environment);
        else if(inputEvent.type==InputEventType::PointerReleased)editorTool.EndStroke(environment);
    } else if(const auto *move=inputEvent.GetIf<PointerMoveInput>();move && editorTool.IsEnabled() && inputEvent.type==InputEventType::PointerMoved) {
        editorTool.SetPosition(context.MapPixelToWorld(move->position));
        if(editorTool.IsStrokeActive())editorTool.UpdateStroke(environment);
    }
}

void AntSimulationRuntime::Reset() {
    liveAuthoringState=pipeframe::ProjectRuntimeAuthoringState::Stopped;
    playing = false;

    simulationElapsedTime = 0.0f;

    RebuildSimulation();

    editorTool.SetEnabled(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
}

void AntSimulationRuntime::Stop() {
    if (liveAuthoringState==pipeframe::ProjectRuntimeAuthoringState::Playing) liveAuthoringState=pipeframe::ProjectRuntimeAuthoringState::Paused;
    playing = false;

    if (simulationWorld != nullptr && editorTool.IsStrokeActive()) {
        editorTool.CancelStroke(simulationWorld->GetEnvironment());
    }

    editorTool.SetEnabled(viewMode != pipeframe::ProjectRuntimeViewMode::Zen);
}

void AntSimulationRuntime::Unload() {
    liveAuthoringState=pipeframe::ProjectRuntimeAuthoringState::Stopped;
    dashboard.reset();
    playing = false;
    loaded = false;
    cameraInitialized = false;

    simulationWorld.reset();

    registeredObjects.Clear();
    authoredObjects.clear();
    selectedObjectId.reset();

    tilemapAssets.Unload();environmentVisuals.Unload();authoredMap={};authoredMapRevision=0;
    projectDirectory.clear();

    renderStatistics = {};

    simulationElapsedTime = 0.0f;

    editorTool = AntEditorTool{};
    editorTool.SetEnabled(false);

    antInspector.SetSelectedAnt(std::nullopt);

    colonyInspector.SetSelectedColony(std::nullopt);

    histories.clear(); selectedColony.reset();

}

pipeframe::ProjectRuntimeStatistics AntSimulationRuntime::GetStatistics() const {
    return {
        .available = loaded && simulationWorld != nullptr,

        .usingQuads = renderStatistics.usingQuads,

        .candidateObjectCount = renderStatistics.candidates,

        .visibleObjectCount = renderStatistics.visible,

        .vertexCount = renderStatistics.vertices,

        .movementTimeMs = renderStatistics.movementTimeMs,

        .spatialGridTimeMs = renderStatistics.spatialGridTimeMs,

        .geometryTimeMs = renderStatistics.geometryTimeMs,
    };
}

const AntSimulationRuntime::RenderStatistics &AntSimulationRuntime::GetRenderStatistics() const {
    return renderStatistics;
}

const AntSimulationRuntime::RenderOptions &AntSimulationRuntime::GetRenderOptions() const { return renderOptions; }

AntWorld *AntSimulationRuntime::GetSimulationWorld() { return simulationWorld.get(); }

const AntWorld *AntSimulationRuntime::GetSimulationWorld() const { return simulationWorld.get(); }

AntEditorTool &AntSimulationRuntime::GetEditorTool() { return editorTool; }

bool AntSimulationRuntime::SelectAntAt(const pipeframe::Vector2f worldPosition, const float selectionRadius) {
    if (simulationWorld == nullptr || selectionRadius <= 0.0f) {
        ClearSelectedAnt();
        return false;
    }

    const std::span<const AntView> ants = simulationWorld->GetAntQuery().GetAnts();

    const AntView *closestAnt = nullptr;

    float closestDistanceSquared = selectionRadius * selectionRadius;

    for (const AntView &ant : ants) {
        if (ant.IsDead()) {
            continue;
        }

        const pipeframe::Vector2f difference =
            ant.GetPosition() - worldPosition;

        const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

        if (distanceSquared > closestDistanceSquared) {
            continue;
        }

        closestDistanceSquared = distanceSquared;

        closestAnt = &ant;
    }

    if (closestAnt == nullptr) {
        ClearSelectedAnt();
        return false;
    }

    antInspector.SetSelectedAnt(closestAnt->GetId());

    antInspector.Refresh(ants);

    colonyInspector.SetSelectedColony(closestAnt->GetColonyId());
    selectedColony=closestAnt->GetColonyId();
    colonyInspector.Refresh(simulationWorld->GetColonyLifecycleSystem().GetColonies());

    return true;
}

void AntSimulationRuntime::ClearSelectedAnt() {
    antInspector.SetSelectedAnt(std::nullopt);

    colonyInspector.SetSelectedColony(std::nullopt);
}

const AntInspectorData &AntSimulationRuntime::GetAntInspectorData() const { return antInspector.GetData(); }

bool AntSimulationRuntime::RebuildSimulation(std::string *errorMessage) {
    histories.clear(); selectedColony.reset();

    simulationElapsedTime = 0.0f;
    AntConfiguration rebuiltConfiguration;

    std::uint32_t randomSeed{1};

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != ColonyTypeId) {
            continue;
        }

        ColonySettingsComponent settings;
        std::string validationError;
        pipeframe::PropertyMap values;
        const auto component = std::ranges::find(object.components, std::string(ColonyTypeId),
                                                 &pipeframe::SceneComponentData::typeId);
        if (component != object.components.end()) {
            for (const auto &[key, value] : component->properties) values.try_emplace(key, value);
        }
        if (!ColonySettingsComponent::Schema().Apply(settings, values, validationError)) {
            if (errorMessage) *errorMessage = validationError;
            return false;
        }
        rebuiltConfiguration.colonyInitialAntCount = static_cast<std::uint32_t>(settings.population);
        rebuiltConfiguration.colonyRadius = static_cast<float>(settings.radius);
        rebuiltConfiguration.antSpeed = static_cast<float>(settings.speed);
        randomSeed = static_cast<std::uint32_t>(settings.seed);

        break;
    }

    pipeframe::AssetReference nextMap;std::uint64_t nextMapRevision{};
    std::optional<pipeframe::PlaygroundComponent> playground;
    pipeframe::AssetReference playgroundMap;
    for(const auto &object:authoredObjects)if(object.typeId==pipeframe::PlaygroundEntityTypeId){
        if(playground){if(errorMessage)*errorMessage="Ant supports one Playground per scene.";return false;}
        if(object.transform.position!=pipeframe::Vector2f{} || object.transform.rotation!=0 || object.transform.scale!=pipeframe::Vector2f{1,1}){
            if(errorMessage)*errorMessage="Ant Playground must remain at origin with unit scale and zero rotation.";return false;
        }
        playground.emplace();pipeframe::TilemapComponent tiles;std::string error;
        for(const auto &component:object.components){
            if(component.typeId==pipeframe::PlaygroundComponentTypeId&&!pipeframe::PlaygroundComponent::Schema().Apply(*playground,component.properties,error)){if(errorMessage)*errorMessage=error;return false;}
            if(component.typeId==pipeframe::TilemapComponentTypeId&&!pipeframe::TilemapComponent::Schema().Apply(tiles,component.properties,error)){if(errorMessage)*errorMessage=error;return false;}
        }
        if(playground->cellSize!=1 || playground->columns<=4 || playground->rows<=4){if(errorMessage)*errorMessage="Ant Playground requires unit cells and dimensions larger than its two-cell border.";return false;}
        playgroundMap=tiles.asset;
    }
    std::filesystem::path mapPath;
    std::optional<pipeframe::Tilemap2D> mapData;

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != SimulationSettingsTypeId) {
            continue;
        }

        rebuiltConfiguration.colonyPosition = object.transform.position;

        rebuiltConfiguration.antUpdateWorkerCount = static_cast<std::uint32_t>(
            std::clamp(
                ReadInteger(object, UpdateWorkerCountKey, 1),
                std::int64_t{1},
                std::int64_t{64}));

        if(playground)break;
        pipeframe::AssetReference mapAsset;
        const pipeframe::PropertyMap *properties = &object.properties;
        if (!properties->contains(MapAssetKey)) {
            const auto component = std::ranges::find(object.components, std::string(SimulationSettingsTypeId),
                                                      &pipeframe::SceneComponentData::typeId);
            if (component != object.components.end()) properties = &component->properties;
        }
        if (const auto field = properties->find(MapAssetKey); field != properties->end()) {
            if (const auto *reference = std::get_if<pipeframe::AssetReference>(&field->second)) mapAsset = *reference;
        }
        std::string imageError;
        if (!mapAsset.assetId.empty()) {
            const auto *resource = tilemapAssets.Resolve(mapAsset);
            if (resource && resource->map) mapData = *resource->map;
            else imageError = resource ? resource->error : "Missing environment map asset";
        } else {
            break; // No environment asset: start with an empty world.
        }
        if (!mapData) {

            if (errorMessage) *errorMessage = "Unable to load Ant environment: " + imageError;
            return false;
        }

        const pipeframe::Vector2u imageSize{unsigned(mapData->Columns()),unsigned(mapData->Rows())};

        rebuiltConfiguration.worldSize = {
            static_cast<int>(imageSize.x),
            static_cast<int>(imageSize.y),
        };


        break;
    }

    if(playground){
        std::string mapError;
        if(playgroundMap.assetId.empty()){
            mapData.emplace(int(playground->columns),int(playground->rows));mapData->AddLayer("Terrain");
        }else{
            const auto *resource=tilemapAssets.Resolve(playgroundMap);
            if(!resource||!resource->map){if(errorMessage)*errorMessage=resource?resource->error:"Missing Playground tilemap";return false;}
            mapData=*resource->map;nextMap=playgroundMap;nextMapRevision=resource->revision;
        }
        if(mapData->Columns()!=playground->columns||mapData->Rows()!=playground->rows){if(errorMessage)*errorMessage="Ant Playground dimensions must match its tilemap.";return false;}
        rebuiltConfiguration.worldSize={mapData->Columns(),mapData->Rows()};
        rebuiltConfiguration.colonyPosition={mapData->Columns()*.5f,mapData->Rows()*.5f};
    }


    std::unique_ptr<AntWorld> rebuiltWorld = std::make_unique<AntWorld>(rebuiltConfiguration, randomSeed);

    std::string localError;

    if (!rebuiltWorld->Initialize(localError)) {


        if (errorMessage != nullptr) {
            *errorMessage = std::move(localError);
        }

        return false;
    }

    if (mapData.has_value()) {
        WorldMapLoadResult mapResult;

        if (!WorldMapLoader::LoadMap(rebuiltWorld->GetEnvironment(), *mapData, mapPath, mapResult, localError)) {


            if (errorMessage != nullptr) {
                *errorMessage = std::move(localError);
            }

            return false;
        }

        rebuiltConfiguration = rebuiltWorld->GetEnvironment().GetConfiguration();
    }

    auto creationTypes=entityRegistry;
    BindAntFactories(creationTypes,*rebuiltWorld);

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != FoodSourceTypeId) {
            continue;
        }

        const float radius = std::clamp(static_cast<float>(ReadNumber(object, FoodRadiusKey, 8.0)), 0.5f, 128.0f);

        const std::int64_t quantity =
            std::clamp(ReadInteger(object, FoodAmountKey, 7), std::int64_t{0}, MaximumFoodPerCell);

        if (quantity == 0) {
            continue;
        }

        (void)rebuiltWorld->GetEnvironment().AddFoodPatch(
            object.transform.position,
            radius,
            static_cast<std::size_t>(quantity));
    }

    pipeframe::RegisteredEntityObjects rebuiltEntities;
    try {rebuiltEntities.Rebuild(rebuiltWorld->GetScene(),authoredObjects,creationTypes,componentRegistry);}
    catch(const std::exception &error) {
        if(errorMessage)*errorMessage=error.what();
        return false;
    }
    configuration=rebuiltConfiguration;authoredMap=nextMap;authoredMapRevision=nextMapRevision;
    simulationWorld = std::move(rebuiltWorld);
    simulationWorld->SetRenderingWorld(renderingWorld);
    registeredObjects=std::move(rebuiltEntities);

    renderStatistics = {};

    editorTool = AntEditorTool{};
    if(playground)editorTool.SetMode(AntEditorToolMode::None);
    editorTool.SetEnabled(viewMode!=pipeframe::ProjectRuntimeViewMode::Zen);

    antInspector.SetSelectedAnt(std::nullopt);

    colonyInspector.SetSelectedColony(std::nullopt);

    cameraInitialized = false;

    SetSelectedObject(selectedObjectId);

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    return true;
}

void AntSimulationRuntime::FitCamera(RenderContext &context) {
    if (simulationWorld == nullptr) {
        return;
    }


    const auto pixelViewport = context.GetViewportRectangle();

    const float pixelWidth = static_cast<float>(std::max(1, pixelViewport.size.x));

    const float pixelHeight = static_cast<float>(std::max(1, pixelViewport.size.y));

    const float aspectRatio = pixelWidth / pixelHeight;

    const pipeframe::Vector2f worldSize = configuration.GetWorldSizeFloat();

    pipeframe::Vector2f cameraSize = worldSize * 1.1f;

    if (cameraSize.x / cameraSize.y > aspectRatio) {
        cameraSize.y = cameraSize.x / aspectRatio;
    } else {
        cameraSize.x = cameraSize.y * aspectRatio;
    }

    context.SetCameraCenter({worldSize.x*0.5f,worldSize.y*0.5f});

    context.SetCameraSize({cameraSize.x,cameraSize.y});
    context.SetCameraZoom(1.0f);

    cameraInitialized = true;
}

void AntSimulationRuntime::DrawAuthoredSelection(RenderContext &context) const {
    if (!selectedObjectId.has_value()) {
        return;
    }

    const pipeframe::SceneObjectData *object = FindAuthoredObject(*selectedObjectId);

    if (object == nullptr) {
        return;
    }

    if(object->typeId==pipeframe::PlaygroundEntityTypeId){
        const auto entity=registeredObjects.Resolve(object->id);
        if(const auto *ground=entity.GetComponent<pipeframe::PlaygroundComponent>()){
            const auto size=ground->Size();const pipeframe::Color color{245,179,103,255};
            const pipeframe::Vertex2D edges[]={{{0,0},color},{{size.x,0},color},{{size.x,0},color},{size,color},{size,color},{{0,size.y},color},{{0,size.y},color},{{0,0},color}};
            context.GetCanvas().Draw(edges,8,pipeframe::PrimitiveTopology::Lines);
        }return;
    }
    const float radius = GetSelectionRadius(*object);

    context.GetCanvas().DrawRing(object->transform.position,radius,0.2f,{245,179,103,255});
}

pipeframe::Rectanglef AntSimulationRuntime::GetWorldViewport(const RenderContext &context) const {

    const auto size = context.GetCameraSize();

    return {
        {context.GetCameraCenter().x - size.x * 0.5f, context.GetCameraCenter().y - size.y * 0.5f},
        {size.x, size.y},
    };
}

float AntSimulationRuntime::GetRenderZoom(const RenderContext &context) const {
    const auto pixelViewport = context.GetViewportRectangle();

    const float pixelWidth = static_cast<float>(std::max(1, pixelViewport.size.x));

    const float worldWidth = std::max(0.001f, context.GetCameraSize().x);

    return pixelWidth / worldWidth;
}

const pipeframe::SceneObjectData *
AntSimulationRuntime::FindAuthoredObject(const pipeframe::SceneObjectId objectId) const {
    const auto iterator =
        std::find_if(authoredObjects.begin(), authoredObjects.end(),
                     [objectId](const pipeframe::SceneObjectData &object) { return object.id == objectId; });

    if (iterator == authoredObjects.end()) {
        return nullptr;
    }

    return &*iterator;
}

std::optional<std::int64_t> AntSimulationRuntime::GetRemainingFood(const pipeframe::SceneObjectId foodSourceId) const {
    const pipeframe::SceneObjectData *object = FindAuthoredObject(foodSourceId);

    if (object == nullptr || object->typeId != FoodSourceTypeId || simulationWorld == nullptr) {
        return std::nullopt;
    }

    const float radius = std::clamp(static_cast<float>(ReadNumber(*object, FoodRadiusKey, 8.0)), 0.5f, 128.0f);

    return GetFoodQuantityInRadius(object->transform.position, radius);
}

std::optional<std::int64_t> AntSimulationRuntime::GetDeliveredFood(const pipeframe::SceneObjectId colonyId) const {
    if (simulationWorld == nullptr) {
        return std::nullopt;
    }

    const ColonyView *colony = simulationWorld->GetColonyLifecycleSystem().FindColony(colonyId);

    if (colony == nullptr) {
        return std::nullopt;
    }

    return static_cast<std::int64_t>(colony->GetFoodQuantity());
}

std::int64_t AntSimulationRuntime::GetFoodQuantityInRadius(const pipeframe::Vector2f position, const float radius) const {
    if (simulationWorld == nullptr) {
        return 0;
    }

    const AntEnvironment &environment = simulationWorld->GetEnvironment();

    const float radiusSquared = radius * radius;

    std::uint64_t quantity{0};

    for (int y = 0; y < environment.GetHeight(); ++y) {
        for (int x = 0; x < environment.GetWidth(); ++x) {
            const pipeframe::Vector2f center = AntEnvironment::GetCellCenter({
                x,
                y,
            });

            const pipeframe::Vector2f difference =
                center - position;

            const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

            if (distanceSquared >= radiusSquared) {
                continue;
            }

            const AntWorldCell *cell = environment.TryGetCell(x, y);

            if (cell != nullptr) {
                quantity += cell->foodQuantity;
            }
        }
    }

    const std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());

    return static_cast<std::int64_t>(std::min(quantity, maximum));
}



float AntSimulationRuntime::GetSelectionRadius(const pipeframe::SceneObjectData &object) {
    if (object.typeId == SignalBeaconTypeId) {
        return std::clamp(static_cast<float>(ReadNumber(object,"radius",5.0)),0.5f,32.0f)*
            std::max(std::abs(object.transform.scale.x),std::abs(object.transform.scale.y));
    }
    if (object.typeId == ColonyTypeId) {
        return std::clamp(static_cast<float>(ReadNumber(object, SpawnRadiusKey, 4.0)), 0.5f, 64.0f);
    }

    if (object.typeId == FoodSourceTypeId) {
        return std::clamp(static_cast<float>(ReadNumber(object, FoodRadiusKey, 8.0)), 0.5f, 128.0f);
    }

    return 1.0f;
}



bool AntSimulationRuntime::ContainsPoint(const pipeframe::Vector2f center, const float radius, const pipeframe::Vector2f point) {
    const pipeframe::Vector2f difference = point - center;

    return difference.x * difference.x + difference.y * difference.y <= radius * radius;
}


} // namespace ant_simulation

namespace ant_simulation {
pipeframe::SceneObject AntSimulationRuntime::ResolveSceneObject(pipeframe::SceneObjectId id) const {
    return registeredObjects.Resolve(id);
}
bool AntSimulationRuntime::ApplyComponentEdits(std::span<const pipeframe::ProjectRuntimeComponentEdit> edits,
                                              pipeframe::ProjectRuntimeAuthoringState state) {
    if (!simulationWorld || edits.empty()) return false;
    // These domain edits rebuild food patches/map/render configuration.
    for(const auto &edit:edits) {
        const auto *object=FindAuthoredObject(edit.objectId);
        if(object && (object->typeId==FoodSourceTypeId || object->typeId==SimulationSettingsTypeId || object->typeId==pipeframe::PlaygroundEntityTypeId))return false;
    }
    std::vector<pipeframe::ComponentMutation> mutations;
    if(!ApplyAuthoredComponentEdits(authoredObjects,edits,mutations))return false;
    for (const auto &mutation : mutations) {
        // Component-level domain reaction: no field-name switches.
        if(!mutation.object.GetComponent<ColonyStateComponent>())continue;
        auto colony=ColonyView(mutation.object);
        simulationWorld->GetColonyLifecycleSystem().ApplySettings(colony.GetId(),state==pipeframe::ProjectRuntimeAuthoringState::Stopped);
    }
    liveAuthoringState=state;
    return true;
}
std::vector<pipeframe::ProjectRuntimeComponentEdit> AntSimulationRuntime::GetLiveComponentProperties() const {
    std::vector<pipeframe::ProjectRuntimeComponentEdit> result;
    for (const auto &object : authoredObjects)
        if (const auto components=InspectObjectComponents(object.id))
            for (const auto &component : *components)
                for (const auto &[key,value] : component.properties) result.push_back({object.id,component.typeId,key,value});
    return result;
}
}

namespace ant_simulation {
void AntSimulationRuntime::RefreshAuthoredEnvironment(){
    if(authoredMap.assetId.empty())return;
    const auto *resource=tilemapAssets.Resolve(authoredMap);
    if(resource && resource->map && resource->revision!=authoredMapRevision){
        std::string error;
        if(RebuildSimulation(&error))simulationElapsedTime=0;
        else {authoredMapRevision=resource->revision;if(environmentLog)environmentLog->Write("environment",error);}
    }
}
}

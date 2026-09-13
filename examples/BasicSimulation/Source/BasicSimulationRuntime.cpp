#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include "BasicSimulationRuntime.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <PipeFrame/Render/RenderServices2D.h>

#include "BasicSimulationTypes.h"
#include "PopulationLod.h"
#include "Components/ComponentContract.h"
#include "Systems/SystemContract.h"
#include "Runtime/RuntimeContract.h"
#include "Editor/ExtensionContract.h"

namespace basic_simulation {

const char *
BasicSimulationRuntime::GetName() const {
    return "Basic Simulation";
}

pipeframe::ProjectPluginDescriptor BasicSimulationRuntime::GetPluginDescriptor() const {
    return contract::Descriptor();
}

bool BasicSimulationRuntime::RegisterPlugin(pipeframe::PluginRegistrar &registrar,
                                            std::string &error) {
    pipeframe::ExtensionDescriptor overlay{contract::PopulationOverlay, {}, "Population Overlay",
                                           pipeframe::ExtensionPoint::Overlay, "scene"};
    overlay.invoke=[this](const auto &){populationRenderer.BeginFrame();};
    pipeframe::ExtensionDescriptor setting{contract::PopulationSettings, {}, "Population Settings",
                                           pipeframe::ExtensionPoint::Setting, "project"};
    setting.invoke=[this](const auto &){RebuildRuntimeObjects(false);};
    if (!registrar.Extension(std::move(overlay), &error) ||
        !registrar.Extension(std::move(setting), &error)) return false;
    pipeframe::ProjectSystemDescriptor simulation{contract::SimulationSystem, {}, pipeframe::SystemPhase::FixedBehavior, {},
        {DemoAgentTypeId, PopulationTypeId}, {DemoAgentTypeId, PopulationTypeId},false,false,false,
        [this](pipeframe::SystemContext &context) {
            if(context.components)context.components->With(PopulationTypeId);
            if(auto *spatial=context.services->Find<pipeframe::UniformSpatialIndex<pipeframe::SceneObjectId>>())
                (void)spatial->QueryRadius({},512.0f);
            context.jobs->Submit(context.tick,[this,delta=context.deltaTime]{FixedUpdate(delta);});
            context.events->Publish({"basic.tick",std::to_string(context.tick)});
            if(context.tick==1)context.log->Write("basic","scheduled simulation started");
        }};
    simulation.requiredServices={std::string(pipeframe::SpatialServiceId),std::string(pipeframe::EventServiceId),
        std::string(pipeframe::JobServiceId),std::string(pipeframe::LogServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    if (!registrar.System(std::move(simulation),&error)) return false;
    pipeframe::ProjectSystemDescriptor render{contract::RenderSystem, {}, pipeframe::SystemPhase::Render,
        {contract::SimulationSystem}, {DemoAgentTypeId, PopulationTypeId}, {},false,false,false,
        [this](pipeframe::SystemContext &context) {
            (void)context.services->Find<pipeframe::GraphicsResourceService>();
            (void)context.services->Find<pipeframe::SurfaceRegistry>();
            if(auto *target=context.services->Find<RenderContext>())Render(*target);
        }};
    render.requiredServices={std::string(pipeframe::RenderServiceId),std::string(pipeframe::ResourceServiceId),
        std::string(pipeframe::ProfilingServiceId)};
    return registrar.System(std::move(render), &error);
}

bool BasicSimulationRuntime::Load(
    const pipeframe::ProjectRuntimeContext &context,
    std::string &errorMessage) {

    (void)context;

    errorMessage.clear();

    authoredObjects.clear();
    agents.clear();
    populations.clear();

    selectedObjectId.reset();
    playing = false;

    return true;
}

std::span<
    const pipeframe::SceneObjectTypeDescriptor>
BasicSimulationRuntime::GetSceneObjectTypes()
    const {

    return objectTypes;
}

pipeframe::SceneObjectData
BasicSimulationRuntime::CreateDefaultObject(
    const pipeframe::SceneObjectTypeId
        &typeId) const {

    pipeframe::SceneObjectData object =
        ProjectRuntime::CreateDefaultObject(
            typeId);

    if (typeId == DemoAgentTypeId) {
        object.name = "DEMO AGENT";
    } else if (typeId == PopulationTypeId) {
        object.name = "AGENT POPULATION";
    }

    return object;
}

void BasicSimulationRuntime::SynchronizeScene(
    std::span<
        const pipeframe::SceneObjectData>
        objects) {

    authoredObjects.assign(
        objects.begin(),
        objects.end());

    RebuildRuntimeObjects(false);
}

void BasicSimulationRuntime::SetSelectedObject(
    std::optional<pipeframe::SceneObjectId>
        objectId) {

    selectedObjectId = objectId;

    for (DemoAgent &agent : agents) {
        agent.SetSelected(
            selectedObjectId.has_value() &&
            agent.GetId() ==
                *selectedObjectId);
    }
}

std::optional<pipeframe::SceneObjectId>
BasicSimulationRuntime::HitTest(
    const pipeframe::Vector2f worldPosition) const {

    for (auto iterator = agents.rbegin();
         iterator != agents.rend();
         ++iterator) {

        if (iterator->Contains(pipeframe::backend::sfml::ToBackend(worldPosition))) {
            return iterator->GetId();
        }
    }

    for (auto iterator = populations.rbegin();
         iterator != populations.rend();
         ++iterator) {

        if (iterator->Contains(pipeframe::backend::sfml::ToBackend(worldPosition))) {
            return iterator->GetSourceObjectId();
        }
    }

    return std::nullopt;
}

void BasicSimulationRuntime::Start() {
    playing = true;

    for (DemoAgent &agent : agents) {
        agent.SetSimulationPlaying(true);
    }
}

void BasicSimulationRuntime::FixedUpdate(
    const float fixedDeltaTime) {

    if (!playing) {
        return;
    }

    for (DemoAgent &agent : agents) {
        agent.FixedUpdate(fixedDeltaTime);
    }

    for (Population &population : populations) {
        population.FixedUpdate(fixedDeltaTime);
        population.RebuildSpatialGrid();
    }
}

void BasicSimulationRuntime::Render(
    RenderContext &context) {

    Camera2D &camera =
        context.GetCamera();

    const sf::Vector2f cameraSize =
        {camera.GetSize().x,camera.GetSize().y};

    const sf::Vector2f cameraCenter =
        {camera.GetCenter().x,camera.GetCenter().y};

    const sf::FloatRect worldViewport{
        cameraCenter -
            cameraSize * 0.5f,
        cameraSize,
    };

    populationRenderer.SetMode(
        SelectPopulationRenderMode(
            populationRenderer.GetMode(),
            camera.GetZoom()));

    populationRenderer.BeginFrame();

    for (const Population &population :
         populations) {

        const bool selected =
            selectedObjectId.has_value() &&
            population.GetSourceObjectId() ==
                *selectedObjectId;

        population.RenderBounds(
            pipeframe::backend::sfml::GetWindow(context),
            selected);

        populationRenderer.Render(
            pipeframe::backend::sfml::GetWindow(context),
            population,
            worldViewport);
    }

    for (const DemoAgent &agent : agents) {
        agent.Render(pipeframe::backend::sfml::GetWindow(context));
    }
}

void BasicSimulationRuntime::Reset() {
    RebuildRuntimeObjects(true);
}

void BasicSimulationRuntime::Stop() {
    playing = false;

    for (DemoAgent &agent : agents) {
        agent.SetSimulationPlaying(false);
    }
}

void BasicSimulationRuntime::Unload() {
    playing = false;

    agents.clear();
    populations.clear();
    authoredObjects.clear();

    selectedObjectId.reset();

    populationRenderer.BeginFrame();
}

std::vector<
    pipeframe::SceneObjectTypeDescriptor>
BasicSimulationRuntime::CreateObjectTypes() {

    pipeframe::SceneObjectTypeDescriptor
        agentType;

    agentType.typeId = DemoAgentTypeId;
    agentType.displayName = "DEMO AGENT";

    agentType.properties.push_back({
        .key = AngularSpeedKey,
        .displayName = "ANGULAR SPEED",
        .kind =
            pipeframe::PropertyKind::Number,
        .defaultValue = 90.0,
        .editable = true,
    });

    pipeframe::SceneObjectTypeDescriptor
        populationType;

    populationType.typeId = PopulationTypeId;
    populationType.displayName =
        "AGENT POPULATION";

    populationType.properties.push_back({
        .key = AgentCountKey,
        .displayName = "AGENT COUNT",
        .kind =
            pipeframe::PropertyKind::Integer,
        .defaultValue =
            std::int64_t{100'000},
        .editable = true,
    });

    populationType.properties.push_back({
        .key = SpawnAreaKey,
        .displayName = "SPAWN AREA",
        .kind =
            pipeframe::PropertyKind::Vector2,
        .defaultValue =
            pipeframe::Vector2f{
                4000.0f,
                4000.0f,
            },
        .editable = true,
    });

    populationType.properties.push_back({
        .key = RandomSeedKey,
        .displayName = "RANDOM SEED",
        .kind =
            pipeframe::PropertyKind::Integer,
        .defaultValue =
            std::int64_t{1},
        .editable = true,
    });

    return {
        std::move(agentType),
        std::move(populationType),
    };
}

void BasicSimulationRuntime::
    RebuildRuntimeObjects(
        const bool forcePopulationRebuild) {

    agents.clear();

    std::vector<Population>
        previousPopulations =
            std::move(populations);

    populations.clear();

    for (const pipeframe::SceneObjectData &object :
         authoredObjects) {

        if (object.typeId == DemoAgentTypeId) {
            agents.emplace_back(
                object.id,
                object.name,
                pipeframe::backend::sfml::ToBackend(object.transform.position));

            DemoAgent &agent = agents.back();

            agent.SetRotation(
                object.transform.rotation);

            agent.SetAngularSpeed(
                static_cast<float>(
                    ReadNumber(
                        object,
                        AngularSpeedKey,
                        90.0)));

            agent.SetSimulationPlaying(playing);

            agent.SetSelected(
                selectedObjectId.has_value() &&
                object.id ==
                    *selectedObjectId);

            continue;
        }

        if (object.typeId != PopulationTypeId) {
            continue;
        }

        auto reusablePopulation =
            std::find_if(
                previousPopulations.begin(),
                previousPopulations.end(),
                [&object](
                    const Population &population) {

                    return population
                        .GetSourceObjectId() ==
                        object.id;
                });

        if (!forcePopulationRebuild &&
            reusablePopulation !=
                previousPopulations.end() &&
            reusablePopulation->MatchesSource(
                object)) {

            populations.push_back(
                std::move(
                    *reusablePopulation));

            continue;
        }

        Population population;

        if (population.Initialize(object)) {
            populations.push_back(
                std::move(population));
        }
    }
}

double BasicSimulationRuntime::ReadNumber(
    const pipeframe::SceneObjectData &object,
    const std::string &key,
    const double defaultValue) {

    const auto iterator =
        object.properties.find(key);

    if (iterator ==
        object.properties.end()) {

        return defaultValue;
    }

    if (const auto *number =
            std::get_if<double>(
                &iterator->second)) {

        return *number;
    }

    if (const auto *integer =
            std::get_if<std::int64_t>(
                &iterator->second)) {

        return static_cast<double>(*integer);
    }

    return defaultValue;
}

} // namespace basic_simulation

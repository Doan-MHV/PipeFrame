#include "AntSimulationModule.h"

#include <cmath>
#include <cstdio>

#include "Entity/AntSwarm.h"
#include "Entity/Colony.h"
#include "Entity/FoodSource.h"
#include "Generated/ProjectComponents.generated.h"
#include "Reflection/EditorMetadata.h"
#include "ECS/Registry.h"

std::string AntSimulationModule::GetName() const
{
    return "AntSimulationDemo";
}

void AntSimulationModule::RegisterEntitySystems(Registry& registry)
{
    (void)registry;
}

void AntSimulationModule::OnWorldLoaded(Registry& registry)
{
    antSwarmSystem.OnWorldLoaded(antSwarmSimulation, registry);
}

void AntSimulationModule::RegisterComponents(ComponentRegistry& registry)
{
    RegisterGeneratedProjectComponents(registry);
}

void AntSimulationModule::RegisterEntityClasses(ClassRegistry& registry)
{
    registry.RegisterEntityClass({
        .typeName = "Colony",
        .displayName = "Colony",
        .category = "Ant Simulation",
        .create = CreateColonyEntity
    });
    registry.RegisterEntityClass({
        .typeName = "FoodSource",
        .displayName = "Food Source",
        .category = "Ant Simulation",
        .create = CreateFoodSourceEntity
    });
    registry.RegisterEntityClass({
        .typeName = "AntSwarm",
        .displayName = "Ant Swarm",
        .category = "Ant Simulation",
        .create = CreateAntSwarmEntity
    });
}

void AntSimulationModule::ResetProjectSimulation()
{
    antSwarmSimulation.Reset();
}

void AntSimulationModule::UpdateProjectSimulation(ProjectRuntimeContext& context)
{
    antSwarmSystem.Update(antSwarmSimulation, context);
}

void AntSimulationModule::RenderProjectSimulation(
    SDL_Renderer* renderer,
    AssetRegistry& assetRegistry,
    const SDL_FRect& camera
)
{
    antSwarmRenderPass.Render(antSwarmSimulation, renderer, assetRegistry, camera);
}

const std::unordered_map<std::string, FieldGrid>& AntSimulationModule::GetFieldGrids() const
{
    return antSwarmSimulation.GetFields();
}

int AntSimulationModule::PickProjectObject(glm::vec2 worldPosition, float radius)
{
    int selectedId = -1;
    float bestDistance = radius;

    for (const AntAgent& ant : antSwarmSimulation.Agents())
    {
        const float distance = glm::length(ant.position - worldPosition);
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            selectedId = ant.id;
        }
    }

    return selectedId;
}

void AntSimulationModule::SetSelectedProjectObject(int id)
{
    antSwarmSimulation.SetSelectedAgentId(id);
}

ProjectObjectInspector AntSimulationModule::GetSelectedProjectObjectInspector() const
{
    const int selectedId = antSwarmSimulation.GetSelectedAgentId();
    for (const AntAgent& ant : antSwarmSimulation.Agents())
    {
        if (ant.id != selectedId)
        {
            continue;
        }

        auto formatFloat = [](float value)
        {
            char buffer[64] = {};
            std::snprintf(buffer, sizeof(buffer), "%.2f", value);
            return std::string(buffer);
        };

        auto stateName = [](AntState state)
        {
            switch (state)
            {
            case AntState::ToFood:
                return "To Food";
            case AntState::ToHomeWithFood:
                return "To Home With Food";
            case AntState::ToHomeNoFood:
                return "To Home No Food";
            }
            return "Unknown";
        };

        ProjectObjectInspector inspector;
        inspector.id = ant.id;
        inspector.typeName = "AntAgent";
        inspector.displayName = "Ant Agent";
        inspector.properties = {
            {"State", stateName(ant.state)},
            {"Role", ant.role == AntRole::Explorer ? "Explorer" : "Follower"},
            {"Speed", formatFloat(ant.currentSpeed)},
            {"Energy", formatFloat(ant.energy)},
            {"Blocked", ant.blocked ? "true" : "false"},
            {"Collected Food", std::to_string(ant.collectedFood)},
            {"Total Distance", formatFloat(ant.totalTravelDistance)}
        };
        return inspector;
    }

    return {};
}

extern "C" ProjectModule* CreateProjectModule()
{
    return new AntSimulationModule();
}

extern "C" void DestroyProjectModule(ProjectModule* module)
{
    delete module;
}

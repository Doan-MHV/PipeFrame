#ifndef PIPEFRAME_ANT_SIMULATION_MODULE_H
#define PIPEFRAME_ANT_SIMULATION_MODULE_H

#include "Simulation/ProjectModule.h"
#include "Simulations/AntSwarmSimulation.h"
#include "Systems/AntSwarmRenderPass.h"
#include "Systems/AntSwarmSimulationSystem.h"

class AntSimulationModule : public ProjectModule
{
private:
    AntSwarmSimulation antSwarmSimulation;
    AntSwarmSimulationSystem antSwarmSystem;
    AntSwarmRenderPass antSwarmRenderPass;

public:
    std::string GetName() const override;
    void RegisterComponents(ComponentRegistry& registry) override;
    void RegisterEntityClasses(ClassRegistry& registry) override;
    void RegisterEntitySystems(Registry& registry) override;
    void OnWorldLoaded(Registry& registry) override;
    void ResetProjectSimulation() override;
    void UpdateProjectSimulation(ProjectRuntimeContext& context) override;
    void RenderProjectSimulation(
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        const SDL_FRect& camera
    ) override;
    const std::unordered_map<std::string, FieldGrid>& GetFieldGrids() const override;
    int PickProjectObject(glm::vec2 worldPosition, float radius) override;
    void SetSelectedProjectObject(int id) override;
    ProjectObjectInspector GetSelectedProjectObjectInspector() const override;
};

#endif // PIPEFRAME_ANT_SIMULATION_MODULE_H

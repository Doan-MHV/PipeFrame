#ifndef FlappyBird_MODULE_H
#define FlappyBird_MODULE_H

#include "HUD/FlappyHud.h"
#include "Simulation/ProjectModule.h"
#include "State/FlappyGameState.h"

class FlappyBirdModule : public ProjectModule
{
public:
    std::string GetName() const override;
    void RegisterComponents(ComponentRegistry& registry) override;
    void RegisterEntityClasses(ClassRegistry& registry) override;
    void RegisterEntitySystems(Registry& registry) override;
    void Loaded(ProjectRuntimeContext& context) override;
    void Start(ProjectRuntimeContext& context) override;
    void Update(ProjectRuntimeContext& context) override;
    void Stop(ProjectRuntimeContext& context) override;
    void Unloaded(ProjectRuntimeContext& context) override;
    void RenderProjectSimulation(
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        const SDL_FRect& camera
    ) override;

    void RenderHud(HudContext& context) override;

private:
    FlappyGameState gameState;
    FlappyHud hud;
};

#endif // FlappyBird_MODULE_H

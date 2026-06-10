#include "FlappyBirdModule.h"
#include "Entity/Bird.h"
#include "Systems/BirdSystem.h"
#include "Systems/FlappyBoundsSystem.h"
#include "Systems/FlappyDeathSystem.h"
#include "Entity/Pipe.h"
#include "Entity/PipeSpawner.h"
#include "Systems/PipeSpawnerSystem.h"
#include "Systems/LevelExitSystem.h"
#include "Entity/LevelExit.h"
#include "Entity/ScoreZone.h"

#include "Components/BirdComponent.h"
#include "Components/ScoreComponent.h"
#include "Generated/ProjectComponents.generated.h"
#include "Reflection/EditorMetadata.h"
#include "ECS/Registry.h"
#include "Systems/ScoreSystem.h"
#include "UI/HudContext.h"

std::string FlappyBirdModule::GetName() const
{
    return "FlappyBird";
}

void FlappyBirdModule::RegisterComponents(ComponentRegistry& registry)
{
    RegisterGeneratedProjectComponents(registry);
}

void FlappyBirdModule::RegisterEntityClasses(ClassRegistry& registry)
{
    registry.RegisterEntityClass({
        .typeName = "Bird",
        .displayName = "Bird",
        .category = "Project",
        .create = Bird::Create
    });
    registry.RegisterEntityClass({
        .typeName = "Pipe",
        .displayName = "Pipe",
        .category = "Project",
        .create = Pipe::Create
    });
    registry.RegisterEntityClass({
        .typeName = "PipeSpawner",
        .displayName = "PipeSpawner",
        .category = "Project",
        .create = PipeSpawner::Create
    });
    registry.RegisterEntityClass({
        .typeName = "LevelExit",
        .displayName = "LevelExit",
        .category = "Project",
        .create = LevelExit::Create
    });
    registry.RegisterEntityClass({
        .typeName = "ScoreZone",
        .displayName = "Score Zone",
        .category = "Project",
        .create = ScoreZone::Create
    });
}

void FlappyBirdModule::RegisterEntitySystems(Registry& registry)
{
    registry.AddSystem<BirdSystem>();
    registry.AddSystem<FlappyBoundsSystem>();
    registry.AddSystem<FlappyDeathSystem>();
    registry.AddSystem<PipeSpawnerSystem>();
    registry.AddSystem<ScoreSystem>();
    registry.AddSystem<LevelExitSystem>();
}

void FlappyBirdModule::Loaded(ProjectRuntimeContext& context)
{
    (void)context;
}

void FlappyBirdModule::Start(ProjectRuntimeContext& context)
{
    (void)context;
}

void FlappyBirdModule::Update(ProjectRuntimeContext& context)
{
    gameState.score = 0;
    gameState.gameOver = false;

    for (const auto entity : context.registry.GetAllEntities())
    {
        if (!entity.HasComponent<BirdComponent>() || !entity.HasComponent<ScoreComponent>())
        {
            continue;
        }

        const auto& bird = entity.GetComponent<BirdComponent>();
        const auto& score = entity.GetComponent<ScoreComponent>();

        gameState.score = score.score;
        gameState.bestScore = score.bestScore;
        gameState.gameOver = !bird.isAlive;
        return;
    }
}

void FlappyBirdModule::Stop(ProjectRuntimeContext& context)
{
    (void)context;
}

void FlappyBirdModule::Unloaded(ProjectRuntimeContext& context)
{
    (void)context;
}

void FlappyBirdModule::RenderProjectSimulation(
    SDL_Renderer* renderer,
    AssetRegistry& assetRegistry,
    const SDL_FRect& camera
)
{
    (void)renderer;
    (void)assetRegistry;
    (void)camera;
}

void FlappyBirdModule::RenderHud(HudContext& context)
{
    hud.Render(context, gameState);
}

extern "C" ProjectModule* CreateProjectModule()
{
    return new FlappyBirdModule();
}

extern "C" void DestroyProjectModule(ProjectModule* module)
{
    delete module;
}

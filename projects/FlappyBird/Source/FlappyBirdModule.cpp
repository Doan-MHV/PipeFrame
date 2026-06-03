#include "FlappyBirdModule.h"
#include "Entity/Bird.h"
#include "Systems/BirdSystem.h"

#include "Entity/ExampleEntity.h"
#include "Generated/ProjectComponents.generated.h"
#include "Systems/ExampleEntitySystem.h"
#include "Reflection/EditorMetadata.h"
#include "ECS/Registry.h"

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
        .typeName = "ExampleEntity",
        .displayName = "Example Entity",
        .category = "Entities",
        .create = ExampleEntity::Create
    });
    registry.RegisterEntityClass({
        .typeName = "Bird",
        .displayName = "Bird",
        .category = "Project",
        .create = Bird::Create
    });
}

void FlappyBirdModule::RegisterEntitySystems(Registry& registry)
{
    registry.AddSystem<ExampleEntitySystem>();
    registry.AddSystem<BirdSystem>();
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
    (void)context;
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

extern "C" ProjectModule* CreateProjectModule()
{
    return new FlappyBirdModule();
}

extern "C" void DestroyProjectModule(ProjectModule* module)
{
    delete module;
}

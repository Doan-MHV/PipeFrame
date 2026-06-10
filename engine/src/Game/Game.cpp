#include "Game.h"

#include "Components/EngineComponents.h"
#include "Components/PersistentIdComponent.h"
#include "Components/SpriteComponent.h"
#include "ECS/ECS.h"
#include "Logger/Logger.h"
#include "Systems/MovementSystem.h"
#include "Systems/SpriteRenderSystem.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <utility>

#include "EntitySerializer.h"
#include "LevelLoader.h"

#include "Events/KeyPressedEvent.h"
#include "Simulation/SimulationUpdatePhase.h"
#include "Systems/AnimationSystem.h"
#include "Systems/CameraMovementSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/DamageSystem.h"
#include "Systems/DestroyWhenOffscreenSystem.h"
#include "Systems/KeyboardControlSystem.h"
#include "Systems/ProjectileEmitSystem.h"
#include "Systems/ProjectileLifecycleSystem.h"
#include "Systems/RenderHealthBarSystem.h"
#include "Systems/RenderTextSystem.h"
#include "Systems/SoftCollisionSystem.h"
#include "Systems/TerrainConstraintSystem.h"
#include "UI/HudContext.h"

namespace
{
constexpr int DefaultViewportWidth = 1600;
constexpr int DefaultViewportHeight = 900;

void ClampCameraToTileMap(SDL_FRect& camera, const TileMap* tileMap)
{
    if (!tileMap)
    {
        return;
    }

    float maxCameraX = static_cast<float>(tileMap->GetWorldWidth()) - camera.w;
    float maxCameraY = static_cast<float>(tileMap->GetWorldHeight()) - camera.h;

    if (maxCameraX < 0.0f)
    {
        maxCameraX = 0.0f;
    }

    if (maxCameraY < 0.0f)
    {
        maxCameraY = 0.0f;
    }

    if (camera.x < 0.0f)
    {
        camera.x = 0.0f;
    }

    if (camera.y < 0.0f)
    {
        camera.y = 0.0f;
    }

    if (camera.x > maxCameraX)
    {
        camera.x = maxCameraX;
    }

    if (camera.y > maxCameraY)
    {
        camera.y = maxCameraY;
    }
}

Entity FindEntityById(const std::unique_ptr<Registry>& registry, int entityId)
{
    if (!registry)
    {
        return Entity(-1);
    }

    for (auto entity : registry->GetAllEntities())
    {
        if (entity.GetId() == entityId)
        {
            return entity;
        }
    }

    return Entity(-1);
}

std::string SanitizePrefabId(const std::string& prefabName)
{
    std::string prefabId;

    for (const unsigned char ch : prefabName)
    {
        if (std::isalnum(ch) || ch == '_' || ch == '-')
        {
            prefabId += static_cast<char>(ch);
        }
        else if (std::isspace(ch))
        {
            prefabId += '_';
        }
    }

    return prefabId;
}

std::string ToLower(std::string value)
{
    for (char& ch : value)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    return value;
}

nlohmann::json BuildSpriteMetadataJson(const TextureSpriteMetadata& metadata)
{
    nlohmann::json spriteJson;
    spriteJson["mode"] = metadata.mode == TextureSpriteMode::SpriteSheet ? "sheet" : "image";
    spriteJson["default_display_size"] = {
        {"w", metadata.defaultDisplayWidth},
        {"h", metadata.defaultDisplayHeight}
    };

    if (metadata.mode == TextureSpriteMode::SpriteSheet)
    {
        spriteJson["frame_width"] = metadata.frameWidth;
        spriteJson["frame_height"] = metadata.frameHeight;
        spriteJson["default_frame"] = metadata.defaultFrame;
    }

    return spriteJson;
}

}

Game::Game(ProjectConfig projectConfig): projectConfig(std::move(projectConfig))
{
    registry = std::make_unique<Registry>();
    assetRegistry = std::make_unique<AssetRegistry>();
    eventBus = std::make_unique<EventBus>();
    Logger::Log("Game constructor called!");
}

Game::~Game()
{
    Logger::Log("Game destructor called!");
}


void Game::EnterPlayMode()
{
    CaptureWorldSnapshot();
    RestoreWorldSnapshot();

    mode = EngineMode::Play;
    elapsedTime = 0;
    millisecsPreviousFrame = SDL_GetTicks();
    NotifyEngineSystemsStart();
    NotifyProjectStart();
}

void Game::EnterEditMode()
{
    NotifyProjectStop();
    NotifyEngineSystemsStop();
    RestoreWorldSnapshot();

    mode = EngineMode::Edit;
    elapsedTime = 0;
    millisecsPreviousFrame = SDL_GetTicks();
}

void Game::ToggleMode()
{
    if (mode == EngineMode::Edit)
    {
        EnterPlayMode();
    }
    else
    {
        EnterEditMode();
    }
}

void Game::SetPlaySpeed(float speed)
{
    playSpeed = std::clamp(speed, 0.0f, 8.0f);
}

EntitySystemContext Game::CreateEntitySystemContext(double deltaTime)
{
    return EntitySystemContext{
        *registry,
        *eventBus,
        *tileMap,
        *assetRegistry,
        renderer,
        camera,
        levelLoadRequests,
        deltaTime,
        elapsedTime
    };
}

ProjectRuntimeContext Game::CreateProjectRuntimeContext(double deltaTime)
{
    return ProjectRuntimeContext{
        *registry,
        componentRegistry,
        *tileMap,
        prefabRegistry,
        projectConfig,
        levelLoadRequests,
        deltaTime,
        elapsedTime
    };
}

void Game::NotifyEngineSystemsLoaded()
{
    if (registry)
    {
        registry->LoadedSystems();
    }
}

void Game::NotifyEngineSystemsStart()
{
    if (registry && eventBus && tileMap && assetRegistry)
    {
        EntitySystemContext context = CreateEntitySystemContext();
        registry->SubscribeSystems(context);
        registry->StartSystems(context);
    }
}

void Game::NotifyEngineSystemsStop()
{
    if (registry && eventBus && tileMap && assetRegistry)
    {
        EntitySystemContext context = CreateEntitySystemContext();
        registry->StopSystems(context);
    }
}

void Game::NotifyEngineSystemsUnloaded()
{
    if (registry && eventBus && tileMap && assetRegistry)
    {
        EntitySystemContext context = CreateEntitySystemContext();
        registry->UnloadedSystems(context);
    }
}

void Game::NotifyProjectLoaded()
{
    if (projectModule && registry && tileMap)
    {
        ProjectRuntimeContext context = CreateProjectRuntimeContext();
        projectModule->Loaded(context);
    }
}

void Game::NotifyProjectStart()
{
    if (projectModule && registry && tileMap)
    {
        ProjectRuntimeContext context = CreateProjectRuntimeContext();
        projectModule->Start(context);
    }
}

void Game::NotifyProjectStop()
{
    if (projectModule && registry && tileMap)
    {
        ProjectRuntimeContext context = CreateProjectRuntimeContext();
        projectModule->Stop(context);
    }
}

void Game::NotifyProjectUnloaded()
{
    if (projectModule && registry && tileMap)
    {
        ProjectRuntimeContext context = CreateProjectRuntimeContext();
        projectModule->Unloaded(context);
    }
}

void Game::ResetWorldRuntime(bool registerProjectSystems)
{
    registry = std::make_unique<Registry>();
    eventBus = std::make_unique<EventBus>();

    registry->AddManualSystem<MovementSystem>();
    registry->AddManualSystem<TerrainConstraintSystem>();
    registry->AddManualSystem<SoftCollisionSystem>();
    registry->AddManualSystem<SpriteRenderSystem>();
    registry->AddManualSystem<AnimationSystem>();
    registry->AddManualSystem<CollisionSystem>();
    registry->AddManualSystem<DamageSystem>();
    registry->AddManualSystem<KeyboardControlSystem>();
    registry->AddManualSystem<CameraMovementSystem>();
    registry->AddManualSystem<ProjectileEmitSystem>();
    registry->AddManualSystem<ProjectileLifecycleSystem>();
    registry->AddManualSystem<DestroyWhenOffscreenSystem>();
    registry->AddManualSystem<RenderTextSystem>();
    registry->AddManualSystem<RenderHealthBarSystem>();

    if (projectModule && registerProjectSystems)
    {
        projectModule->RegisterEntitySystems(*registry);
    }

    NotifyEngineSystemsLoaded();
}

void Game::RebuildWorld()
{
    NotifyEngineSystemsUnloaded();
    NotifyProjectUnloaded();
    ResetWorldRuntime();

    LevelLoader loader;
    loader.LoadLevel(
        registry,
        assetRegistry,
        renderer,
        tileMap,
        projectConfig.startupLevelPath.string(),
        &componentRegistry,
        &currentLevelFilePaths
    );
    NotifyProjectLoaded();
}

void Game::CaptureWorldSnapshot()
{
    if (!registry || !tileMap)
    {
        Logger::Err("Cannot capture world snapshot: registry or tile map is missing");
        return;
    }

    WorldSnapshot snapshot;
    snapshot.entitiesJson = EntitySerializer::SerializeEntities(registry, &componentRegistry);

    snapshot.rows = tileMap->GetRows();
    snapshot.cols = tileMap->GetCols();
    snapshot.tileSize = tileMap->GetTileSize();
    snapshot.scale = tileMap->GetScale();
    snapshot.textureAssetId = tileMap->GetTextureAssetId();
    snapshot.levelFilePaths = currentLevelFilePaths;

    snapshot.tiles.reserve(snapshot.rows * snapshot.cols);

    for (int row = 0; row < snapshot.rows; ++row)
    {
        for (int col = 0; col < snapshot.cols; ++col)
        {
            snapshot.tiles.push_back(tileMap->GetTile(row, col));
        }
    }

    authoredSnapshot = std::move(snapshot);
    Logger::Log("Captured authored world snapshot");
}

void Game::RestoreWorldSnapshot()
{
    if (!authoredSnapshot.has_value())
    {
        Logger::Err("Cannot restore world snapshot: no authored snapshot available");
        return;
    }

    NotifyEngineSystemsUnloaded();
    NotifyProjectUnloaded();
    ResetWorldRuntime();

    const WorldSnapshot& snapshot = *authoredSnapshot;

    tileMap = std::make_unique<TileMap>(
        snapshot.rows,
        snapshot.cols,
        snapshot.tileSize,
        snapshot.scale
    );

    tileMap->SetTextureAssetId(snapshot.textureAssetId);
    currentLevelFilePaths = snapshot.levelFilePaths;

    for (int row = 0; row < snapshot.rows; ++row)
    {
        for (int col = 0; col < snapshot.cols; ++col)
        {
            tileMap->GetTile(row, col) = snapshot.tiles[row * snapshot.cols + col];
        }
    }

    LevelLoader loader;
    loader.LoadEntitiesFromJson(snapshot.entitiesJson, registry, &componentRegistry);
    NotifyProjectLoaded();

    Logger::Log("Restored world from authored snapshot");
}

void Game::HandleEvent(const SDL_Event& event, bool& shouldQuit)
{
    switch (event.type)
    {
    case SDL_EVENT_QUIT:
        shouldQuit = true;
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE)
        {
            shouldQuit = true;
        }
        if (event.key.key == SDLK_F1)
        {
            ToggleMode();
        }
        if (mode == EngineMode::Play)
        {
            EntitySystemContext context = CreateEntitySystemContext();
            eventBus->EmitEventWithContext<KeyPressedEvent>(context, event.key.key);
        }
        break;
    default:
        break;
    }
}

void Game::RenderSceneToViewportTexture()
{
    if (!viewportTexture)
    {
        return;
    }

    // Make camera match viewport render target size
    camera.w = static_cast<float>(viewportTextureWidth);
    camera.h = static_cast<float>(viewportTextureHeight);

    SDL_SetRenderTarget(renderer, viewportTexture);

    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    EntitySystemContext context = CreateEntitySystemContext();
    registry->GetSystem<SpriteRenderSystem>().RenderLayerPass(context, true);

    if (tileMap && tileMapRenderer)
    {
        tileMapRenderer->Render(
            renderer,
            assetRegistry->GetTexture(tileMap->GetTextureAssetId()),
            *tileMap,
            camera
        );
    }

    registry->GetSystem<SpriteRenderSystem>().Update(context);
    registry->GetSystem<RenderTextSystem>().Update(context);
    registry->GetSystem<RenderHealthBarSystem>().Update(context);

    if (projectModule)
    {
        projectModule->RenderProjectSimulation(renderer, *assetRegistry, camera);
    }

    RenderProjectHud(viewportTextureWidth, viewportTextureHeight);

    SDL_SetRenderTarget(renderer, nullptr);
}

void Game::EnsureViewportTexture(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (viewportTexture &&
        viewportTextureWidth == width &&
        viewportTextureHeight == height)
    {
        return;
    }

    if (viewportTexture)
    {
        SDL_DestroyTexture(viewportTexture);
        viewportTexture = nullptr;
    }

    viewportTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );

    if (!viewportTexture)
    {
        Logger::Err("Failed to create viewport texture.");
        return;
    }

    viewportTextureWidth = width;
    viewportTextureHeight = height;
}

void Game::Initialize(SDL_Renderer* renderer)
{
    this->renderer = renderer;

    camera.x = 0.0;
    camera.y = 0.0;
    camera.w = static_cast<float>(DefaultViewportWidth);
    camera.h = static_cast<float>(DefaultViewportHeight);
}

void Game::Setup()
{
    tileMapRenderer = std::make_unique<TilemapRenderer>();

    LevelLoader loader;
    loader.LoadAssetManifest(assetRegistry, renderer, projectConfig.assetManifestPath.string());
    prefabRegistry.LoadFromDirectory(projectConfig.prefabDirectory);

    RebuildWorld();
    CaptureWorldSnapshot();

    millisecsPreviousFrame = SDL_GetTicks();
    elapsedTime = 0;
}

bool Game::LoadProject(ProjectConfig newProjectConfig)
{
    if (!renderer)
    {
        Logger::Err("Cannot load project before renderer is initialized");
        return false;
    }

    mode = EngineMode::Edit;
    authoredSnapshot.reset();
    projectConfig = std::move(newProjectConfig);

    assetRegistry = std::make_unique<AssetRegistry>();
    camera.x = 0.0f;
    camera.y = 0.0f;

    Setup();
    return tileMap != nullptr;
}

void Game::SetProjectModule(std::shared_ptr<ProjectModule> module)
{
    projectModule = std::move(module);
    componentRegistry.Clear();
    classRegistry.Clear();

    RegisterEngineComponents(componentRegistry);

    if (projectModule)
    {
        projectModule->RegisterComponents(componentRegistry);
        projectModule->RegisterEntityClasses(classRegistry);
        AttachProjectModuleToCurrentWorld();
    }
}

void Game::AttachProjectModuleToCurrentWorld()
{
    NotifyProjectLoaded();
}

void Game::PrepareForProjectModuleUnload(bool preserveCurrentWorld)
{
    if (!projectModule || !registry)
    {
        return;
    }

    if (preserveCurrentWorld && mode == EngineMode::Edit && tileMap)
    {
        CaptureWorldSnapshot();
    }

    NotifyProjectStop();
    NotifyEngineSystemsStop();
    NotifyEngineSystemsUnloaded();
    NotifyProjectUnloaded();
    ResetWorldRuntime(false);
}

void Game::RestoreWorldAfterProjectModuleReload()
{
    if (mode == EngineMode::Edit && authoredSnapshot.has_value())
    {
        RestoreWorldSnapshot();
        return;
    }

    AttachProjectModuleToCurrentWorld();
}

bool Game::LoadLevel(const std::filesystem::path& levelFilePath)
{
    if (!renderer)
    {
        Logger::Err("Cannot load level before renderer is initialized");
        return false;
    }

    mode = EngineMode::Edit;
    authoredSnapshot.reset();
    projectConfig.startupLevelPath = levelFilePath;
    camera.x = 0.0f;
    camera.y = 0.0f;

    RebuildWorld();
    CaptureWorldSnapshot();

    return tileMap != nullptr;
}

bool Game::SaveEntityAsPrefab(int entityId, const std::string& prefabName)
{
    if (mode != EngineMode::Edit)
    {
        Logger::Err("Cannot save prefab while Play mode is running");
        return false;
    }

    const std::string prefabId = SanitizePrefabId(prefabName);
    if (prefabId.empty())
    {
        Logger::Err("Cannot save prefab: prefab name is empty");
        return false;
    }

    Entity entity = FindEntityById(registry, entityId);
    if (entity.GetId() < 0)
    {
        Logger::Err("Cannot save prefab: selected entity does not exist. Runtime ID: " +
            std::to_string(entityId));
        return false;
    }

    nlohmann::json entityJson = EntitySerializer::SerializeEntity(entity, &componentRegistry);
    entityJson["id"] = prefabId;

    std::error_code error;
    std::filesystem::create_directories(projectConfig.prefabDirectory, error);
    if (error)
    {
        Logger::Err("Cannot create prefab directory: " + error.message());
        return false;
    }

    const std::filesystem::path prefabFilePath = projectConfig.prefabDirectory / (prefabId + ".json");
    std::ofstream output(prefabFilePath);
    if (!output.is_open())
    {
        Logger::Err("Cannot write prefab file: " + prefabFilePath.string());
        return false;
    }

    output << entityJson.dump(4) << "\n";
    output.close();

    if (!output)
    {
        Logger::Err("Failed to finish writing prefab file: " + prefabFilePath.string());
        return false;
    }

    prefabRegistry.LoadFromDirectory(projectConfig.prefabDirectory);
    Logger::Log("Saved prefab: " + prefabFilePath.string());
    return true;
}

bool Game::ImportTextureAsset(const TextureAssetImportOptions& options)
{
    if (mode != EngineMode::Edit)
    {
        Logger::Err("Cannot import texture while Play mode is running");
        return false;
    }

    if (options.assetId.empty())
    {
        Logger::Err("Cannot import texture: asset id is empty");
        return false;
    }

    if (options.sourceFilePath.empty() || !std::filesystem::exists(options.sourceFilePath))
    {
        Logger::Err("Cannot import texture: source file does not exist");
        return false;
    }

    const std::set<std::string> allowedExtensions = {".bmp", ".jpg", ".jpeg", ".png", ".webp"};
    const std::string extension = ToLower(options.sourceFilePath.extension().string());
    if (!allowedExtensions.contains(extension))
    {
        Logger::Err("Cannot import texture: unsupported image format " + extension);
        return false;
    }

    std::error_code error;
    const std::filesystem::path imageDirectory = projectConfig.assetsRoot / "images";
    std::filesystem::create_directories(imageDirectory, error);
    if (error)
    {
        Logger::Err("Cannot create image directory: " + error.message());
        return false;
    }

    const std::filesystem::path destinationFilePath = imageDirectory / options.sourceFilePath.filename();
    if (std::filesystem::absolute(options.sourceFilePath) != std::filesystem::absolute(destinationFilePath))
    {
        std::filesystem::copy_file(
            options.sourceFilePath,
            destinationFilePath,
            std::filesystem::copy_options::overwrite_existing,
            error
        );

        if (error)
        {
            Logger::Err("Cannot copy texture into project: " + error.message());
            return false;
        }
    }

    nlohmann::json manifestJson;
    if (std::filesystem::exists(projectConfig.assetManifestPath))
    {
        std::ifstream input(projectConfig.assetManifestPath);
        if (input.is_open())
        {
            try
            {
                input >> manifestJson;
            }
            catch (const std::exception& exception)
            {
                Logger::Err("Cannot parse asset manifest: " + std::string(exception.what()));
                return false;
            }
        }
    }

    if (!manifestJson.contains("assets") || !manifestJson["assets"].is_array())
    {
        manifestJson["assets"] = nlohmann::json::array();
    }

    const std::filesystem::path manifestDirectory = projectConfig.assetManifestPath.parent_path();
    const std::filesystem::path relativeTexturePath = std::filesystem::relative(
        destinationFilePath,
        manifestDirectory,
        error
    );

    const std::string textureFile = error
        ? destinationFilePath.string()
        : relativeTexturePath.string();

    nlohmann::json assetJson = {
        {"type", "texture"},
        {"id", options.assetId},
        {"file", textureFile},
        {"sprite", BuildSpriteMetadataJson(options.sprite)}
    };

    bool updatedExistingAsset = false;
    for (auto& assetJsonEntry : manifestJson["assets"])
    {
        if (assetJsonEntry.is_object() &&
            assetJsonEntry.value("id", std::string("")) == options.assetId)
        {
            assetJsonEntry = assetJson;
            updatedExistingAsset = true;
            break;
        }
    }

    if (!updatedExistingAsset)
    {
        manifestJson["assets"].push_back(assetJson);
    }

    std::ofstream output(projectConfig.assetManifestPath);
    if (!output.is_open())
    {
        Logger::Err("Cannot write asset manifest: " + projectConfig.assetManifestPath.string());
        return false;
    }

    output << manifestJson.dump(2) << "\n";

    assetRegistry->AddTexture(
        renderer,
        options.assetId,
        destinationFilePath.string(),
        options.sprite
    );

    Logger::Log("Imported texture asset: " + options.assetId);
    return true;
}

void Game::Update()
{
    // If we are too fast, waste some time until we reach the MILLISECS_PER_FRAME
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME)
    {
        SDL_Delay(timeToWait);
    }

    // The difference in ticks since the last frame, converted to seconds
    double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

    // Store the "previous" frame time
    millisecsPreviousFrame = SDL_GetTicks();

    const double simulationDeltaTime = mode == EngineMode::Play
        ? deltaTime * static_cast<double>(playSpeed)
        : deltaTime;

    elapsedTime += static_cast<int>(simulationDeltaTime * 1000.0);

    // Update the registry to process the entities that are waiting to be created/deleted
    registry->Update();

    // Invoke all the systems that need to update
    if (mode == EngineMode::Play)
    {
        UpdatePlaySimulation(simulationDeltaTime);
        ProcessPendingLevelLoad();
    }
    else
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        float cameraSpeed = 500.0f * static_cast<float>(deltaTime);

        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
        {
            camera.y -= cameraSpeed;
        }

        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
        {
            camera.y += cameraSpeed;
        }

        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
        {
            camera.x -= cameraSpeed;
        }

        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
        {
            camera.x += cameraSpeed;
        }

        ClampCameraToTileMap(camera, tileMap.get());
    }
}

std::filesystem::path Game::ResolveLevelLoadPath(const std::filesystem::path& levelPath) const
{
    if (levelPath.is_absolute())
    {
        return levelPath;
    }

    if (levelPath.has_parent_path())
    {
        return projectConfig.projectRoot / levelPath;
    }

    return projectConfig.assetsRoot / "levels" / levelPath;
}

bool Game::ProcessPendingLevelLoad()
{
    std::optional<std::filesystem::path> requestedLevel = levelLoadRequests.ConsumePendingLoad();
    if (!requestedLevel.has_value())
    {
        return false;
    }

    const std::filesystem::path resolvedLevelPath = ResolveLevelLoadPath(*requestedLevel);
    if (!std::filesystem::exists(resolvedLevelPath))
    {
        Logger::Err("Cannot load requested level: file does not exist: " + resolvedLevelPath.string());
        return false;
    }

    if (mode != EngineMode::Play)
    {
        return LoadLevel(resolvedLevelPath);
    }

    Logger::Log("Loading gameplay level: " + resolvedLevelPath.string());

    NotifyProjectStop();
    NotifyEngineSystemsStop();

    const std::filesystem::path previousStartupLevelPath = projectConfig.startupLevelPath;
    projectConfig.startupLevelPath = resolvedLevelPath;

    camera.x = 0.0f;
    camera.y = 0.0f;
    elapsedTime = 0;

    RebuildWorld();

    projectConfig.startupLevelPath = previousStartupLevelPath;

    NotifyEngineSystemsStart();
    NotifyProjectStart();

    return tileMap != nullptr;
}

void Game::UpdatePlaySimulation(double deltaTime)
{
    EntitySystemContext entitySystemContext = CreateEntitySystemContext(deltaTime);
    const auto engineSystemEnabled = [this](const std::string& name)
    {
        return IsEngineSystemEnabled(projectConfig, name);
    };

    for (SimulationUpdatePhase phase : PlaySimulationUpdateOrder)
    {
        switch (phase)
        {
        case SimulationUpdatePhase::Input:
            if (engineSystemEnabled("input"))
            {
                registry->GetSystem<KeyboardControlSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::ProjectSimulation:
            if (projectModule && tileMap)
            {
                ProjectRuntimeContext context = CreateProjectRuntimeContext(deltaTime);
                projectModule->Update(context);
            }
            break;

        case SimulationUpdatePhase::ProjectEntitySystems:
            registry->UpdateAutomaticSystems(entitySystemContext);
            break;

        case SimulationUpdatePhase::Movement:
            if (engineSystemEnabled("movement"))
            {
                registry->GetSystem<MovementSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::TerrainConstraint:
            if (engineSystemEnabled("terrain_constraint"))
            {
                registry->GetSystem<TerrainConstraintSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::SoftCollision:
            if (engineSystemEnabled("soft_collision"))
            {
                registry->GetSystem<SoftCollisionSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::Collision:
            if (engineSystemEnabled("collision"))
            {
                registry->GetSystem<CollisionSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::ProjectileEmission:
            if (engineSystemEnabled("projectile_emission"))
            {
                registry->GetSystem<ProjectileEmitSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::ProjectileLifecycle:
            if (engineSystemEnabled("projectile_lifecycle"))
            {
                registry->GetSystem<ProjectileLifecycleSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::OffscreenLifecycle:
            if (engineSystemEnabled("offscreen_lifecycle"))
            {
                registry->GetSystem<DestroyWhenOffscreenSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::Animation:
            if (engineSystemEnabled("animation"))
            {
                registry->GetSystem<AnimationSystem>().Update(entitySystemContext);
            }
            break;

        case SimulationUpdatePhase::Camera:
            if (engineSystemEnabled("camera"))
            {
                registry->GetSystem<CameraMovementSystem>().Update(entitySystemContext);
            }
            break;
        }
    }
}

void Game::RenderSceneToViewport(int width, int height)
{
    EnsureViewportTexture(width, height);
    RenderSceneToViewportTexture();
}

void Game::RenderSceneToWindow(int width, int height)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        return;
    }

    camera.w = static_cast<float>(width);
    camera.h = static_cast<float>(height);

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    EntitySystemContext context = CreateEntitySystemContext();
    registry->GetSystem<SpriteRenderSystem>().RenderLayerPass(context, true);

    if (tileMap && tileMapRenderer)
    {
        tileMapRenderer->Render(
            renderer,
            assetRegistry->GetTexture(tileMap->GetTextureAssetId()),
            *tileMap,
            camera
        );
    }

    registry->GetSystem<SpriteRenderSystem>().Update(context);
    registry->GetSystem<RenderTextSystem>().Update(context);
    registry->GetSystem<RenderHealthBarSystem>().Update(context);

    if (projectModule)
    {
        projectModule->RenderProjectSimulation(renderer, *assetRegistry, camera);
    }

    RenderProjectHud(width, height);
}

void Game::RenderProjectHud(int width, int height)
{
    if (!projectModule || !renderer || !assetRegistry || width <= 0 || height <= 0)
    {
        return;
    }

    HudContext context(renderer, *assetRegistry, width, height);
    projectModule->RenderHud(context);
}

bool Game::RenderSceneToPixels(int width, int height, std::vector<std::uint32_t>& pixels)
{
    if (!renderer)
    {
        return false;
    }

    RenderSceneToViewport(width, height);
    if (!viewportTexture)
    {
        return false;
    }

    SDL_SetRenderTarget(renderer, viewportTexture);
    SDL_Surface* readbackSurface = SDL_RenderReadPixels(renderer, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);

    if (!readbackSurface)
    {
        Logger::Err("Failed to read viewport pixels: " + std::string(SDL_GetError()));
        return false;
    }

    SDL_Surface* argbSurface = SDL_ConvertSurface(readbackSurface, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(readbackSurface);

    if (!argbSurface)
    {
        Logger::Err("Failed to convert viewport pixels: " + std::string(SDL_GetError()));
        return false;
    }

    pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    const int sourceWidth = std::min(width, argbSurface->w);
    const int sourceHeight = std::min(height, argbSurface->h);
    const auto* sourcePixels = static_cast<const std::uint8_t*>(argbSurface->pixels);
    auto* destinationPixels = reinterpret_cast<std::uint8_t*>(pixels.data());
    const int destinationPitch = width * static_cast<int>(sizeof(std::uint32_t));
    const int copyBytes = sourceWidth * static_cast<int>(sizeof(std::uint32_t));

    for (int row = 0; row < sourceHeight; ++row)
    {
        std::memcpy(
            destinationPixels + row * destinationPitch,
            sourcePixels + row * argbSurface->pitch,
            copyBytes
        );
    }

    SDL_DestroySurface(argbSurface);
    return true;
}

SDL_Texture* Game::GetTilePaletteTexture() const
{
    if (tileMap)
    {
        return assetRegistry->GetTexture(tileMap->GetTextureAssetId());
    }

    return nullptr;
}

const std::unordered_map<std::string, FieldGrid>& Game::GetFieldGrids() const
{
    static const std::unordered_map<std::string, FieldGrid> emptyFieldGrids;

    if (projectModule)
    {
        return projectModule->GetFieldGrids();
    }

    return emptyFieldGrids;
}

void Game::Shutdown()
{
    if (viewportTexture)
    {
        SDL_DestroyTexture(viewportTexture);
        viewportTexture = nullptr;
    }
}

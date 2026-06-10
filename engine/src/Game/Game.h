#ifndef GAME_H
#define GAME_H

#include <memory>
#include <optional>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "Assets/AssetRegistry.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Game/EngineMode.h"
#include "Game/LevelFilePaths.h"
#include "Game/LevelLoadRequests.h"
#include "Map/TileMap.h"
#include "Map/TileMapRenderer.h"
#include "Prefabs/PrefabRegistry.h"
#include "Project/ProjectConfig.h"
#include "Reflection/EditorMetadata.h"
#include "Fields/FieldGrid.h"
#include "Simulation/ProjectModule.h"

const int FPS = 60;
const int MILLISECS_PER_FRAME = 1000 / FPS;

struct WorldSnapshot
{
    nlohmann::json entitiesJson;

    std::vector<TileCell> tiles;
    int rows = 0;
    int cols = 0;
    int tileSize = 0;
    float scale = 1.0f;

    std::string textureAssetId;
    LevelFilePaths levelFilePaths;
};

struct TextureAssetImportOptions
{
    std::string assetId;
    std::filesystem::path sourceFilePath;
    TextureSpriteMetadata sprite;
};


class Game
{
private:
    ProjectConfig projectConfig = CreateDefaultProjectConfig();

    int millisecsPreviousFrame = 0;
    int elapsedTime = 0;

    SDL_Renderer* renderer = nullptr;
    SDL_FRect camera;

    SDL_Texture* viewportTexture = nullptr;
    int viewportTextureWidth = 0;
    int viewportTextureHeight = 0;

    std::optional<WorldSnapshot> authoredSnapshot;

    EngineMode mode = EngineMode::Edit;
    float playSpeed = 1.0f;

    std::unique_ptr<Registry> registry;
    std::unique_ptr<AssetRegistry> assetRegistry;
    std::unique_ptr<EventBus> eventBus;

    std::unique_ptr<TileMap> tileMap;
    std::unique_ptr<TilemapRenderer> tileMapRenderer;

    PrefabRegistry prefabRegistry;
    std::shared_ptr<ProjectModule> projectModule;
    ComponentRegistry componentRegistry;
    ClassRegistry classRegistry;
    LevelFilePaths currentLevelFilePaths;
    LevelLoadRequests levelLoadRequests;

private:
    void EnsureViewportTexture(int width, int height);
    void RenderSceneToViewportTexture();
    void RenderProjectHud(int width, int height);

    void ResetWorldRuntime(bool registerProjectSystems = true);
    void RebuildWorld();
    void CaptureWorldSnapshot();
    void RestoreWorldSnapshot();

    void EnterPlayMode();
    void EnterEditMode();
    void UpdatePlaySimulation(double deltaTime);
    bool ProcessPendingLevelLoad();
    std::filesystem::path ResolveLevelLoadPath(const std::filesystem::path& levelPath) const;
    EntitySystemContext CreateEntitySystemContext(double deltaTime = 0.0);
    ProjectRuntimeContext CreateProjectRuntimeContext(double deltaTime = 0.0);
    void NotifyEngineSystemsLoaded();
    void NotifyEngineSystemsStart();
    void NotifyEngineSystemsStop();
    void NotifyEngineSystemsUnloaded();
    void NotifyProjectLoaded();
    void NotifyProjectStart();
    void NotifyProjectStop();
    void NotifyProjectUnloaded();

public:
    explicit Game(ProjectConfig projectConfig = CreateDefaultProjectConfig());
    ~Game();
    void Initialize(SDL_Renderer* renderer);
    void Setup();
    void HandleEvent(const SDL_Event& event, bool& shouldQuit);
    void Update();
    void RenderSceneToViewport(int width, int height);
    void RenderSceneToWindow(int width, int height);
    bool RenderSceneToPixels(int width, int height, std::vector<std::uint32_t>& pixels);
    void Shutdown();

    bool LoadProject(ProjectConfig newProjectConfig);
    bool LoadLevel(const std::filesystem::path& levelFilePath);
    bool SaveEntityAsPrefab(int entityId, const std::string& prefabName);
    bool ImportTextureAsset(const TextureAssetImportOptions& options);
    void SetProjectModule(std::shared_ptr<ProjectModule> module);
    void AttachProjectModuleToCurrentWorld();
    void PrepareForProjectModuleUnload(bool preserveCurrentWorld);
    void RestoreWorldAfterProjectModuleReload();
    void ToggleMode();
    void SetPlaySpeed(float speed);
    EngineMode GetMode() const { return mode; }
    float GetPlaySpeed() const { return playSpeed; }
    SDL_Texture* GetViewportTexture() const { return viewportTexture; }
    SDL_Texture* GetTilePaletteTexture() const;
    const std::unique_ptr<Registry>& GetRegistry() const { return registry; }
    std::unique_ptr<AssetRegistry>& GetAssetRegistry() { return assetRegistry; }
    const std::unordered_map<std::string, FieldGrid>& GetFieldGrids() const;
    const PrefabRegistry& GetPrefabRegistry() const { return prefabRegistry; }
    const ProjectConfig& GetProjectConfig() const { return projectConfig; }
    LevelFilePaths& GetCurrentLevelFilePaths() { return currentLevelFilePaths; }
    const LevelFilePaths& GetCurrentLevelFilePaths() const { return currentLevelFilePaths; }
    ProjectModule* GetProjectModule() const { return projectModule.get(); }
    const ComponentRegistry& GetComponentRegistry() const { return componentRegistry; }
    const ClassRegistry& GetClassRegistry() const { return classRegistry; }
    TileMap* GetTileMap() const { return tileMap.get(); }
    SDL_FRect& GetCamera() { return camera; }
};

#endif

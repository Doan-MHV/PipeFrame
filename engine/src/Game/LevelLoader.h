

#ifndef PIPEFRAME_LEVELLOADER_H
#define PIPEFRAME_LEVELLOADER_H

#include <memory>
#include <string>

#include <SDL3/SDL_render.h>
#include <nlohmann/json_fwd.hpp>

#include "Assets/AssetRegistry.h"
#include "ECS/ECS.h"
#include "ECS/Entity.h"
#include "Game/LevelFilePaths.h"
#include "Map/TileMap.h"

class ComponentRegistry;

class LevelLoader
{
public:
    LevelLoader();
    ~LevelLoader();

    void LoadAssetManifest(
        const std::unique_ptr<AssetRegistry>& assetRegistry,
        SDL_Renderer* renderer,
        const std::string& manifestFilePath
    );

    void LoadEntitiesFromJson(
        const nlohmann::json& entitiesJson,
        const std::unique_ptr<Registry>& registry
    );

    void LoadEntitiesFromJson(
        const nlohmann::json& entitiesJson,
        const std::unique_ptr<Registry>& registry,
        const ComponentRegistry* componentRegistry
    );

    Entity LoadEntityFromJson(
        const nlohmann::json& entityJson,
        const std::unique_ptr<Registry>& registry
    );

    Entity LoadEntityFromJson(
        const nlohmann::json& entityJson,
        const std::unique_ptr<Registry>& registry,
        const ComponentRegistry* componentRegistry
    );

    Entity LoadEntityFromJson(
        const nlohmann::json& entityJson,
        Registry& registry
    );

    Entity LoadEntityFromJson(
        const nlohmann::json& entityJson,
        Registry& registry,
        const ComponentRegistry* componentRegistry
    );

    void LoadLevel(
        const std::unique_ptr<Registry>& registry,
        const std::unique_ptr<AssetRegistry>& assetRegistry,
        SDL_Renderer* renderer,
        std::unique_ptr<TileMap>& tileMap,
        const std::string& levelFilePath,
        const ComponentRegistry* componentRegistry = nullptr,
        LevelFilePaths* outLevelFilePaths = nullptr
    );
};


#endif //PIPEFRAME_LEVELLOADER_H

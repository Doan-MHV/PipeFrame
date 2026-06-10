#include "LevelLoader.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "Components/AnimationComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/CameraFollowComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/HealthComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SoftCollisionComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Logger/Logger.h"
#include "Map/TileMapSerializer.h"
#include "Reflection/EditorMetadata.h"

static std::string ResolvePathRelativeTo(
    const std::filesystem::path& baseDirectory,
    const std::string& path
)
{
    if (path.empty())
    {
        return "";
    }

    std::filesystem::path resolvedPath(path);

    if (resolvedPath.is_relative())
    {
        resolvedPath = baseDirectory / resolvedPath;
    }

    return resolvedPath.string();
}

static bool LoadJsonFile(const std::string& filePath, nlohmann::json& outJson)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        Logger::Err("Failed to open json file: " + filePath);
        return false;
    }

    try
    {
        input >> outJson;
    }
    catch (const std::exception& exception)
    {
        Logger::Err("Failed to parse json file: " + filePath + " error: " + exception.what());
        return false;
    }

    return true;
}

static TextureSpriteMetadata ParseTextureSpriteMetadata(const nlohmann::json& assetJson)
{
    TextureSpriteMetadata metadata;

    if (!assetJson.contains("sprite") || !assetJson["sprite"].is_object())
    {
        return metadata;
    }

    const auto& spriteJson = assetJson["sprite"];
    const std::string mode = spriteJson.value("mode", std::string("image"));

    if (mode == "sheet" || mode == "spritesheet" || mode == "sprite_sheet")
    {
        metadata.mode = TextureSpriteMode::SpriteSheet;
    }
    else
    {
        metadata.mode = TextureSpriteMode::SingleImage;
    }

    if (spriteJson.contains("default_display_size") && spriteJson["default_display_size"].is_object())
    {
        const auto& displaySize = spriteJson["default_display_size"];
        metadata.defaultDisplayWidth = displaySize.value("w", metadata.defaultDisplayWidth);
        metadata.defaultDisplayHeight = displaySize.value("h", metadata.defaultDisplayHeight);
    }

    metadata.frameWidth = spriteJson.value("frame_width", metadata.frameWidth);
    metadata.frameHeight = spriteJson.value("frame_height", metadata.frameHeight);
    metadata.defaultFrame = spriteJson.value("default_frame", metadata.defaultFrame);

    if (spriteJson.contains("default_src_rect") && spriteJson["default_src_rect"].is_object())
    {
        const auto& sourceRect = spriteJson["default_src_rect"];
        metadata.frameWidth = sourceRect.value("w", metadata.frameWidth);
        metadata.frameHeight = sourceRect.value("h", metadata.frameHeight);
    }

    return metadata;
}


static TerrainType GetTerrainTypeFromTile(int row, int col)
{
    // Fully navigable water
    if (
        (row == 2 && col == 1) ||
        (row == 1 && col == 6) ||
        (row == 1 && col == 7) ||
        (row == 1 && col == 8) ||
        (row == 1 && col == 9)
    )
    {
        return TerrainType::Water;
    }

    // Shoreline / transition tiles
    if (
        (row == 1 && col == 1) ||
        (row == 1 && col == 3) ||
        (row == 2 && col == 2) ||
        (row == 0 && col == 9) ||
        (row == 1 && col == 0) ||
        (row == 1 && col == 2) ||
        (row == 1 && col == 4) ||
        (row == 1 && col == 5)
    )
    {
        return TerrainType::Blocked;
    }

    return TerrainType::Land;
}

static void ApplyFallbackTerrain(TileMap& tileMap)
{
    for (int row = 0; row < tileMap.GetRows(); row++)
    {
        for (int col = 0; col < tileMap.GetCols(); col++)
        {
            TileCell& tile = tileMap.GetTile(row, col);
            tile.terrain = GetTerrainTypeFromTile(tile.tilesetRow, tile.tilesetColumn);
        }
    }
}


LevelLoader::LevelLoader()
{
    Logger::Log("LevelLoader constructor called!");
}

LevelLoader::~LevelLoader()
{
    Logger::Log("LevelLoader destructor called!");
}

void LevelLoader::LoadAssetManifest(
    const std::unique_ptr<AssetRegistry>& assetRegistry,
    SDL_Renderer* renderer,
    const std::string& manifestFilePath
)
{
    nlohmann::json manifestJson;
    if (!LoadJsonFile(manifestFilePath, manifestJson))
    {
        return;
    }

    if (!manifestJson.contains("assets") || !manifestJson["assets"].is_array())
    {
        Logger::Err("Asset manifest missing 'assets' array: " + manifestFilePath);
        return;
    }

    for (const auto& asset : manifestJson["assets"])
    {
        const std::string assetType = asset.value("type", "");
        const std::string assetId = asset.value("id", "");
        const std::string assetFile = asset.value("file", "");
        const std::filesystem::path manifestDirectory = std::filesystem::path(manifestFilePath).parent_path();
        const std::string resolvedAssetFile = ResolvePathRelativeTo(manifestDirectory, assetFile);

        if (assetType.empty() || assetId.empty() || assetFile.empty())
        {
            Logger::Err("Skipping invalid asset entry in manifest: " + manifestFilePath);
            continue;
        }

        if (assetType == "texture")
        {
            assetRegistry->AddTexture(
                renderer,
                assetId,
                resolvedAssetFile,
                ParseTextureSpriteMetadata(asset)
            );
            Logger::Log("Loaded texture asset from manifest, id: " + assetId);
        }
        else if (assetType == "font")
        {
            const int fontSize = asset.value("font_size", 0);

            if (fontSize <= 0)
            {
                Logger::Err("Skipping font asset with invalid size, id: " + assetId);
                continue;
            }

            assetRegistry->AddFont(assetId, resolvedAssetFile, fontSize);
            Logger::Log("Loaded font asset from manifest, id: " + assetId);
        }
    }
}

Entity LevelLoader::LoadEntityFromJson(
    const nlohmann::json& entityJson,
    const std::unique_ptr<Registry>& registry
)
{
    return LoadEntityFromJson(entityJson, *registry, nullptr);
}

Entity LevelLoader::LoadEntityFromJson(
    const nlohmann::json& entityJson,
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry* componentRegistry
)
{
    return LoadEntityFromJson(entityJson, *registry, componentRegistry);
}

Entity LevelLoader::LoadEntityFromJson(
    const nlohmann::json& entityJson,
    Registry& registry
)
{
    return LoadEntityFromJson(entityJson, registry, nullptr);
}

Entity LevelLoader::LoadEntityFromJson(
    const nlohmann::json& entityJson,
    Registry& registry,
    const ComponentRegistry* componentRegistry
)
{
    Entity newEntity = registry.CreateEntity();
    newEntity.AddComponent<EditorEntityComponent>();

    if (entityJson.contains("id") && entityJson["id"].is_string())
    {
        newEntity.AddComponent<PersistentIdComponent>(entityJson["id"].get<std::string>());
    }

    if (entityJson.contains("tag") && entityJson["tag"].is_string())
    {
        newEntity.Tag(entityJson["tag"].get<std::string>());
    }

    if (entityJson.contains("group") && entityJson["group"].is_string())
    {
        newEntity.Group(entityJson["group"].get<std::string>());
    }

    if (!entityJson.contains("components") || !entityJson["components"].is_object())
    {
        return newEntity;
    }

    const nlohmann::json& components = entityJson["components"];

    if (components.contains("transform") && components["transform"].is_object())
    {
        const auto& transform = components["transform"];

        newEntity.AddComponent<TransformComponent>(
            glm::vec2(
                transform["position"].value("x", 0.0f),
                transform["position"].value("y", 0.0f)
            ),
            glm::vec2(
                transform["scale"].value("x", 1.0f),
                transform["scale"].value("y", 1.0f)
            ),
            transform.value("rotation", 0.0)
        );
    }

    if (components.contains("rigidbody") && components["rigidbody"].is_object())
    {
        const auto& rigidbody = components["rigidbody"];

        newEntity.AddComponent<RigidBodyComponent>(
            glm::vec2(
                rigidbody["velocity"].value("x", 0.0f),
                rigidbody["velocity"].value("y", 0.0f)
            )
        );
    }

    if (components.contains("sprite") && components["sprite"].is_object())
    {
        const auto& sprite = components["sprite"];

        newEntity.AddComponent<SpriteComponent>(
            sprite.value("texture_asset_id", std::string("")),
            sprite.value("width", 0),
            sprite.value("height", 0),
            sprite.value("z_index", 1),
            sprite.value("fixed", false),
            sprite.value("src_rect_x", 0.0f),
            sprite.value("src_rect_y", 0.0f),
            sprite.value("src_rect_w", sprite.value("src_rect_width", 0.0f)),
            sprite.value("src_rect_h", sprite.value("src_rect_height", 0.0f))
        );
    }

    if (components.contains("animation") && components["animation"].is_object())
    {
        const auto& animation = components["animation"];

        newEntity.AddComponent<AnimationComponent>(
            animation.value("num_frames", 1),
            animation.value("speed_rate", 1)
        );
    }

    if (components.contains("movement") && components["movement"].is_object())
    {
        const auto& movement = components["movement"];
        std::string movementType = movement.value("type", std::string("land"));

        newEntity.AddComponent<MovementTypeComponent>(ParseMovementType(movementType));
    }

    if (components.contains("boxcollider") && components["boxcollider"].is_object())
    {
        const auto& collider = components["boxcollider"];
        const auto offset = collider.value("offset", nlohmann::json::object());

        newEntity.AddComponent<BoxColliderComponent>(
            collider.value("width", 0),
            collider.value("height", 0),
            glm::vec2(
                offset.value("x", 0.0f),
                offset.value("y", 0.0f)
            ),
            collider.value("match_sprite_size", false),
            collider.value("rotate_with_transform", false)
        );
    }

    if (components.contains("soft_collision") && components["soft_collision"].is_object())
    {
        const auto& softCollision = components["soft_collision"];

        newEntity.AddComponent<SoftCollisionComponent>(
            softCollision.value("radius", 16.0f),
            softCollision.value("push_strength", 0.65f),
            softCollision.value("immovable", false)
        );
    }

    if (components.contains("health") && components["health"].is_object())
    {
        const auto& health = components["health"];

        newEntity.AddComponent<HealthComponent>(
            health.value("health_percentage", 100)
        );
    }

    if (components.contains("attributes") && components["attributes"].is_object())
    {
        newEntity.AddComponent<AttributesComponent>(components["attributes"]);
    }

    if (components.contains("projectile_emitter") && components["projectile_emitter"].is_object())
    {
        const auto& projectileEmitter = components["projectile_emitter"];

        newEntity.AddComponent<ProjectileEmitterComponent>(
            glm::vec2(
                projectileEmitter["projectile_velocity"].value("x", 0.0f),
                projectileEmitter["projectile_velocity"].value("y", 0.0f)
            ),
            projectileEmitter.value("repeat_frequency", 1) * 1000,
            projectileEmitter.value("projectile_duration", 10) * 1000,
            projectileEmitter.value("hit_percentage_damage", 10),
            projectileEmitter.value("friendly", false)
        );
    }

    if (components.contains("camera_follow") && components["camera_follow"].is_object())
    {
        newEntity.AddComponent<CameraFollowComponent>();
    }

    if (components.contains("keyboard_controller") && components["keyboard_controller"].is_object())
    {
        const auto& keyboardControlled = components["keyboard_controller"];

        newEntity.AddComponent<KeyboardControlledComponent>(
            glm::vec2(
                keyboardControlled["up_velocity"].value("x", 0.0f),
                keyboardControlled["up_velocity"].value("y", 0.0f)
            ),
            glm::vec2(
                keyboardControlled["right_velocity"].value("x", 0.0f),
                keyboardControlled["right_velocity"].value("y", 0.0f)
            ),
            glm::vec2(
                keyboardControlled["down_velocity"].value("x", 0.0f),
                keyboardControlled["down_velocity"].value("y", 0.0f)
            ),
            glm::vec2(
                keyboardControlled["left_velocity"].value("x", 0.0f),
                keyboardControlled["left_velocity"].value("y", 0.0f)
            )
        );
    }

    if (componentRegistry)
    {
        ApplyRegisteredComponents(newEntity, components, *componentRegistry);
    }

    return newEntity;
}

static bool LoadEntitiesFromJsonFile(
    const std::string& filePath,
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry* componentRegistry
)
{
    nlohmann::json entitiesJson;
    if (!LoadJsonFile(filePath, entitiesJson))
    {
        return false;
    }

    if (!entitiesJson.contains("entities") || !entitiesJson["entities"].is_array())
    {
        Logger::Err("Entities file missing 'entities' array: " + filePath);
        return false;
    }

    LevelLoader loader;

    for (const auto& entityJson : entitiesJson["entities"])
    {
        if (!entityJson.is_object())
        {
            continue;
        }

        loader.LoadEntityFromJson(entityJson, registry, componentRegistry);
    }

    Logger::Log("Loaded entities from " + filePath);
    return true;
}

void LevelLoader::LoadEntitiesFromJson(
    const nlohmann::json& entitiesJson,
    const std::unique_ptr<Registry>& registry
)
{
    LoadEntitiesFromJson(entitiesJson, registry, nullptr);
}

void LevelLoader::LoadEntitiesFromJson(
    const nlohmann::json& entitiesJson,
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry* componentRegistry
)
{
    if (!entitiesJson.contains("entities") || !entitiesJson["entities"].is_array())
    {
        Logger::Err("In-memory entities json missing 'entities' array");
        return;
    }

    for (const auto& entityJson : entitiesJson["entities"])
    {
        if (!entityJson.is_object())
        {
            continue;
        }

        LoadEntityFromJson(entityJson, registry, componentRegistry);
    }

    Logger::Log("Loaded entities from in-memory snapshot");
}


void LevelLoader::LoadLevel(
    const std::unique_ptr<Registry>& registry,
    const std::unique_ptr<AssetRegistry>& assetRegistry,
    SDL_Renderer* renderer,
    std::unique_ptr<TileMap>& tileMap,
    const std::string& levelFilePath,
    const ComponentRegistry* componentRegistry,
    LevelFilePaths* outLevelFilePaths
)
{
    nlohmann::json levelJson;

    if (!LoadJsonFile(levelFilePath, levelJson))
    {
        return;
    }

    std::string tileMapFilePath = levelJson.value("tilemap_file", "");
    std::string terrainFilePath = levelJson.value("terrain_file", "");
    std::string entitiesFilePath = levelJson.value("entities_file", "");
    const std::filesystem::path levelDirectory = std::filesystem::path(levelFilePath).parent_path();

    tileMapFilePath = ResolvePathRelativeTo(levelDirectory, tileMapFilePath);
    terrainFilePath = ResolvePathRelativeTo(levelDirectory, terrainFilePath);
    entitiesFilePath = ResolvePathRelativeTo(levelDirectory, entitiesFilePath);

    bool usesLegacyTileMap = false;

    if (tileMapFilePath.empty())
    {
        if (!levelJson.contains("tilemap") || !levelJson["tilemap"].is_object())
        {
            Logger::Err("Level file missing tilemap_file: " + levelFilePath);
            return;
        }

        usesLegacyTileMap = true;
        const nlohmann::json& map = levelJson["tilemap"];
        tileMapFilePath = ResolvePathRelativeTo(levelDirectory, map.value("map_file", std::string()));
    }

    if (outLevelFilePaths)
    {
        outLevelFilePaths->levelPath = levelFilePath;
        outLevelFilePaths->tileMapPath = tileMapFilePath;
        outLevelFilePaths->entitiesPath = entitiesFilePath;
    }

    if (tileMapFilePath.empty())
    {
        Logger::Err("Level file has invalid tilemap path: " + levelFilePath);
        return;
    }

    if (!usesLegacyTileMap)
    {
        if (!TileMapSerializer::LoadTileMap(tileMapFilePath, tileMap))
        {
            Logger::Err("Failed to load tilemap json: " + tileMapFilePath);
            return;
        }

        Logger::Log("Loaded tilemap json from " + tileMapFilePath);
    }
    else
    {
        const nlohmann::json& map = levelJson["tilemap"];
        std::string mapTextureAssetId = map.value("texture_asset_id", "");
        int mapNumRows = map.value("num_rows", 0);
        int mapNumCols = map.value("num_cols", 0);
        int tileSize = map.value("tile_size", 0);
        float mapScale = map.value("scale", 1.0f);

        if (mapTextureAssetId.empty() || mapNumRows <= 0 || mapNumCols <= 0 || tileSize <= 0)
        {
            Logger::Err("Level file has invalid legacy tilemap config: " + levelFilePath);
            return;
        }

        tileMap = std::make_unique<TileMap>(mapNumRows, mapNumCols, tileSize, mapScale);
        tileMap->SetTextureAssetId(mapTextureAssetId);

        std::fstream mapFile;
        mapFile.open(tileMapFilePath);

        if (!mapFile.is_open())
        {
            Logger::Err("Failed to open legacy visual tile map: " + tileMapFilePath);
            return;
        }

        for (int y = 0; y < mapNumRows; y++)
        {
            for (int x = 0; x < mapNumCols; x++)
            {
                char ch;

                mapFile.get(ch);
                int tilesetRow = ch - '0';

                mapFile.get(ch);
                int tilesetColumn = ch - '0';

                mapFile.ignore();

                TileCell& tile = tileMap->GetTile(y, x);
                tile.tilesetRow = tilesetRow;
                tile.tilesetColumn = tilesetColumn;
            }
        }

        mapFile.close();

        std::ifstream terrainFile(terrainFilePath);

        if (terrainFile.is_open())
        {
            bool terrainLoadOk = true;

            for (int row = 0; row < tileMap->GetRows() && terrainLoadOk; row++)
            {
                for (int col = 0; col < tileMap->GetCols(); col++)
                {
                    char ch;
                    terrainFile.get(ch);

                    if (!terrainFile.good())
                    {
                        terrainLoadOk = false;
                        break;
                    }

                    int terrainValue = ch - '0';

                    if (terrainValue < static_cast<int>(TerrainType::Land) ||
                        terrainValue > static_cast<int>(TerrainType::Blocked))
                    {
                        terrainLoadOk = false;
                        break;
                    }

                    tileMap->GetTile(row, col).terrain = static_cast<TerrainType>(terrainValue);

                    if (col < tileMap->GetCols() - 1)
                    {
                        terrainFile.ignore(); // skip comma
                    }
                }

                if (row < tileMap->GetRows() - 1)
                {
                    terrainFile.ignore(); // skip newline
                }
            }

            terrainFile.close();

            if (terrainLoadOk)
            {
                Logger::Log("Loaded legacy terrain map from " + terrainFilePath);
            }
            else
            {
                Logger::Err("Terrain file format error in " + terrainFilePath + ". Using inferred terrain defaults.");
                ApplyFallbackTerrain(*tileMap);
            }
        }
        else
        {
            Logger::Log("No legacy terrain file found. Using inferred terrain defaults.");
            ApplyFallbackTerrain(*tileMap);
        }
    }

    if (entitiesFilePath.empty())
    {
        Logger::Err("Level file missing entities_file: " + levelFilePath);
        return;
    }

    if (!LoadEntitiesFromJsonFile(entitiesFilePath, registry, componentRegistry))
    {
        Logger::Err("Failed to load entities from " + entitiesFilePath);
        return;
    }
}

#include "World/AntWorld.h"
#include "Components/ColonySettingsComponent.h"
#include "Runtime/AntTypeIds.h"
#include "World/Runtime/Environment/WorldMapLoader.h"
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Environment/TilemapAssetModule.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <algorithm>
namespace ant_simulation {
std::optional<AntWorldBuildResult> AntWorld::FromScene(
    std::span<const pipeframe::SceneObjectData> authoredObjects,
    pipeframe::TilemapAssetModule &tilemapAssets,
    const pipeframe::EntityRegistry &entityRegistry,
    const pipeframe::ComponentRegistry &componentRegistry,
    const std::function<void(pipeframe::EntityRegistry &, AntWorld &)> &bindFactories,
    std::string *errorMessage) {
    AntConfiguration rebuiltConfiguration;

    std::uint32_t randomSeed{1};

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != ColonyTypeId) {
            continue;
        }

        ColonySettingsComponent settings;
        std::string validationError;
        pipeframe::PropertyMap values;
        const auto component =
            std::ranges::find(object.components, std::string(ColonyTypeId), &pipeframe::SceneComponentData::typeId);
        if (component != object.components.end()) {
            for (const auto &[key, value] : component->properties)
                values.try_emplace(key, value);
        }
        if (!ColonySettingsComponent::Schema().Apply(settings, values, validationError)) {
            if (errorMessage)
                *errorMessage = validationError;
            return std::nullopt;
        }
        rebuiltConfiguration.colonyInitialAntCount = static_cast<std::uint32_t>(settings.population);
        rebuiltConfiguration.colonyRadius = static_cast<float>(settings.radius);
        rebuiltConfiguration.antSpeed = static_cast<float>(settings.speed);
        randomSeed = static_cast<std::uint32_t>(settings.seed);

        break;
    }

    pipeframe::AssetReference nextMap;
    std::uint64_t nextMapRevision{};
    std::optional<pipeframe::PlaygroundComponent> playground;
    pipeframe::AssetReference playgroundMap;
    for (const auto &object : authoredObjects)
        if (object.typeId == pipeframe::PlaygroundEntityTypeId) {
            if (playground) {
                if (errorMessage)
                    *errorMessage = "Ant supports one Playground per scene.";
                return std::nullopt;
            }
            if (object.transform.position != pipeframe::Vector2f{} || object.transform.rotation != 0 ||
                object.transform.scale != pipeframe::Vector2f{1, 1}) {
                if (errorMessage)
                    *errorMessage = "Ant Playground must remain at origin with unit scale and zero rotation.";
                return std::nullopt;
            }
            playground.emplace();
            pipeframe::TilemapComponent tiles;
            std::string error;
            for (const auto &component : object.components) {
                if (component.typeId == pipeframe::PlaygroundComponentTypeId &&
                    !pipeframe::PlaygroundComponent::Schema().Apply(*playground, component.properties, error)) {
                    if (errorMessage)
                        *errorMessage = error;
                    return std::nullopt;
                }
                if (component.typeId == pipeframe::TilemapComponentTypeId &&
                    !pipeframe::TilemapComponent::Schema().Apply(tiles, component.properties, error)) {
                    if (errorMessage)
                        *errorMessage = error;
                    return std::nullopt;
                }
            }
            if (playground->cellSize != 1 || playground->columns <= 4 || playground->rows <= 4) {
                if (errorMessage)
                    *errorMessage =
                        "Ant Playground requires unit cells and dimensions larger than its two-cell border.";
                return std::nullopt;
            }
            playgroundMap = tiles.asset;
        }
    std::filesystem::path mapPath;
    std::optional<pipeframe::Tilemap2D> mapData;

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != SimulationSettingsTypeId) {
            continue;
        }

        rebuiltConfiguration.colonyPosition = object.transform.position;

        rebuiltConfiguration.antUpdateWorkerCount = static_cast<std::uint32_t>(
            std::clamp(pipeframe::ProjectRuntime::ReadInteger(object, UpdateWorkerCountKey, 1), std::int64_t{1}, std::int64_t{64}));

        if (playground)
            break;
        pipeframe::AssetReference mapAsset;
        const pipeframe::PropertyMap *properties = &object.properties;
        if (!properties->contains(MapAssetKey)) {
            const auto component = std::ranges::find(object.components, std::string(SimulationSettingsTypeId),
                                                     &pipeframe::SceneComponentData::typeId);
            if (component != object.components.end())
                properties = &component->properties;
        }
        if (const auto field = properties->find(MapAssetKey); field != properties->end()) {
            if (const auto *reference = std::get_if<pipeframe::AssetReference>(&field->second))
                mapAsset = *reference;
        }
        std::string imageError;
        if (!mapAsset.assetId.empty()) {
            const auto *resource = tilemapAssets.Resolve(mapAsset);
            if (resource && resource->map)
                mapData = *resource->map;
            else
                imageError = resource ? resource->error : "Missing environment map asset";
        } else {
            break; // No environment asset: start with an empty world.
        }
        if (!mapData) {

            if (errorMessage)
                *errorMessage = "Unable to load Ant environment: " + imageError;
            return std::nullopt;
        }

        const pipeframe::Vector2u imageSize{unsigned(mapData->Columns()), unsigned(mapData->Rows())};

        rebuiltConfiguration.worldSize = {
            static_cast<int>(imageSize.x),
            static_cast<int>(imageSize.y),
        };

        break;
    }

    if (playground) {
        std::string mapError;
        if (playgroundMap.assetId.empty()) {
            mapData.emplace(int(playground->columns), int(playground->rows));
            mapData->AddLayer("Terrain");
        } else {
            const auto *resource = tilemapAssets.Resolve(playgroundMap);
            if (!resource || !resource->map) {
                if (errorMessage)
                    *errorMessage = resource ? resource->error : "Missing Playground tilemap";
                return std::nullopt;
            }
            mapData = *resource->map;
            nextMap = playgroundMap;
            nextMapRevision = resource->revision;
        }
        if (mapData->Columns() != playground->columns || mapData->Rows() != playground->rows) {
            if (errorMessage)
                *errorMessage = "Ant Playground dimensions must match its tilemap.";
            return std::nullopt;
        }
        rebuiltConfiguration.worldSize = {mapData->Columns(), mapData->Rows()};
        rebuiltConfiguration.colonyPosition = {mapData->Columns() * .5f, mapData->Rows() * .5f};
    }

    std::unique_ptr<AntWorld> rebuiltWorld = std::make_unique<AntWorld>(rebuiltConfiguration, randomSeed);

    std::string localError;

    if (!rebuiltWorld->Initialize(localError)) {

        if (errorMessage != nullptr) {
            *errorMessage = std::move(localError);
        }

        return std::nullopt;
    }

    if (mapData.has_value()) {
        WorldMapLoadResult mapResult;

        if (!WorldMapLoader::LoadMap(rebuiltWorld->GetEnvironment(), *mapData, mapPath, mapResult, localError)) {

            if (errorMessage != nullptr) {
                *errorMessage = std::move(localError);
            }

            return std::nullopt;
        }

        rebuiltConfiguration = rebuiltWorld->GetEnvironment().GetConfiguration();
    }

    auto creationTypes = entityRegistry;
    bindFactories(creationTypes, *rebuiltWorld);

    for (const pipeframe::SceneObjectData &object : authoredObjects) {
        if (object.typeId != FoodSourceTypeId) {
            continue;
        }

        const float radius = std::clamp(static_cast<float>(pipeframe::ProjectRuntime::ReadNumber(object, FoodRadiusKey, 8.0)), 0.5f, 128.0f);

        const std::int64_t quantity =
            std::clamp(pipeframe::ProjectRuntime::ReadInteger(object, FoodAmountKey, 7), std::int64_t{0}, std::int64_t{1'000'000});

        if (quantity == 0) {
            continue;
        }

        (void)rebuiltWorld->GetEnvironment().AddFoodPatch(object.transform.position, radius,
                                                          static_cast<std::size_t>(quantity));
    }

    pipeframe::RegisteredEntityObjects rebuiltEntities;
    try {
        rebuiltEntities.Rebuild(rebuiltWorld->GetScene(), authoredObjects, creationTypes, componentRegistry);
    } catch (const std::exception &error) {
        if (errorMessage)
            *errorMessage = error.what();
        return std::nullopt;
    }
    if(errorMessage)errorMessage->clear();
    return AntWorldBuildResult{std::move(rebuiltWorld), rebuiltConfiguration,
        std::move(rebuiltEntities),nextMap,nextMapRevision,playground.has_value()};
}
}

#include "World/Runtime/Environment/WorldMapLoader.h"

#include <PipeFrame/Environment/TilemapSerializer.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>

#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/WallBuilder.h"

namespace ant_simulation {

namespace {

pipeframe::Vector2f GetCellCenter(const unsigned int x, const unsigned int y) {
    return {
        static_cast<float>(x) + 0.5f,
        static_cast<float>(y) + 0.5f,
    };
}

void ResetResult(WorldMapLoadResult &result, const pipeframe::Vector2u mapSize = {0, 0}) {
    result = {};
    result.mapSize = mapSize;
}

} // namespace

bool WorldMapLoader::LoadMapFromFile(AntEnvironment &environment, const std::filesystem::path &filePath,
                                     WorldMapLoadResult &result, std::string &errorMessage) {
    const auto map = ReadMap(filePath, errorMessage);
    if (!map) {
        ResetResult(result);
        return false;
    }
    return LoadMap(environment, *map, filePath, result, errorMessage);
}

std::optional<pipeframe::Tilemap2D> WorldMapLoader::ReadMap(const std::filesystem::path &path, std::string &error) {
    if (path.extension() != ".pftilemap") {
        error = "Ant environments use .pftilemap assets. Create or edit a map in Assets / Maps.";
        return std::nullopt;
    }
    std::ifstream input(path);
    return pipeframe::TilemapSerializer::Load(input, error);
}

bool WorldMapLoader::LoadMap(AntEnvironment &environment, const pipeframe::Tilemap2D &map,
                             const std::filesystem::path &sourcePath, WorldMapLoadResult &result,
                             std::string &errorMessage) {
    errorMessage.clear();
    const pipeframe::Vector2u mapSize{unsigned(map.Columns()), unsigned(map.Rows())};
    ResetResult(result, mapSize);
    if (map.CellSize() != 1 || map.Origin() != pipeframe::Vector2f{}) {
        errorMessage = "Ant currently requires unit tile cells and origin (0,0).";
        return false;
    }
    if (mapSize.x <= static_cast<unsigned int>(AntEnvironment::BorderMargin * 2) ||
        mapSize.y <= static_cast<unsigned int>(AntEnvironment::BorderMargin * 2)) {
        errorMessage = "Tilemap is too small to contain the "
                       "two-cell Ant physics border.";

        return false;
    }

    if (const auto *density = map.DataLayer("ant.food-density")) {
        for (int y = 0; y < map.Rows(); ++y)
            for (int x = 0; x < map.Columns(); ++x) {
                const double value = density->cells.At({x, y});
                if (value < 0 || value > 10000 || std::floor(value) != value) {
                    errorMessage = "Ant food density must be an integer from 0 to 10000.";
                    return false;
                }
            }
    }
    AntConfiguration configuration = environment.GetConfiguration();

    configuration.worldSize = {
        static_cast<int>(mapSize.x),
        static_cast<int>(mapSize.y),
    };

    configuration.mapFilename = sourcePath.string();

    if (configuration.colonyPosition.x < static_cast<float>(AntEnvironment::BorderMargin) ||
        configuration.colonyPosition.y < static_cast<float>(AntEnvironment::BorderMargin) ||
        configuration.colonyPosition.x >=
            static_cast<float>(configuration.worldSize.x - AntEnvironment::BorderMargin) ||
        configuration.colonyPosition.y >=
            static_cast<float>(configuration.worldSize.y - AntEnvironment::BorderMargin)) {
        configuration.colonyPosition = {
            static_cast<float>(configuration.worldSize.x) * 0.5f,
            static_cast<float>(configuration.worldSize.y) * 0.5f,
        };
    }

    if (!environment.Initialize(configuration, errorMessage)) {
        return false;
    }

    for (unsigned int y = 0; y < mapSize.y; ++y) {
        for (unsigned int x = 0; x < mapSize.x; ++x) {
            const pipeframe::GridCoordinate coordinate{int(x), int(y)};

            const pipeframe::Vector2f position = GetCellCenter(x, y);

            if (!environment.IsSimulationPositionValid(position)) {
                continue;
            }

            AntWorldCell *cell = environment.TryGetCellAtWorldPosition(position);

            if (cell == nullptr) {
                continue;
            }

            if (map.IsSolid(coordinate)) {
                if (!cell->wall) {
                    cell->wall = true;
                    ++result.addedWallCells;
                }

                continue;
            }

            const auto *density = map.DataLayer("ant.food-density");
            const auto quantity = density ? static_cast<std::size_t>(density->cells.At(coordinate)) : 0;
            if (quantity > 0) {
                const WorldEntityId foodId = environment.AddFood(position, quantity);

                if (foodId != InvalidWorldEntityId) {
                    ++result.addedFoodCells;

                    result.addedFoodQuantity += quantity;
                }
            }
        }
    }

    WallBuilder::RebuildSamplingCoefficients(environment);

    return true;
}

} // namespace ant_simulation

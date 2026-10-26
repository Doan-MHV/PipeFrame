#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>
#include <cstddef>
#include <filesystem>
#include <string>

namespace ant_simulation {
class AntEnvironment;
struct WorldMapLoadResult {
    pipeframe::Vector2u mapSize{};
    std::size_t addedWallCells{}, addedFoodCells{}, addedFoodQuantity{};
};
// Collision-enabled solid tiles define walls; ant.food-density defines food amounts.
class WorldMapLoader {
  public:
    static std::optional<pipeframe::Tilemap2D> ReadMap(const std::filesystem::path &path, std::string &error);
    static bool LoadMap(AntEnvironment &environment, const pipeframe::Tilemap2D &map,
                        const std::filesystem::path &sourcePath, WorldMapLoadResult &result, std::string &error);
    static bool LoadMapFromFile(AntEnvironment &environment, const std::filesystem::path &path,
                                WorldMapLoadResult &result, std::string &error);
};
} // namespace ant_simulation

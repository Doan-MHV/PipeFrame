#include "TileMapSerializer.h"
#include <fstream>
#include <memory>

#include <nlohmann/json.hpp>

namespace
{
std::string TerrainToString(TerrainType terrain)
{
    switch (terrain)
    {
        case TerrainType::Land:
            return "Land";
        case TerrainType::Water:
            return "Water";
        case TerrainType::Runway:
            return "Runway";
        case TerrainType::Blocked:
            return "Blocked";
    }

    return "Land";
}

TerrainType TerrainFromString(const std::string& terrain)
{
    if (terrain == "Water")
    {
        return TerrainType::Water;
    }

    if (terrain == "Runway")
    {
        return TerrainType::Runway;
    }

    if (terrain == "Blocked")
    {
        return TerrainType::Blocked;
    }

    return TerrainType::Land;
}
}

bool TileMapSerializer::LoadTileMap(const std::string& filePath, std::unique_ptr<TileMap>& tileMap)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        return false;
    }

    nlohmann::json tileMapJson;
    try
    {
        file >> tileMapJson;
    }
    catch (const std::exception&)
    {
        return false;
    }

    const int rows = tileMapJson.value("rows", 0);
    const int cols = tileMapJson.value("cols", 0);
    const int tileSize = tileMapJson.value("tile_size", 0);
    const float scale = tileMapJson.value("scale", 1.0f);
    const std::string textureAssetId = tileMapJson.value("texture_asset_id", std::string());

    if (rows <= 0 || cols <= 0 || tileSize <= 0 || textureAssetId.empty() ||
        !tileMapJson.contains("tiles") || !tileMapJson["tiles"].is_array())
    {
        return false;
    }

    auto loadedTileMap = std::make_unique<TileMap>(rows, cols, tileSize, scale);
    loadedTileMap->SetTextureAssetId(textureAssetId);

    const auto& tileRows = tileMapJson["tiles"];
    if (static_cast<int>(tileRows.size()) != rows)
    {
        return false;
    }

    for (int row = 0; row < rows; row++)
    {
        if (!tileRows[row].is_array() || static_cast<int>(tileRows[row].size()) != cols)
        {
            return false;
        }

        for (int col = 0; col < cols; col++)
        {
            const auto& tileJson = tileRows[row][col];
            if (!tileJson.is_object())
            {
                return false;
            }

            TileCell& tile = loadedTileMap->GetTile(row, col);

            if (tileJson.contains("tile") && tileJson["tile"].is_object())
            {
                const auto& visualTile = tileJson["tile"];
                tile.tilesetRow = visualTile.value("row", 0);
                tile.tilesetColumn = visualTile.value("col", 0);
            }
            else
            {
                tile.tilesetRow = tileJson.value("tileset_row", 0);
                tile.tilesetColumn = tileJson.value("tileset_col", 0);
            }

            tile.terrain = TerrainFromString(tileJson.value("terrain", std::string("Land")));
        }
    }

    tileMap = std::move(loadedTileMap);
    return true;
}

bool TileMapSerializer::SaveTileMap(const TileMap& tileMap, const std::string& filePath)
{
    std::ofstream file(filePath);

    if (!file.is_open())
    {
        return false;
    }

    nlohmann::json tileMapJson;
    tileMapJson["version"] = 1;
    tileMapJson["texture_asset_id"] = tileMap.GetTextureAssetId();
    tileMapJson["rows"] = tileMap.GetRows();
    tileMapJson["cols"] = tileMap.GetCols();
    tileMapJson["tile_size"] = tileMap.GetTileSize();
    tileMapJson["scale"] = tileMap.GetScale();
    tileMapJson["tiles"] = nlohmann::json::array();

    for (int row = 0; row < tileMap.GetRows(); row++)
    {
        nlohmann::json tileRow = nlohmann::json::array();

        for (int col = 0; col < tileMap.GetCols(); col++)
        {
            const TileCell& tile = tileMap.GetTile(row, col);
            tileRow.push_back({
                {"tile", {
                    {"row", tile.tilesetRow},
                    {"col", tile.tilesetColumn}
                }},
                {"terrain", TerrainToString(tile.terrain)}
            });
        }

        tileMapJson["tiles"].push_back(tileRow);
    }

    file << tileMapJson.dump(2) << "\n";
    return true;
}

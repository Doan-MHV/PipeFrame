#include "TileMapSerializer.h"
#include <fstream>
#include <sstream>

bool TileMapSerializer::SaveVisualMap(const TileMap& tileMap, const std::string& filePath)
{
    std::ofstream file(filePath);

    if (!file.is_open())
    {
        return false;
    }

    for (int row = 0; row < tileMap.GetRows(); row++)
    {
        for (int col = 0; col < tileMap.GetCols(); col++)
        {
            const TileCell& tile = tileMap.GetTile(row, col);

            file << tile.tilesetRow << tile.tilesetColumn;

            if (col < tileMap.GetCols() - 1)
            {
                file << ",";
            }
        }

        if (row < tileMap.GetRows() - 1)
        {
            file << "\n";
        }
    }

    return true;
}

bool TileMapSerializer::SaveTerrainMap(const TileMap& tileMap, const std::string& filePath)
{
    std::ofstream file(filePath);

    if (!file.is_open())
    {
        return false;
    }

    for (int row = 0; row < tileMap.GetRows(); row++)
    {
        for (int col = 0; col < tileMap.GetCols(); col++)
        {
            const TileCell& tile = tileMap.GetTile(row, col);
            file << static_cast<int>(tile.terrain);

            if (col < tileMap.GetCols() - 1)
            {
                file << ",";
            }
        }

        if (row < tileMap.GetRows() - 1)
        {
            file << "\n";
        }
    }

    return true;
}

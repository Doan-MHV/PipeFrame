#include "TileMap.h"

#include <algorithm>
#include <stdexcept>

TileMap::TileMap(int rows, int cols, int tileSize, float scale)
    : rows(rows), cols(cols), tileSize(tileSize), scale(scale), tiles(rows * cols)
{
}

const std::string& TileMap::GetTextureAssetId() const
{
    return textureAssetId;
}

void TileMap::SetTextureAssetId(const std::string& textureAssetId)
{
    this->textureAssetId = textureAssetId;
}

void TileMap::Resize(int newRows, int newCols)
{
    if (newRows <= 0 || newCols <= 0)
    {
        throw std::invalid_argument("TileMap::Resize requires positive rows and columns");
    }

    std::vector<TileCell> resizedTiles(static_cast<std::size_t>(newRows * newCols));

    const int rowsToCopy = std::min(rows, newRows);
    const int colsToCopy = std::min(cols, newCols);

    for (int row = 0; row < rowsToCopy; row++)
    {
        for (int col = 0; col < colsToCopy; col++)
        {
            resizedTiles[static_cast<std::size_t>(row * newCols + col)] =
                tiles[static_cast<std::size_t>(row * cols + col)];
        }
    }

    rows = newRows;
    cols = newCols;
    tiles = std::move(resizedTiles);
}

void TileMap::SetTileSize(int newTileSize)
{
    if (newTileSize <= 0)
    {
        throw std::invalid_argument("TileMap::SetTileSize requires a positive tile size");
    }

    tileSize = newTileSize;
}

void TileMap::SetScale(float newScale)
{
    if (newScale <= 0.0f)
    {
        throw std::invalid_argument("TileMap::SetScale requires a positive scale");
    }

    scale = newScale;
}

int TileMap::GetRows() const
{
    return rows;
}

int TileMap::GetCols() const
{
    return cols;
}

int TileMap::GetTileSize() const
{
    return tileSize;
}

int TileMap::GetWorldWidth() const
{
    return static_cast<int>(cols * tileSize * scale);
}

int TileMap::GetWorldHeight() const
{
    return static_cast<int>(rows * tileSize * scale);
}

float TileMap::GetScale() const
{
    return scale;
}

bool TileMap::IsInBounds(int row, int col) const
{
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

TileCell& TileMap::GetTile(int row, int col)
{
    if (!IsInBounds(row, col))
    {
        throw std::out_of_range("Tilemap::GetTile out of bounds");
    }

    return tiles[(row * cols) + col];
}

const TileCell& TileMap::GetTile(int row, int col) const
{
    if (!IsInBounds(row, col))
    {
        throw std::out_of_range("Tilemap::GetTile out of bounds");
    }

    return tiles[(row * cols) + col];
}

bool TileMap::WorldToGrid(float worldX, float worldY, int& row, int& col) const
{
    const float scaledTileSize = tileSize * scale;

    if (worldX < 0.0f || worldY < 0.0f)
    {
        return false;
    }

    col = static_cast<int>(worldX / scaledTileSize);
    row = static_cast<int>(worldY / scaledTileSize);

    return IsInBounds(row, col);
}

TerrainType TileMap::GetTerrainAtWorldPosition(float worldX, float worldY) const
{
    int row = 0;
    int col = 0;

    if (!WorldToGrid(worldX, worldY, row, col))
    {
        return TerrainType::Blocked;
    }

    return GetTile(row, col).terrain;
}

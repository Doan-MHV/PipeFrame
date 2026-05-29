

#ifndef PIPEFRAME_TILEMAP_H
#define PIPEFRAME_TILEMAP_H
#include <vector>
#include <string>

#include "TileCell.h"


class TileMap
{
public:
    TileMap(int rows, int cols, int tileSize, float scale);

    int GetRows() const;
    int GetCols() const;
    int GetTileSize() const;
    int GetWorldWidth() const;
    int GetWorldHeight() const;
    float GetScale() const;
    const std::string& GetTextureAssetId() const;
    void SetTextureAssetId(const std::string& textureAssetId);
    TileCell& GetTile(int row, int col);
    const TileCell& GetTile(int row, int col) const;

    bool IsInBounds(int row, int col) const;

    bool WorldToGrid(float worldX, float worldY, int& row, int& col) const;
    TerrainType GetTerrainAtWorldPosition(float worldX, float worldY) const;

private:
    int rows;
    int cols;
    int tileSize;
    float scale;
    std::vector<TileCell> tiles;
    std::string textureAssetId;
};


#endif //PIPEFRAME_TILEMAP_H

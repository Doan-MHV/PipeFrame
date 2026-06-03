#ifndef PIPEFRAME_TILEINSPECTOR_H
#define PIPEFRAME_TILEINSPECTOR_H

#include "EditorSessionState.h"
#include "Map/TileMap.h"

class TileInspector
{
public:
    bool Draw(
        TileMap* tileMap,
        EditorSessionState& state
    );

private:
    TileMap* editedTileMap = nullptr;
    int pendingRows = 0;
    int pendingCols = 0;
    int pendingTileSize = 0;
    float pendingScale = 1.0f;

    void SyncPendingMapSettings(TileMap* tileMap);
    const char* TerrainTypeToString(TerrainType terrain) const;
};

#endif // PIPEFRAME_TILEINSPECTOR_H

#ifndef PIPEFRAME_TILEINSPECTOR_H
#define PIPEFRAME_TILEINSPECTOR_H

#include "EditorSessionState.h"
#include "Map/TileMap.h"

class TileInspector
{
public:
    bool Draw(
        const TileMap* tileMap,
        const EditorSessionState& state
    ) const;

private:
    const char* TerrainTypeToString(TerrainType terrain) const;
};

#endif // PIPEFRAME_TILEINSPECTOR_H

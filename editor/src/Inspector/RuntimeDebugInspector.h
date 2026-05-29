#ifndef PIPEFRAME_RUNTIMEDEBUGINSPECTOR_H
#define PIPEFRAME_RUNTIMEDEBUGINSPECTOR_H

#include <string>
#include <unordered_map>

#include "Game/EngineMode.h"
#include "Map/TerrainType.h"
#include "Fields/FieldGrid.h"

class Entity;
class TileMap;

class RuntimeDebugInspector
{
public:
    void Draw(
        EngineMode mode,
        Entity selectedEntity,
        const TileMap* tileMap,
        const std::unordered_map<std::string, FieldGrid>& fieldGrids
    );

private:
    double SampleFieldAt(
        const FieldGrid& fieldGrid,
        float worldX,
        float worldY
    ) const;

    const char* TerrainTypeToString(TerrainType terrain) const;
};

#endif

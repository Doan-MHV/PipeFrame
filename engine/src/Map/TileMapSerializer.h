

#ifndef PIPEFRAME_TILEMAPSERIALIZER_H
#define PIPEFRAME_TILEMAPSERIALIZER_H

#include <string>
#include "TileMap.h"

class TileMapSerializer
{
public:
    static bool SaveVisualMap(const TileMap& tileMap, const std::string& filePath);
    static bool SaveTerrainMap(const TileMap& tileMap, const std::string& filePath);
};


#endif //PIPEFRAME_TILEMAPSERIALIZER_H

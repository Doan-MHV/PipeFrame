

#ifndef PIPEFRAME_TILEMAPSERIALIZER_H
#define PIPEFRAME_TILEMAPSERIALIZER_H

#include <memory>
#include <string>
#include "TileMap.h"

class TileMapSerializer
{
public:
    static bool LoadTileMap(const std::string& filePath, std::unique_ptr<TileMap>& tileMap);
    static bool SaveTileMap(const TileMap& tileMap, const std::string& filePath);
};


#endif //PIPEFRAME_TILEMAPSERIALIZER_H

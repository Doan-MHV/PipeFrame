

#ifndef PIPEFRAME_TILECELL_H
#define PIPEFRAME_TILECELL_H

#include "TerrainType.h"

struct TileCell
{
    int tilesetColumn = 0;
    int tilesetRow = 0;
    TerrainType terrain = TerrainType::Land;
};

#endif //PIPEFRAME_TILECELL_H

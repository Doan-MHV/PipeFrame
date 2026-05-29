#ifndef PIPEFRAME_EDITORSESSIONSTATE_H
#define PIPEFRAME_EDITORSESSIONSTATE_H

#include "EditorTypes.h"
#include "Map/TileCell.h"

struct EditorSessionState
{
    int selectedEntityId = -1;
    int selectedProjectObjectId = -1;

    EditorTool activeTool = EditorTool::EntitySelect;

    bool hasSelectedTile = false;
    int selectedTileRow = -1;
    int selectedTileCol = -1;

    int brushTileRow = 0;
    int brushTileCol = 0;

    TerrainType terrainBrush = TerrainType::Land;
    bool showTerrainOverlay = false;
    bool showFieldOverlay = false;
    bool showPathDebugOverlay = false;
    bool showColliderOverlay = false;

    char saveAsPathBuffer[512] = {};
};

#endif // PIPEFRAME_EDITORSESSIONSTATE_H

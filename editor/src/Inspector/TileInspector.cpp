#include "TileInspector.h"

#include "imgui.h"

bool TileInspector::Draw(
    const TileMap* tileMap,
    const EditorSessionState& state
) const
{
    if (state.activeTool != EditorTool::TileSelect || !state.hasSelectedTile || !tileMap)
    {
        return false;
    }

    const TileCell& tile = tileMap->GetTile(
        state.selectedTileRow,
        state.selectedTileCol
    );

    ImGui::Text("Selected Tile");
    ImGui::Separator();
    ImGui::Text("Grid: (%d, %d)", state.selectedTileRow, state.selectedTileCol);
    ImGui::Text("Atlas: (%d, %d)", tile.tilesetRow, tile.tilesetColumn);
    ImGui::Text("Terrain: %s", TerrainTypeToString(tile.terrain));

    return true;
}

const char* TileInspector::TerrainTypeToString(TerrainType terrain) const
{
    switch (terrain)
    {
    case TerrainType::Land:
        return "Land";

    case TerrainType::Water:
        return "Water";

    case TerrainType::Runway:
        return "Runway";

    case TerrainType::Blocked:
        return "Blocked";
    }

    return "Unknown";
}

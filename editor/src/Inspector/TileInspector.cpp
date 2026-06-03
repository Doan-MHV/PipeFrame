#include "TileInspector.h"

#include <algorithm>
#include <stdexcept>

#include "imgui.h"

void TileInspector::SyncPendingMapSettings(TileMap* tileMap)
{
    if (editedTileMap == tileMap)
    {
        return;
    }

    editedTileMap = tileMap;
    pendingRows = tileMap->GetRows();
    pendingCols = tileMap->GetCols();
    pendingTileSize = tileMap->GetTileSize();
    pendingScale = tileMap->GetScale();
}

bool TileInspector::Draw(
    TileMap* tileMap,
    EditorSessionState& state
)
{
    if (state.activeTool != EditorTool::TileSelect || !tileMap)
    {
        return false;
    }

    SyncPendingMapSettings(tileMap);

    ImGui::SeparatorText("Map Settings");

    ImGui::InputInt("Rows", &pendingRows);
    ImGui::InputInt("Columns", &pendingCols);
    ImGui::InputInt("Tile Size", &pendingTileSize);
    ImGui::DragFloat("Scale", &pendingScale, 0.05f, 0.05f, 16.0f, "%.2f");

    pendingRows = std::max(pendingRows, 1);
    pendingCols = std::max(pendingCols, 1);
    pendingTileSize = std::max(pendingTileSize, 1);
    pendingScale = std::max(pendingScale, 0.05f);

    ImGui::Text(
        "World: %d x %d px",
        static_cast<int>(pendingCols * pendingTileSize * pendingScale),
        static_cast<int>(pendingRows * pendingTileSize * pendingScale)
    );

    const bool mapSettingsChanged =
        pendingRows != tileMap->GetRows() ||
        pendingCols != tileMap->GetCols() ||
        pendingTileSize != tileMap->GetTileSize() ||
        pendingScale != tileMap->GetScale();

    if (!mapSettingsChanged)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Apply Map Settings"))
    {
        try
        {
            tileMap->Resize(pendingRows, pendingCols);
            tileMap->SetTileSize(pendingTileSize);
            tileMap->SetScale(pendingScale);

            if (state.hasSelectedTile)
            {
                state.selectedTileRow = std::clamp(state.selectedTileRow, 0, tileMap->GetRows() - 1);
                state.selectedTileCol = std::clamp(state.selectedTileCol, 0, tileMap->GetCols() - 1);
            }

            SyncPendingMapSettings(tileMap);
        }
        catch (const std::exception& exception)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                "Cannot resize map: %s",
                exception.what()
            );
        }
    }

    if (!mapSettingsChanged)
    {
        ImGui::EndDisabled();
    }

    ImGui::Spacing();

    if (!state.hasSelectedTile)
    {
        ImGui::TextDisabled("No tile selected");
        return true;
    }

    const TileCell& tile = tileMap->GetTile(
        state.selectedTileRow,
        state.selectedTileCol
    );

    ImGui::SeparatorText("Selected Tile");
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

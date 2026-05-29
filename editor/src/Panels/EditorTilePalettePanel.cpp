#include "EditorTilePalettePanel.h"

#include "imgui.h"

const char* EditorTilePalettePanel::TerrainTypeToString(TerrainType terrain) const
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

void EditorTilePalettePanel::Draw(
    const TileMap* tileMap,
    SDL_Texture* tilePaletteTexture,
    EditorSessionState& state
)
{
    ImGui::Begin("Tile Palette");

    ImGui::Text("Brush");
    ImGui::Separator();
    ImGui::Text("Atlas: (%d, %d)", state.brushTileRow, state.brushTileCol);

    if (!tilePaletteTexture)
    {
        ImGui::Text("Tile palette texture not available");
        ImGui::End();
        return;
    }

    const int atlasRows = 3;
    const int atlasCols = 10;
    const float tileSize = 32.0f;
    const float textureWidth = 320.0f;
    const float textureHeight = 96.0f;
    const float buttonSize = 40.0f;

    {
        float u0 = (state.brushTileCol * tileSize) / textureWidth;
        float v0 = (state.brushTileRow * tileSize) / textureHeight;
        float u1 = ((state.brushTileCol + 1) * tileSize) / textureWidth;
        float v1 = ((state.brushTileRow + 1) * tileSize) / textureHeight;

        ImGui::Spacing();
        ImGui::Text("Preview");
        ImGui::Image(
            reinterpret_cast<ImTextureID>(tilePaletteTexture),
            ImVec2(64.0f, 64.0f),
            ImVec2(u0, v0),
            ImVec2(u1, v1)
        );
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Tiles");

    for (int row = 0; row < atlasRows; row++)
    {
        for (int col = 0; col < atlasCols; col++)
        {
            float u0 = (col * tileSize) / textureWidth;
            float v0 = (row * tileSize) / textureHeight;
            float u1 = ((col + 1) * tileSize) / textureWidth;
            float v1 = ((row + 1) * tileSize) / textureHeight;

            bool selected = (state.brushTileRow == row && state.brushTileCol == col);

            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 120, 200, 255));
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 0, 255));
            }

            ImGui::PushID(row * atlasCols + col);

            if (ImGui::ImageButton(
                "##tile",
                reinterpret_cast<ImTextureID>(tilePaletteTexture),
                ImVec2(buttonSize, buttonSize),
                ImVec2(u0, v0),
                ImVec2(u1, v1)
            ))
            {
                state.brushTileRow = row;
                state.brushTileCol = col;
            }

            ImGui::PopID();

            if (selected)
            {
                ImGui::PopStyleColor(2);
            }

            if (col < atlasCols - 1)
            {
                ImGui::SameLine();
            }
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Terrain Brush");

    if (ImGui::Button("Land"))
    {
        state.terrainBrush = TerrainType::Land;
    }
    ImGui::SameLine();

    if (ImGui::Button("Water"))
    {
        state.terrainBrush = TerrainType::Water;
    }
    ImGui::SameLine();

    if (ImGui::Button("Blocked"))
    {
        state.terrainBrush = TerrainType::Blocked;
    }

    ImGui::Text("Terrain Brush: %s", TerrainTypeToString(state.terrainBrush));

    ImGui::End();
}

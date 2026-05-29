#include "NewLevelDialog.h"

#include "imgui.h"

void NewLevelDialog::Open()
{
    ImGui::OpenPopup("New Level");
}

NewLevelResult NewLevelDialog::Draw()
{
    NewLevelResult result;

    if (!ImGui::BeginPopupModal("New Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return result;
    }

    ImGui::TextUnformatted("Create a blank level in the active project.");
    ImGui::Spacing();

    ImGui::InputText("Level Name", levelNameBuffer, sizeof(levelNameBuffer));
    ImGui::InputInt("Rows", &rows);
    ImGui::InputInt("Columns", &cols);
    ImGui::InputInt("Tile Size", &tileSize);
    ImGui::InputFloat("Scale", &scale);

    if (rows < 1)
    {
        rows = 1;
    }

    if (cols < 1)
    {
        cols = 1;
    }

    if (tileSize < 1)
    {
        tileSize = 1;
    }

    if (scale <= 0.0f)
    {
        scale = 1.0f;
    }

    ImGui::Spacing();

    if (ImGui::Button("Create Level"))
    {
        result.requestedCreate = true;
        result.levelName = levelNameBuffer;
        result.rows = rows;
        result.cols = cols;
        result.tileSize = tileSize;
        result.scale = scale;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return result;
}

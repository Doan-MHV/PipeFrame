#include "OpenProjectDialog.h"

#include "imgui.h"

void OpenProjectDialog::Open()
{
    ImGui::OpenPopup("Open Project");
}

OpenProjectResult OpenProjectDialog::Draw()
{
    OpenProjectResult result;

    if (!ImGui::BeginPopupModal("Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return result;
    }

    ImGui::TextUnformatted("Open a PipeFrameProject.json file.");
    ImGui::Spacing();

    ImGui::InputText(
        "Project File",
        projectFilePathBuffer,
        sizeof(projectFilePathBuffer)
    );

    ImGui::Spacing();

    if (ImGui::Button("Open Project"))
    {
        result.requestedOpen = true;
        result.projectFilePath = projectFilePathBuffer;
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

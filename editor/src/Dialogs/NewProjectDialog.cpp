#include "NewProjectDialog.h"

#include "imgui.h"

void NewProjectDialog::Open()
{
    ImGui::OpenPopup("New Project");
}

NewProjectResult NewProjectDialog::Draw()
{
    NewProjectResult result;

    if (!ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return result;
    }

    ImGui::TextUnformatted("Create a C++ PipeFrame project.");
    ImGui::Spacing();

    ImGui::InputText("Project Name", projectNameBuffer, sizeof(projectNameBuffer));
    ImGui::InputTextWithHint(
        "Parent Folder",
        "empty = current projects folder",
        parentDirectoryBuffer,
        sizeof(parentDirectoryBuffer)
    );

    ImGui::Checkbox("Copy sample ant/marker assets into this project", &copySampleAntAssets);

    ImGui::Spacing();

    if (ImGui::Button("Create Project"))
    {
        result.requestedCreate = true;
        result.projectName = projectNameBuffer;
        result.parentDirectory = parentDirectoryBuffer;
        result.copySampleAntAssets = copySampleAntAssets;
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

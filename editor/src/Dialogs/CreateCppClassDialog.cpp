#include "CreateCppClassDialog.h"

#include "imgui.h"

namespace
{
const char* GetKindHint(int selectedKind)
{
    switch (static_cast<CppClassKind>(selectedKind))
    {
        case CppClassKind::Component:
            return "Editable ECS data attached to an entity.";
        case CppClassKind::ProjectSystem:
            return "Project-owned update logic that runs during native simulation.";
        case CppClassKind::EntityClass:
            return "Spawnable entity recipe that adds default components.";
        case CppClassKind::DenseAgentSimulation:
            return "Dense array simulation for thousands of similar agents.";
        case CppClassKind::PhysicsScenario:
            return "Project-owned physics setup, reset, and debug scenario.";
    }

    return "";
}
}

void CreateCppClassDialog::Open()
{
    ImGui::OpenPopup("Create C++ Class");
}

CreateCppClassResult CreateCppClassDialog::Draw()
{
    CreateCppClassResult result;

    if (!ImGui::BeginPopupModal("Create C++ Class", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return result;
    }

    ImGui::TextUnformatted("Create a starter C++ class in the active project Source folder.");
    ImGui::Spacing();

    const char* classKinds[] = {
        "Component",
        "Project System",
        "Entity Class",
        "Dense Agent Simulation",
        "Physics Scenario"
    };

    ImGui::Combo("Kind", &selectedKind, classKinds, IM_ARRAYSIZE(classKinds));
    ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer));
    ImGui::TextDisabled("%s", GetKindHint(selectedKind));

    ImGui::Spacing();

    if (ImGui::Button("Create Class"))
    {
        result.requestedCreate = true;
        result.kind = static_cast<CppClassKind>(selectedKind);
        result.className = classNameBuffer;
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

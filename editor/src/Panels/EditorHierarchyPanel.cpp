#include "EditorHierarchyPanel.h"

#include <cstdio>
#include <vector>

#include <nlohmann/json.hpp>

#include "imgui.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Core/EditorEntityActions.h"
#include "Core/EditorViewModels.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Prefabs/PrefabRegistry.h"

namespace
{
void CopyToBuffer(char* buffer, std::size_t bufferSize, const std::string& value)
{
    std::snprintf(buffer, bufferSize, "%s", value.c_str());
}
}

EditorHierarchyResult EditorHierarchyPanel::Draw(
    const std::unique_ptr<Registry>& registry,
    EditorSessionState& state,
    const PrefabRegistry& prefabRegistry,
    const ClassRegistry& classRegistry,
    const ComponentRegistry& componentRegistry,
    const SDL_FRect& camera
)
{
    EditorHierarchyResult result;

    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Create Entity"))
    {
        Entity entity = CreateEditorEntity(registry, camera);

        state.selectedEntityId = entity.GetId();
        state.hasSelectedTile = false;
    }

    ImGui::SameLine();

    Entity selectedEntity = FindEntityById(registry, state.selectedEntityId);
    const bool hasSelectedEntity = selectedEntity.GetId() >= 0;

    if (!hasSelectedEntity)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Duplicate"))
    {
        Entity duplicatedEntity = DuplicateEditorEntity(
            registry,
            selectedEntity,
            componentRegistry,
            camera
        );
        state.selectedEntityId = duplicatedEntity.GetId();
        state.hasSelectedTile = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete"))
    {
        DeleteEditorEntity(selectedEntity);
        state.selectedEntityId = -1;
        state.hasSelectedTile = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Save As Prefab"))
    {
        prefabSourceEntityId = selectedEntity.GetId();
        CopyToBuffer(prefabNameBuffer, sizeof(prefabNameBuffer), GetEntityDisplayName(selectedEntity));
        ImGui::OpenPopup("SaveEntityAsPrefabPopup");
    }

    if (!hasSelectedEntity)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupModal("SaveEntityAsPrefabPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save selected entity as prefab");
        ImGui::InputText("Prefab Name", prefabNameBuffer, sizeof(prefabNameBuffer));

        if (ImGui::Button("Save"))
        {
            result.requestedPrefabSave = true;
            result.prefabSourceEntityId = prefabSourceEntityId;
            result.prefabName = prefabNameBuffer;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::SeparatorText("Prefabs");

    const std::vector<std::string> prefabIds = prefabRegistry.GetPrefabIds();
    if (prefabIds.empty())
    {
        ImGui::TextDisabled("No prefabs found. Select an entity and use Save As Prefab.");
        ImGui::BeginDisabled();
        ImGui::Button("Create From Prefab");
        ImGui::EndDisabled();
    }
    else
    {
        if (selectedPrefabIndex >= static_cast<int>(prefabIds.size()))
        {
            selectedPrefabIndex = 0;
        }

        const std::string& selectedPrefabId = prefabIds[selectedPrefabIndex];

        if (ImGui::BeginCombo("Prefab", selectedPrefabId.c_str()))
        {
            for (int index = 0; index < static_cast<int>(prefabIds.size()); index++)
            {
                const bool isSelected = selectedPrefabIndex == index;

                if (ImGui::Selectable(prefabIds[index].c_str(), isSelected))
                {
                    selectedPrefabIndex = index;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (const PrefabDefinition* prefab = prefabRegistry.GetPrefab(selectedPrefabId))
        {
            const nlohmann::json& prefabJson = prefab->entityJson;

            if (prefabJson.contains("tag") && prefabJson["tag"].is_string())
            {
                ImGui::Text("Tag: %s", prefabJson["tag"].get<std::string>().c_str());
            }

            if (prefabJson.contains("group") && prefabJson["group"].is_string())
            {
                ImGui::Text("Group: %s", prefabJson["group"].get<std::string>().c_str());
            }

            if (prefabJson.contains("components") && prefabJson["components"].is_object())
            {
                std::string componentList;

                for (auto it = prefabJson["components"].begin(); it != prefabJson["components"].end(); ++it)
                {
                    if (!componentList.empty())
                    {
                        componentList += ", ";
                    }

                    componentList += it.key();
                }

                ImGui::TextWrapped("Components: %s", componentList.c_str());
            }
        }

        if (ImGui::Button("Create From Prefab"))
        {
            Entity entity = CreateEntityFromPrefab(
                registry,
                prefabRegistry,
                componentRegistry,
                selectedPrefabId,
                camera
            );

            state.selectedEntityId = entity.GetId();
            state.hasSelectedTile = false;
        }
    }

    ImGui::SeparatorText("Project Classes");

    const auto& projectClasses = classRegistry.GetEntityClasses();
    if (projectClasses.empty())
    {
        ImGui::TextDisabled("No project entity classes registered.");
        ImGui::BeginDisabled();
        ImGui::Button("Create From Class");
        ImGui::EndDisabled();
    }
    else
    {
        if (selectedProjectClassIndex >= static_cast<int>(projectClasses.size()))
        {
            selectedProjectClassIndex = 0;
        }

        const EntityClassMetadata& selectedClass = projectClasses[selectedProjectClassIndex];
        if (ImGui::BeginCombo("Class", selectedClass.displayName.c_str()))
        {
            for (int index = 0; index < static_cast<int>(projectClasses.size()); index++)
            {
                const bool isSelected = selectedProjectClassIndex == index;
                const EntityClassMetadata& metadata = projectClasses[index];
                const std::string label = metadata.displayName + " [" + metadata.category + "]";

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    selectedProjectClassIndex = index;
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload(
                        "PIPEFRAME_PROJECT_CLASS",
                        metadata.typeName.c_str(),
                        metadata.typeName.size() + 1
                    );
                    ImGui::Text("Create %s", metadata.displayName.c_str());
                    ImGui::EndDragDropSource();
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::TextDisabled("Type: %s", selectedClass.typeName.c_str());
        ImGui::TextDisabled("Category: %s", selectedClass.category.c_str());

        if (ImGui::Button("Create From Class"))
        {
            Entity entity = CreateEntityFromProjectClass(
                registry,
                classRegistry,
                selectedClass.typeName,
                camera
            );

            state.selectedEntityId = entity.GetId();
            state.hasSelectedTile = false;
        }
    }

    ImGui::Separator();
    ImGui::Text("Entities");
    ImGui::Separator();

    for (auto entity : registry->GetAllEntities())
    {
        bool isSelected = (entity.GetId() == state.selectedEntityId);

        std::string label;

        if (entity.HasComponent<PersistentIdComponent>())
        {
            label = entity.GetComponent<PersistentIdComponent>().value;
        }
        else
        {
            label = "Entity " + std::to_string(entity.GetId());
        }

        const std::string tag = entity.registry->GetEntityTag(entity);
        const std::string group = entity.registry->GetEntityGroup(entity);

        if (!tag.empty())
        {
            label += " [" + tag + "]";
        }
        else if (!group.empty())
        {
            label += " [" + group + "]";
        }

        label += "##" + std::to_string(entity.GetId());

        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            state.selectedEntityId = entity.GetId();
            state.hasSelectedTile = false;
        }
    }

    ImGui::End();
    return result;
}

#include "IdentityInspector.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "imgui.h"
#include "Components/PersistentIdComponent.h"
#include "ECS/ECS.h"

namespace
{
std::string FindCurrentGroup(Entity entity, const std::vector<std::string>& knownGroups)
{
    const std::string registryGroup = entity.registry->GetEntityGroup(entity);
    if (!registryGroup.empty())
    {
        return registryGroup;
    }

    for (const std::string& group : knownGroups)
    {
        if (entity.BelongsToGroup(group))
        {
            return group;
        }
    }

    return "";
}

std::vector<std::string> BuildOptionsWithCurrent(
    const std::vector<std::string>& configuredOptions,
    const std::string& currentValue
)
{
    std::vector<std::string> options;

    for (const std::string& option : configuredOptions)
    {
        if (option.empty())
        {
            continue;
        }

        if (std::find(options.begin(), options.end(), option) == options.end())
        {
            options.push_back(option);
        }
    }

    if (!currentValue.empty() &&
        std::find(options.begin(), options.end(), currentValue) == options.end())
    {
        options.push_back(currentValue);
    }

    return options;
}

bool DrawStringOptionCombo(
    const char* label,
    const std::vector<std::string>& options,
    const std::string& currentValue,
    std::string& selectedValue
)
{
    const char* preview = currentValue.empty() ? "None" : currentValue.c_str();
    bool changed = false;

    if (ImGui::BeginCombo(label, preview))
    {
        const bool noneSelected = currentValue.empty();
        if (ImGui::Selectable("None", noneSelected))
        {
            selectedValue.clear();
            changed = true;
        }

        if (noneSelected)
        {
            ImGui::SetItemDefaultFocus();
        }

        for (const std::string& option : options)
        {
            const bool isSelected = currentValue == option;
            if (ImGui::Selectable(option.c_str(), isSelected))
            {
                selectedValue = option;
                changed = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}
}

void IdentityInspector::SyncBuffers(Entity selectedEntity)
{
    inspectorEntityId = selectedEntity.GetId();
    persistentIdBuffer[0] = '\0';

    if (selectedEntity.HasComponent<PersistentIdComponent>())
    {
        const auto& persistentId = selectedEntity.GetComponent<PersistentIdComponent>();

        std::snprintf(
            persistentIdBuffer,
            sizeof(persistentIdBuffer),
            "%s",
            persistentId.value.c_str()
        );
    }
}

void IdentityInspector::Draw(
    const std::unique_ptr<Registry>& registry,
    const ProjectConfig& projectConfig,
    Entity selectedEntity
)
{
    if (inspectorEntityId != selectedEntity.GetId())
    {
        SyncBuffers(selectedEntity);
    }

    ImGui::Text("Identity");
    ImGui::Separator();

    bool persistentIdIsDuplicate = false;

    if (selectedEntity.HasComponent<PersistentIdComponent>())
    {
        auto& persistentId = selectedEntity.GetComponent<PersistentIdComponent>();

        if (ImGui::InputText("Persistent ID", persistentIdBuffer, sizeof(persistentIdBuffer)))
        {
            std::string candidate = persistentIdBuffer;

            if (candidate.empty())
            {
                candidate = "entity_" + std::to_string(selectedEntity.GetId());
            }

            if (IsPersistentIdUnique(registry, candidate, selectedEntity.GetId()))
            {
                persistentId.value = candidate;

                std::snprintf(
                    persistentIdBuffer,
                    sizeof(persistentIdBuffer),
                    "%s",
                    persistentId.value.c_str()
                );
            }
            else
            {
                persistentIdIsDuplicate = true;
            }
        }

        if (!IsPersistentIdUnique(registry, persistentIdBuffer, selectedEntity.GetId()))
        {
            persistentIdIsDuplicate = true;
        }

        if (persistentIdIsDuplicate)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                "Persistent ID must be unique"
            );
        }
    }
    else
    {
        ImGui::Text("Persistent ID: <missing>");
    }

    ImGui::Text("Runtime ID: %d", selectedEntity.GetId());
    ImGui::Separator();

    const std::string currentTag = selectedEntity.registry->GetEntityTag(selectedEntity);
    const std::vector<std::string> tagOptions = BuildOptionsWithCurrent(projectConfig.tags, currentTag);
    std::string selectedTag = currentTag;

    if (DrawStringOptionCombo("Tag", tagOptions, currentTag, selectedTag))
    {
        selectedEntity.RemoveTag();

        if (!selectedTag.empty())
        {
            selectedEntity.Tag(selectedTag);
        }
    }

    const std::string currentGroup = FindCurrentGroup(selectedEntity, projectConfig.groups);
    const std::vector<std::string> groupOptions = BuildOptionsWithCurrent(projectConfig.groups, currentGroup);
    std::string selectedGroup = currentGroup;

    if (DrawStringOptionCombo("Group", groupOptions, currentGroup, selectedGroup))
    {
        selectedEntity.RemoveGroup();

        if (!selectedGroup.empty())
        {
            selectedEntity.Group(selectedGroup);
        }
    }
}

bool IdentityInspector::IsPersistentIdUnique(
    const std::unique_ptr<Registry>& registry,
    const std::string& candidate,
    int ignoreEntityId
) const
{
    if (candidate.empty())
    {
        return false;
    }

    for (auto entity : registry->GetAllEntities())
    {
        if (entity.GetId() == ignoreEntityId)
        {
            continue;
        }

        if (!entity.HasComponent<PersistentIdComponent>())
        {
            continue;
        }

        if (entity.GetComponent<PersistentIdComponent>().value == candidate)
        {
            return false;
        }
    }

    return true;
}

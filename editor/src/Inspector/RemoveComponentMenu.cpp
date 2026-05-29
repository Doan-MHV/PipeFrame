#include "RemoveComponentMenu.h"

#include <string>

#include "imgui.h"

RemovedComponentType EditorInspector::DrawRemoveComponentMenu(
    Entity selectedEntity,
    const ComponentRegistry& componentRegistry
)
{
    RemovedComponentType removedComponent = RemovedComponentType::None;

    if (!ImGui::BeginPopup("RemoveComponentPopup"))
    {
        return removedComponent;
    }

    bool drewEngineHeader = false;
    bool drewProjectHeader = false;

    for (const auto& [typeName, component] : componentRegistry.GetComponents())
    {
        if (!component.editorRemovable || !component.hasComponent || !component.removeComponent)
        {
            continue;
        }

        if (!component.hasComponent(selectedEntity))
        {
            continue;
        }

        if (component.isEngineComponent && !drewEngineHeader)
        {
            ImGui::SeparatorText("Engine Components");
            drewEngineHeader = true;
        }

        if (!component.isEngineComponent && !drewProjectHeader)
        {
            ImGui::SeparatorText("Project Components");
            drewProjectHeader = true;
        }

        const std::string label = component.displayName.empty() ? typeName : component.displayName;
        if (ImGui::Selectable(label.c_str()))
        {
            component.removeComponent(selectedEntity);
            removedComponent = component.isEngineComponent ? RemovedComponentType::Engine : RemovedComponentType::Project;
        }
    }

    ImGui::EndPopup();
    return removedComponent;
}

#include "AddComponentMenu.h"

#include <string>

#include "imgui.h"

AddedComponentType EditorInspector::DrawAddComponentMenu(
    Entity selectedEntity,
    AssetRegistry& assetRegistry,
    const ComponentRegistry& componentRegistry
)
{
    (void)assetRegistry;
    AddedComponentType addedComponent = AddedComponentType::None;

    if (!ImGui::BeginPopup("AddComponentPopup"))
    {
        return addedComponent;
    }

    bool drewEngineHeader = false;
    bool drewProjectHeader = false;

    for (const auto& [typeName, component] : componentRegistry.GetComponents())
    {
        if (!component.editorAddable || !component.hasComponent || !component.addDefaultComponent)
        {
            continue;
        }

        if (component.hasComponent(selectedEntity))
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
            component.addDefaultComponent(selectedEntity);
            addedComponent = component.isEngineComponent ? AddedComponentType::Engine : AddedComponentType::Project;
        }
    }

    ImGui::EndPopup();
    return addedComponent;
}

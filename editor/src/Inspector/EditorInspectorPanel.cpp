#include "EditorInspectorPanel.h"

#include <cstdio>
#include <variant>

#include "AddComponentMenu.h"
#include "ComponentInspectors.h"
#include "RemoveComponentMenu.h"
#include "imgui.h"

#include "ECS/ECS.h"
#include "Simulation/ProjectModule.h"

namespace
{
void DrawProjectProperty(Entity selectedEntity, const PropertyMetadata& property)
{
    if (!property.visible)
    {
        return;
    }

    if (!property.getValue)
    {
        ImGui::TextDisabled("%s: <no accessor>", property.displayName.c_str());
        return;
    }

    PropertyValue value = property.getValue(selectedEntity);

    switch (property.type)
    {
    case PropertyType::Int:
        if (int* intValue = std::get_if<int>(&value))
        {
            int editedValue = *intValue;
            const int min = property.hasMin ? static_cast<int>(property.min) : 0;
            const int max = property.hasMax ? static_cast<int>(property.max) : 0;
            const float step = static_cast<float>(property.step);

            if (!property.editable)
            {
                ImGui::Text("%s: %d", property.displayName.c_str(), editedValue);
                break;
            }

            if (property.setValue && ImGui::DragInt(property.displayName.c_str(), &editedValue, step, min, max))
            {
                property.setValue(selectedEntity, editedValue);
            }
        }
        break;
    case PropertyType::Float:
        if (float* floatValue = std::get_if<float>(&value))
        {
            float editedValue = *floatValue;
            const float min = property.hasMin ? static_cast<float>(property.min) : 0.0f;
            const float max = property.hasMax ? static_cast<float>(property.max) : 0.0f;
            const float step = static_cast<float>(property.step);

            if (!property.editable)
            {
                ImGui::Text("%s: %.3f", property.displayName.c_str(), editedValue);
                break;
            }

            if (property.setValue && ImGui::DragFloat(property.displayName.c_str(), &editedValue, step, min, max, "%.3f"))
            {
                property.setValue(selectedEntity, editedValue);
            }
        }
        break;
    case PropertyType::Double:
        if (double* doubleValue = std::get_if<double>(&value))
        {
            float editedValue = static_cast<float>(*doubleValue);
            const float min = property.hasMin ? static_cast<float>(property.min) : 0.0f;
            const float max = property.hasMax ? static_cast<float>(property.max) : 0.0f;
            const float step = static_cast<float>(property.step);

            if (!property.editable)
            {
                ImGui::Text("%s: %.3f", property.displayName.c_str(), editedValue);
                break;
            }

            if (property.setValue && ImGui::DragFloat(property.displayName.c_str(), &editedValue, step, min, max, "%.3f"))
            {
                property.setValue(selectedEntity, static_cast<double>(editedValue));
            }
        }
        break;
    case PropertyType::Bool:
        if (bool* boolValue = std::get_if<bool>(&value))
        {
            bool editedValue = *boolValue;
            if (!property.editable)
            {
                ImGui::Text("%s: %s", property.displayName.c_str(), editedValue ? "true" : "false");
                break;
            }

            if (property.setValue && ImGui::Checkbox(property.displayName.c_str(), &editedValue))
            {
                property.setValue(selectedEntity, editedValue);
            }
        }
        break;
    case PropertyType::String:
        if (std::string* stringValue = std::get_if<std::string>(&value))
        {
            if (!property.editable)
            {
                ImGui::Text("%s: %s", property.displayName.c_str(), stringValue->c_str());
                break;
            }

            char buffer[256] = {};
            std::snprintf(buffer, sizeof(buffer), "%s", stringValue->c_str());
            if (property.setValue && ImGui::InputText(property.displayName.c_str(), buffer, sizeof(buffer)))
            {
                property.setValue(selectedEntity, std::string(buffer));
            }
        }
        break;
    case PropertyType::Enum:
        if (std::string* stringValue = std::get_if<std::string>(&value))
        {
            if (!property.editable)
            {
                ImGui::Text("%s: %s", property.displayName.c_str(), stringValue->c_str());
                break;
            }

            if (ImGui::BeginCombo(property.displayName.c_str(), stringValue->c_str()))
            {
                for (const std::string& option : property.options)
                {
                    const bool selected = option == *stringValue;
                    if (ImGui::Selectable(option.c_str(), selected) && property.setValue)
                    {
                        property.setValue(selectedEntity, option);
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        break;
    case PropertyType::Vec2:
        if (glm::vec2* vecValue = std::get_if<glm::vec2>(&value))
        {
            float editedValue[2] = {vecValue->x, vecValue->y};
            const float min = property.hasMin ? static_cast<float>(property.min) : 0.0f;
            const float max = property.hasMax ? static_cast<float>(property.max) : 0.0f;
            const float step = static_cast<float>(property.step);

            if (!property.editable)
            {
                ImGui::Text(
                    "%s: %.3f, %.3f",
                    property.displayName.c_str(),
                    editedValue[0],
                    editedValue[1]
                );
                break;
            }

            if (property.setValue && ImGui::DragFloat2(property.displayName.c_str(), editedValue, step, min, max, "%.3f"))
            {
                property.setValue(selectedEntity, glm::vec2(editedValue[0], editedValue[1]));
            }
        }
        break;
    case PropertyType::Rect:
        if (SDL_FRect* rectValue = std::get_if<SDL_FRect>(&value))
        {
            float position[2] = {rectValue->x, rectValue->y};
            float size[2] = {rectValue->w, rectValue->h};
            const float min = property.hasMin ? static_cast<float>(property.min) : 0.0f;
            const float max = property.hasMax ? static_cast<float>(property.max) : 0.0f;
            const float step = static_cast<float>(property.step);

            if (!property.editable)
            {
                ImGui::Text(
                    "%s: %.3f, %.3f, %.3f, %.3f",
                    property.displayName.c_str(),
                    rectValue->x,
                    rectValue->y,
                    rectValue->w,
                    rectValue->h
                );
                break;
            }

            bool changed = false;
            ImGui::PushID(property.name.c_str());
            changed |= ImGui::DragFloat2("Position", position, step, min, max, "%.3f");
            changed |= ImGui::DragFloat2("Size", size, step, min, max, "%.3f");
            ImGui::PopID();

            if (changed && property.setValue)
            {
                property.setValue(
                    selectedEntity,
                    SDL_FRect{position[0], position[1], size[0], size[1]}
                );
            }
        }
        break;
    case PropertyType::Color:
        if (SDL_Color* colorValue = std::get_if<SDL_Color>(&value))
        {
            float editedColor[4] = {
                static_cast<float>(colorValue->r) / 255.0f,
                static_cast<float>(colorValue->g) / 255.0f,
                static_cast<float>(colorValue->b) / 255.0f,
                static_cast<float>(colorValue->a) / 255.0f
            };

            if (!property.editable)
            {
                ImGui::Text(
                    "%s: %d, %d, %d, %d",
                    property.displayName.c_str(),
                    colorValue->r,
                    colorValue->g,
                    colorValue->b,
                    colorValue->a
                );
                break;
            }

            if (property.setValue && ImGui::ColorEdit4(property.displayName.c_str(), editedColor))
            {
                property.setValue(
                    selectedEntity,
                    SDL_Color{
                        static_cast<Uint8>(editedColor[0] * 255.0f),
                        static_cast<Uint8>(editedColor[1] * 255.0f),
                        static_cast<Uint8>(editedColor[2] * 255.0f),
                        static_cast<Uint8>(editedColor[3] * 255.0f)
                    }
                );
            }
        }
        break;
    case PropertyType::Json:
        if (nlohmann::json* jsonValue = std::get_if<nlohmann::json>(&value))
        {
            if (!jsonValue->is_object())
            {
                *jsonValue = nlohmann::json::object();
            }

            ImGui::Text("%s", property.displayName.c_str());
            ImGui::Indent();
            for (auto it = jsonValue->begin(); it != jsonValue->end(); ++it)
            {
                ImGui::Text("%s: %s", it.key().c_str(), it.value().dump().c_str());
            }
            ImGui::Unindent();
        }
        break;
    }
}

void DrawProjectComponents(Entity selectedEntity, const ComponentRegistry& componentRegistry)
{
    for (const auto& [typeName, component] : componentRegistry.GetComponents())
    {
        if (component.isEngineComponent || !component.editorInspectable)
        {
            continue;
        }

        if (!component.hasComponent || !component.hasComponent(selectedEntity))
        {
            continue;
        }

        ImGui::SeparatorText(component.displayName.empty() ? typeName.c_str() : component.displayName.c_str());

        for (const PropertyMetadata& property : component.properties)
        {
            DrawProjectProperty(selectedEntity, property);
        }
    }
}
}

void EditorInspectorPanel::Draw(
    const std::unique_ptr<Registry>& registry,
    EngineMode mode,
    const ProjectConfig& projectConfig,
    const ComponentRegistry& componentRegistry,
    const TileMap* tileMap,
    std::unique_ptr<AssetRegistry>& assetRegistry,
    const std::unordered_map<std::string, FieldGrid>& fieldGrids,
    ProjectModule* projectModule,
    EditorSessionState& state
)
{
    ImGui::Begin("Inspector");

    ImGui::Separator();

    if (tileInspector.Draw(tileMap, state))
    {
        ImGui::End();
        return;
    }

    if (state.selectedEntityId < 0)
    {
        if (state.selectedProjectObjectId >= 0 && projectModule)
        {
            const ProjectObjectInspector projectInspector = projectModule->GetSelectedProjectObjectInspector();
            if (projectInspector.id >= 0)
            {
                ImGui::SeparatorText(projectInspector.displayName.empty()
                    ? projectInspector.typeName.c_str()
                    : projectInspector.displayName.c_str());
                ImGui::Text("Runtime ID: %d", projectInspector.id);
                ImGui::Text("Type: %s", projectInspector.typeName.c_str());
                ImGui::Separator();
                for (const ProjectObjectProperty& property : projectInspector.properties)
                {
                    ImGui::Text("%s: %s", property.name.c_str(), property.value.c_str());
                }
                ImGui::End();
                return;
            }
        }

        ImGui::Text("No entity selected");
        ImGui::Separator();
        ImGui::End();
        return;
    }

    Entity selectedEntity(-1);
    bool found = false;

    for (auto entity : registry->GetAllEntities())
    {
        if (entity.GetId() == state.selectedEntityId)
        {
            selectedEntity = entity;
            found = true;
            break;
        }
    }

    if (!found)
    {
        ImGui::Text("Selected entity no longer exists");
        state.selectedEntityId = -1;
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Add Component");
    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Component"))
    {
        ImGui::OpenPopup("RemoveComponentPopup");
    }

    const AddedComponentType addedComponent = EditorInspector::DrawAddComponentMenu(
        selectedEntity,
        *assetRegistry,
        componentRegistry
    );

    const RemovedComponentType removedComponent =
        EditorInspector::DrawRemoveComponentMenu(selectedEntity, componentRegistry);
    (void)addedComponent;
    (void)removedComponent;

    identityInspector.Draw(registry, projectConfig, selectedEntity);

    EditorInspector::DrawTransform(selectedEntity);
    EditorInspector::DrawRigidBody(selectedEntity);
    EditorInspector::DrawSoftCollision(selectedEntity);
    EditorInspector::DrawKeyboardControl(selectedEntity);
    EditorInspector::DrawSprite(selectedEntity, *assetRegistry);
    EditorInspector::DrawHealth(selectedEntity);
    EditorInspector::DrawAttributes(selectedEntity);
    EditorInspector::DrawBoxCollider(selectedEntity);
    EditorInspector::DrawMovementType(selectedEntity);
    EditorInspector::DrawAnimation(selectedEntity);
    DrawProjectComponents(selectedEntity, componentRegistry);

    runtimeDebugInspector.Draw(mode, selectedEntity, tileMap, fieldGrids);

    ImGui::End();
}

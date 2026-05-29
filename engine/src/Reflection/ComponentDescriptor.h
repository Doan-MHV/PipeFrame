#ifndef PIPEFRAME_COMPONENTDESCRIPTOR_H
#define PIPEFRAME_COMPONENTDESCRIPTOR_H

#include <memory>
#include <string>

#include "Reflection/EditorMetadata.h"

template <typename TComponent>
class ComponentDescriptor
{
public:
    virtual ~ComponentDescriptor() = default;

    ComponentMetadata CreateMetadata(
        std::shared_ptr<const ComponentDescriptor<TComponent>> descriptor
    ) const
    {
        ComponentMetadata metadata;
        metadata.typeName = GetTypeName();
        metadata.displayName = GetDisplayName();
        metadata.isEngineComponent = IsEngineComponent();
        metadata.editorAddable = IsEditorAddable();
        metadata.editorRemovable = IsEditorRemovable();
        metadata.editorInspectable = IsEditorInspectable();
        metadata.metadataSerializable = IsSerializable();
        metadata.hasComponent = [](Entity entity)
        {
            return entity.HasComponent<TComponent>();
        };
        metadata.addDefaultComponent = [descriptor](Entity entity)
        {
            descriptor->AddDefaultComponent(entity);
        };
        metadata.removeComponent = [descriptor](Entity entity)
        {
            descriptor->RemoveComponent(entity);
        };
        metadata.serializeComponent = [descriptor](Entity entity)
        {
            return descriptor->SerializeComponent(entity);
        };
        metadata.applyComponent = [descriptor](Entity entity, const nlohmann::json& componentJson)
        {
            descriptor->ApplyComponent(entity, componentJson);
        };
        metadata.properties = GetProperties();
        return metadata;
    }

protected:
    virtual std::string GetTypeName() const = 0;
    virtual std::string GetDisplayName() const = 0;
    virtual bool IsEngineComponent() const { return false; }
    virtual bool IsEditorAddable() const { return true; }
    virtual bool IsEditorRemovable() const { return true; }
    virtual bool IsEditorInspectable() const { return true; }
    virtual bool IsSerializable() const { return true; }

    virtual std::vector<PropertyMetadata> GetProperties() const = 0;

    virtual void AddDefaultComponent(Entity entity) const
    {
        if (!entity.HasComponent<TComponent>())
        {
            entity.AddComponent<TComponent>();
        }
    }

    virtual void RemoveComponent(Entity entity) const
    {
        if (entity.HasComponent<TComponent>())
        {
            entity.RemoveComponent<TComponent>();
        }
    }

    virtual nlohmann::json SerializeComponent(Entity entity) const = 0;

    virtual void ApplyComponent(Entity entity, const nlohmann::json& componentJson) const = 0;

    nlohmann::json SerializeProperties(Entity entity) const
    {
        nlohmann::json componentJson = nlohmann::json::object();

        for (const PropertyMetadata& property : GetProperties())
        {
            if (!property.serializable)
            {
                continue;
            }

            if (property.serializeProperty)
            {
                property.serializeProperty(componentJson, entity);
            }
            else if (property.getValue)
            {
                componentJson[property.name] = PropertyValueToJson(property.getValue(entity));
            }
        }

        return componentJson;
    }

    void ApplyProperties(Entity entity, const nlohmann::json& componentJson) const
    {
        if (!componentJson.is_object())
        {
            return;
        }

        for (const PropertyMetadata& property : GetProperties())
        {
            if (!property.serializable)
            {
                continue;
            }

            if (property.applyProperty)
            {
                property.applyProperty(entity, componentJson);
                continue;
            }

            if (!property.setValue || !componentJson.contains(property.name))
            {
                continue;
            }

            std::optional<PropertyValue> value =
                PropertyValueFromJson(componentJson[property.name], property.type);
            if (value.has_value())
            {
                property.setValue(entity, *value);
            }
        }
    }
};

template <typename TComponent>
class EngineComponentDescriptor : public ComponentDescriptor<TComponent>
{
protected:
    bool IsEngineComponent() const override { return true; }
};

template <typename TDescriptor>
void RegisterComponentDescriptor(ComponentRegistry& registry)
{
    auto descriptor = std::make_shared<TDescriptor>();
    registry.RegisterComponent(descriptor->CreateMetadata(descriptor));
}

#endif // PIPEFRAME_COMPONENTDESCRIPTOR_H

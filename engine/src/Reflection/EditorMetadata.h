#ifndef PIPEFRAME_EDITORMETADATA_H
#define PIPEFRAME_EDITORMETADATA_H

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

#include "ECS/Entity.h"

class Registry;

enum class PropertyType
{
    Int,
    Float,
    Double,
    Bool,
    String,
    Vec2,
    Rect,
    Color,
    Json,
    Enum
};

enum class PropertyVisibility
{
    Edit,
    ReadOnly,
    Hidden
};

enum class PropertyStorage
{
    Save,
    RuntimeOnly
};

using PropertyValue = std::variant<int, float, double, bool, std::string, glm::vec2, SDL_FRect, SDL_Color, nlohmann::json>;

struct PropertyMetadata
{
    std::string name;
    std::string displayName;
    PropertyType type = PropertyType::Int;
    double min = 0.0;
    double max = 0.0;
    double step = 1.0;
    bool hasMin = false;
    bool hasMax = false;
    bool visible = true;
    bool editable = true;
    bool serializable = true;
    std::vector<std::string> options;
    std::function<PropertyValue(Entity)> getValue;
    std::function<void(Entity, const PropertyValue&)> setValue;
    std::function<void(nlohmann::json&, Entity)> serializeProperty;
    std::function<void(Entity, const nlohmann::json&)> applyProperty;
};

struct ComponentMetadata
{
    std::string typeName;
    std::string displayName;
    bool isEngineComponent = false;
    bool editorAddable = true;
    bool editorRemovable = true;
    bool editorInspectable = true;
    bool metadataSerializable = true;
    std::function<bool(Entity)> hasComponent;
    std::function<void(Entity)> addDefaultComponent;
    std::function<void(Entity)> removeComponent;
    std::function<nlohmann::json(Entity)> serializeComponent;
    std::function<void(Entity, const nlohmann::json&)> applyComponent;
    std::vector<PropertyMetadata> properties;
};

struct EntityClassMetadata
{
    std::string typeName;
    std::string displayName;
    std::string category;
    std::function<Entity(Registry&, glm::vec2)> create;
};

class ComponentRegistry
{
private:
    std::unordered_map<std::string, ComponentMetadata> components;

public:
    void RegisterComponent(ComponentMetadata metadata)
    {
        components[metadata.typeName] = std::move(metadata);
    }

    void Clear()
    {
        components.clear();
    }

    const std::unordered_map<std::string, ComponentMetadata>& GetComponents() const
    {
        return components;
    }
};

class ClassRegistry
{
private:
    std::vector<EntityClassMetadata> entityClasses;

public:
    void RegisterEntityClass(EntityClassMetadata metadata)
    {
        entityClasses.push_back(std::move(metadata));
    }

    Entity CreateEntity(const std::string& typeName, Registry& registry, glm::vec2 position) const
    {
        for (const EntityClassMetadata& metadata : entityClasses)
        {
            if (metadata.typeName == typeName && metadata.create)
            {
                return metadata.create(registry, position);
            }
        }

        return Entity(-1);
    }

    void Clear()
    {
        entityClasses.clear();
    }

    const std::vector<EntityClassMetadata>& GetEntityClasses() const
    {
        return entityClasses;
    }
};

inline nlohmann::json PropertyValueToJson(const PropertyValue& value)
{
    return std::visit(
        [](const auto& typedValue) -> nlohmann::json
        {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, glm::vec2>)
            {
                return nlohmann::json{{"x", typedValue.x}, {"y", typedValue.y}};
            }
            else if constexpr (std::is_same_v<ValueType, SDL_FRect>)
            {
                return nlohmann::json{
                    {"x", typedValue.x},
                    {"y", typedValue.y},
                    {"w", typedValue.w},
                    {"h", typedValue.h}
                };
            }
            else if constexpr (std::is_same_v<ValueType, SDL_Color>)
            {
                return nlohmann::json{
                    {"r", typedValue.r},
                    {"g", typedValue.g},
                    {"b", typedValue.b},
                    {"a", typedValue.a}
                };
            }
            else if constexpr (std::is_same_v<ValueType, nlohmann::json>)
            {
                return typedValue;
            }
            else
            {
                return typedValue;
            }
        },
        value
    );
}

inline std::optional<PropertyValue> PropertyValueFromJson(
    const nlohmann::json& valueJson,
    PropertyType propertyType
)
{
    try
    {
        switch (propertyType)
        {
        case PropertyType::Int:
            return valueJson.get<int>();
        case PropertyType::Float:
            return valueJson.get<float>();
        case PropertyType::Double:
            return valueJson.get<double>();
        case PropertyType::Bool:
            return valueJson.get<bool>();
        case PropertyType::String:
            return valueJson.get<std::string>();
        case PropertyType::Enum:
            return valueJson.get<std::string>();
        case PropertyType::Vec2:
            if (valueJson.is_object())
            {
                return glm::vec2(
                    valueJson.value("x", 0.0f),
                    valueJson.value("y", 0.0f)
                );
            }
            return std::nullopt;
        case PropertyType::Rect:
            if (valueJson.is_object())
            {
                return SDL_FRect{
                    valueJson.value("x", 0.0f),
                    valueJson.value("y", 0.0f),
                    valueJson.value("w", 0.0f),
                    valueJson.value("h", 0.0f)
                };
            }
            return std::nullopt;
        case PropertyType::Color:
            if (valueJson.is_object())
            {
                return SDL_Color{
                    static_cast<Uint8>(valueJson.value("r", 255)),
                    static_cast<Uint8>(valueJson.value("g", 255)),
                    static_cast<Uint8>(valueJson.value("b", 255)),
                    static_cast<Uint8>(valueJson.value("a", 255))
                };
            }
            return std::nullopt;
        case PropertyType::Json:
            return PropertyValue(valueJson);
        }
    }
    catch (const nlohmann::json::exception&)
    {
        return std::nullopt;
    }

    return std::nullopt;
}

inline nlohmann::json SerializeRegisteredComponents(
    Entity entity,
    const ComponentRegistry& componentRegistry
)
{
    nlohmann::json componentsJson = nlohmann::json::object();

    for (const auto& [typeName, component] : componentRegistry.GetComponents())
    {
        if (!component.metadataSerializable)
        {
            continue;
        }

        if (!component.hasComponent || !component.hasComponent(entity))
        {
            continue;
        }

        if (component.serializeComponent)
        {
            componentsJson[typeName] = component.serializeComponent(entity);
            continue;
        }

        nlohmann::json componentJson = nlohmann::json::object();
        for (const PropertyMetadata& property : component.properties)
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

        componentsJson[typeName] = componentJson;
    }

    return componentsJson;
}

inline void ApplyRegisteredComponents(
    Entity entity,
    const nlohmann::json& componentsJson,
    const ComponentRegistry& componentRegistry
)
{
    if (!componentsJson.is_object())
    {
        return;
    }

    for (const auto& [typeName, component] : componentRegistry.GetComponents())
    {
        if (!component.metadataSerializable)
        {
            continue;
        }

        if (!componentsJson.contains(typeName))
        {
            continue;
        }

        if (component.addDefaultComponent)
        {
            component.addDefaultComponent(entity);
        }

        if (component.applyComponent)
        {
            component.applyComponent(entity, componentsJson[typeName]);
            continue;
        }

        for (const PropertyMetadata& property : component.properties)
        {
            if (!property.serializable)
            {
                continue;
            }

            if (property.applyProperty)
            {
                property.applyProperty(entity, componentsJson[typeName]);
                continue;
            }

            if (!property.setValue || !componentsJson[typeName].contains(property.name))
            {
                continue;
            }

            std::optional<PropertyValue> value =
                PropertyValueFromJson(componentsJson[typeName][property.name], property.type);
            if (value.has_value())
            {
                property.setValue(entity, *value);
            }
        }
    }
}

inline void ApplyPropertyFlags(
    PropertyMetadata& metadata,
    PropertyVisibility visibility,
    PropertyStorage storage
);

template <typename TComponent>
PropertyMetadata RectProperty(
    std::string name,
    std::string displayName,
    SDL_FRect TComponent::* member,
    float min,
    float max,
    float step = 1.0f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Rect,
        .min = static_cast<double>(min),
        .max = static_cast<double>(max),
        .step = static_cast<double>(step),
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const SDL_FRect* rectValue = std::get_if<SDL_FRect>(&value))
        {
            entity.GetComponent<TComponent>().*member = *rectValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata SplitRectProperty(
    std::string namePrefix,
    std::string displayName,
    SDL_FRect TComponent::* member,
    float min,
    float max,
    float step = 1.0f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata = RectProperty<TComponent>(
        namePrefix,
        std::move(displayName),
        member,
        min,
        max,
        step,
        visibility,
        storage
    );
    const std::string prefix = metadata.name;
    metadata.serializeProperty = [member, prefix](nlohmann::json& componentJson, Entity entity)
    {
        const SDL_FRect& rect = entity.GetComponent<TComponent>().*member;
        componentJson[prefix + "_x"] = rect.x;
        componentJson[prefix + "_y"] = rect.y;
        componentJson[prefix + "_w"] = rect.w;
        componentJson[prefix + "_h"] = rect.h;
    };
    metadata.applyProperty = [member, prefix](Entity entity, const nlohmann::json& componentJson)
    {
        if (!componentJson.is_object())
        {
            return;
        }

        SDL_FRect& rect = entity.GetComponent<TComponent>().*member;
        rect.x = componentJson.value(prefix + "_x", rect.x);
        rect.y = componentJson.value(prefix + "_y", rect.y);
        rect.w = componentJson.value(prefix + "_w", componentJson.value(prefix + "_width", rect.w));
        rect.h = componentJson.value(prefix + "_h", componentJson.value(prefix + "_height", rect.h));
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata JsonObjectProperty(
    std::string name,
    std::string displayName,
    nlohmann::json TComponent::* member,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Json
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        const nlohmann::json& value = entity.GetComponent<TComponent>().*member;
        return value.is_object() ? value : nlohmann::json::object();
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const nlohmann::json* jsonValue = std::get_if<nlohmann::json>(&value))
        {
            entity.GetComponent<TComponent>().*member =
                jsonValue->is_object() ? *jsonValue : nlohmann::json::object();
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata ColorProperty(
    std::string name,
    std::string displayName,
    SDL_Color TComponent::* member,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Color
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const SDL_Color* colorValue = std::get_if<SDL_Color>(&value))
        {
            entity.GetComponent<TComponent>().*member = *colorValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata RootJsonObjectProperty(
    std::string name,
    std::string displayName,
    nlohmann::json TComponent::* member,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata = JsonObjectProperty<TComponent>(
        std::move(name),
        std::move(displayName),
        member,
        visibility,
        storage
    );
    metadata.serializeProperty = [member](nlohmann::json& componentJson, Entity entity)
    {
        const nlohmann::json& value = entity.GetComponent<TComponent>().*member;
        componentJson = value.is_object() ? value : nlohmann::json::object();
    };
    metadata.applyProperty = [member](Entity entity, const nlohmann::json& componentJson)
    {
        entity.GetComponent<TComponent>().*member =
            componentJson.is_object() ? componentJson : nlohmann::json::object();
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata MillisecondsAsSecondsProperty(
    std::string name,
    std::string displayName,
    int TComponent::* member,
    float min,
    float max,
    float step = 0.1f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Float,
        .min = static_cast<double>(min),
        .max = static_cast<double>(max),
        .step = static_cast<double>(step),
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return static_cast<float>(entity.GetComponent<TComponent>().*member) / 1000.0f;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const float* seconds = std::get_if<float>(&value))
        {
            entity.GetComponent<TComponent>().*member = static_cast<int>(*seconds * 1000.0f);
        }
    };
    metadata.serializeProperty = [member, name = metadata.name](nlohmann::json& componentJson, Entity entity)
    {
        componentJson[name] = static_cast<float>(entity.GetComponent<TComponent>().*member) / 1000.0f;
    };
    metadata.applyProperty = [member, name = metadata.name](Entity entity, const nlohmann::json& componentJson)
    {
        if (!componentJson.is_object() || !componentJson.contains(name))
        {
            return;
        }

        entity.GetComponent<TComponent>().*member =
            static_cast<int>(componentJson.value(name, 0.0f) * 1000.0f);
    };
    return metadata;
}

inline void ApplyPropertyFlags(
    PropertyMetadata& metadata,
    PropertyVisibility visibility,
    PropertyStorage storage
)
{
    metadata.visible = visibility != PropertyVisibility::Hidden;
    metadata.editable = visibility == PropertyVisibility::Edit;
    metadata.serializable = storage == PropertyStorage::Save;
}

inline PropertyMetadata IntProperty(
    std::string name,
    std::string displayName,
    int min,
    int max,
    int step = 1,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Int,
        .min = static_cast<double>(min),
        .max = static_cast<double>(max),
        .step = static_cast<double>(step),
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    return metadata;
}

template <typename TComponent>
PropertyMetadata IntProperty(
    std::string name,
    std::string displayName,
    int TComponent::* member,
    int min,
    int max,
    int step = 1,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata = IntProperty(
        std::move(name),
        std::move(displayName),
        min,
        max,
        step,
        visibility,
        storage
    );
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const int* intValue = std::get_if<int>(&value))
        {
            entity.GetComponent<TComponent>().*member = *intValue;
        }
    };
    return metadata;
}

inline PropertyMetadata FloatProperty(
    std::string name,
    std::string displayName,
    float min,
    float max,
    float step = 0.01f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Float,
        .min = static_cast<double>(min),
        .max = static_cast<double>(max),
        .step = static_cast<double>(step),
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    return metadata;
}

template <typename TComponent>
PropertyMetadata FloatProperty(
    std::string name,
    std::string displayName,
    float TComponent::* member,
    float min,
    float max,
    float step = 0.01f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata = FloatProperty(
        std::move(name),
        std::move(displayName),
        min,
        max,
        step,
        visibility,
        storage
    );
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const float* floatValue = std::get_if<float>(&value))
        {
            entity.GetComponent<TComponent>().*member = *floatValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata DoubleProperty(
    std::string name,
    std::string displayName,
    double TComponent::* member,
    double min,
    double max,
    double step = 0.01,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Double,
        .min = min,
        .max = max,
        .step = step,
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const double* doubleValue = std::get_if<double>(&value))
        {
            entity.GetComponent<TComponent>().*member = *doubleValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata BoolProperty(
    std::string name,
    std::string displayName,
    bool TComponent::* member,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Bool
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const bool* boolValue = std::get_if<bool>(&value))
        {
            entity.GetComponent<TComponent>().*member = *boolValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata BoolProperty(
    std::string name,
    std::string displayName,
    bool TComponent::* member,
    double,
    double,
    double,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    return BoolProperty<TComponent>(
        std::move(name),
        std::move(displayName),
        member,
        visibility,
        storage
    );
}

template <typename TComponent>
PropertyMetadata StringProperty(
    std::string name,
    std::string displayName,
    std::string TComponent::* member,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::String
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const std::string* stringValue = std::get_if<std::string>(&value))
        {
            entity.GetComponent<TComponent>().*member = *stringValue;
        }
    };
    return metadata;
}

template <typename TComponent>
PropertyMetadata StringProperty(
    std::string name,
    std::string displayName,
    std::string TComponent::* member,
    double,
    double,
    double,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    return StringProperty<TComponent>(
        std::move(name),
        std::move(displayName),
        member,
        visibility,
        storage
    );
}

template <typename TComponent>
PropertyMetadata Vec2Property(
    std::string name,
    std::string displayName,
    glm::vec2 TComponent::* member,
    float min,
    float max,
    float step = 0.01f,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Vec2,
        .min = static_cast<double>(min),
        .max = static_cast<double>(max),
        .step = static_cast<double>(step),
        .hasMin = true,
        .hasMax = true
    };
    ApplyPropertyFlags(metadata, visibility, storage);
    metadata.getValue = [member](Entity entity) -> PropertyValue
    {
        return entity.GetComponent<TComponent>().*member;
    };
    metadata.setValue = [member](Entity entity, const PropertyValue& value)
    {
        if (const glm::vec2* vecValue = std::get_if<glm::vec2>(&value))
        {
            entity.GetComponent<TComponent>().*member = *vecValue;
        }
    };
    return metadata;
}

template <typename TComponent, typename TEnum>
PropertyMetadata EnumProperty(
    std::string name,
    std::string displayName,
    TEnum TComponent::* member,
    std::vector<std::string> options,
    PropertyVisibility visibility = PropertyVisibility::Edit,
    PropertyStorage storage = PropertyStorage::Save
)
{
    PropertyMetadata metadata{
        .name = std::move(name),
        .displayName = std::move(displayName),
        .type = PropertyType::Enum,
        .options = std::move(options)
    };
    ApplyPropertyFlags(metadata, visibility, storage);

    metadata.getValue = [member, options = metadata.options](Entity entity) -> PropertyValue
    {
        using UnderlyingType = std::underlying_type_t<TEnum>;
        const auto index = static_cast<size_t>(
            static_cast<UnderlyingType>(entity.GetComponent<TComponent>().*member)
        );

        if (index < options.size())
        {
            return options[index];
        }

        return std::string{};
    };

    metadata.setValue = [member, options = metadata.options](Entity entity, const PropertyValue& value)
    {
        const std::string* stringValue = std::get_if<std::string>(&value);
        if (!stringValue)
        {
            return;
        }

        for (size_t index = 0; index < options.size(); ++index)
        {
            if (options[index] == *stringValue)
            {
                entity.GetComponent<TComponent>().*member = static_cast<TEnum>(index);
                return;
            }
        }
    };

    return metadata;
}

#endif // PIPEFRAME_EDITORMETADATA_H

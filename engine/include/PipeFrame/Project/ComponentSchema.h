#ifndef PIPEFRAME_COMPONENT_SCHEMA_H
#define PIPEFRAME_COMPONENT_SCHEMA_H

#include <PipeFrame/Project/ProjectTypes.h>

#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace pipeframe {

// One typed field declaration supplies Inspector metadata, serialization and
// validated assignment. No offsets, casts or project-specific property readers.
template <typename Component>
class ComponentSchema {
public:
    ComponentSchema(std::string id, std::string name) : descriptor{std::move(id), std::move(name)} {}

    template <typename Value>
    ComponentSchema& Field(PropertyDescriptor property, Value Component::* member) {
        using Stored = std::conditional_t<std::is_same_v<Value, float>, double, Value>;
        if (!std::holds_alternative<Stored>(property.defaultValue) ||
            !IsPropertyValueCompatible(property.kind, property.defaultValue))
            throw std::invalid_argument("Component field metadata must match its member type");
        if (std::ranges::any_of(descriptor.properties, [&](const auto& field) { return field.key == property.key; }))
            throw std::invalid_argument("Duplicate component field ID");
        if constexpr (std::is_same_v<Value, float>) {
            const double limit = std::numeric_limits<float>::max();
            property.minimum = std::max(property.minimum.value_or(-limit), -limit);
            property.maximum = std::min(property.maximum.value_or(limit), limit);
        }
        bindings.push_back(
            {property,
             [member](const Component& object) -> PropertyValue { return static_cast<Stored>(object.*member); },
             [member](Component& object, const PropertyValue& value) {
                 object.*member = static_cast<Value>(std::get<Stored>(value));
             }});
        descriptor.properties.push_back(std::move(property));
        return *this;
    }
    // Explicit authoring permissions. Unlisted members are not exposed or serialized.
    template <typename Value>
    ComponentSchema& Editable(PropertyDescriptor property, Value Component::* member) {
        property.editable = true;
        return Field(std::move(property), member);
    }
    template <typename Value>
    ComponentSchema& ReadOnly(PropertyDescriptor property, Value Component::* member) {
        property.editable = false;
        return Field(std::move(property), member);
    }
    // Runs against the candidate after built-in field validation and assignment,
    // before publishing it. Use for cross-field/domain rules; no world mutation.
    ComponentSchema& Validate(std::string message, std::function<bool(const Component&)> predicate) {
        if (message.empty() || !predicate) throw std::invalid_argument("Validation requires a message and predicate");
        validators.emplace_back(std::move(message), std::move(predicate));
        return *this;
    }
    // Project-specific representation conversions live in the schema, not the Inspector.
    ComponentSchema& Accessor(PropertyDescriptor property, std::function<PropertyValue(const Component&)> read,
                              std::function<void(Component&, const PropertyValue&)> write) {
        if (!read || !write || !IsPropertyValueCompatible(property.kind, property.defaultValue) ||
            std::ranges::any_of(descriptor.properties, [&](const auto& field) { return field.key == property.key; }))
            throw std::invalid_argument("Invalid or duplicate component accessor");
        bindings.push_back({property, std::move(read), std::move(write)});
        descriptor.properties.push_back(std::move(property));
        return *this;
    }
    ComponentSchema& ReadOnly(PropertyDescriptor property, std::function<PropertyValue(const Component&)> read) {
        property.editable = false;
        return Accessor(std::move(property), std::move(read), [](Component&, const PropertyValue&) {});
    }
    ComponentSchema& Required() {
        descriptor.removable = false;
        return *this;
    }
    const SceneComponentTypeDescriptor& Describe() const { return descriptor; }

    SceneComponentData Serialize(const Component& object) const {
        SceneComponentData result{descriptor.typeId, descriptor.schemaVersion};
        for (const auto& binding : bindings)
            result.properties.emplace(binding.property.key, binding.read(object));
        return result;
    }

    bool Apply(Component& object, const PropertyMap& values, std::string& error) const {
        for (const auto& [key, value] : values) {
            if (std::ranges::none_of(bindings, [&](const auto& binding) { return binding.property.key == key; })) {
                error = "Unknown component field: " + key;
                return false;
            }
        }
        // Validate every supplied field before changing any member.
        for (const auto& binding : bindings) {
            const auto found = values.find(binding.property.key);
            if (found != values.end() && !ValidatePropertyValue(binding.property, found->second, &error)) {
                error = binding.property.displayName + ": " + error;
                return false;
            }
        }
        Component next = object;
        for (const auto& binding : bindings) {
            const auto found = values.find(binding.property.key);
            if (found != values.end()) {
                try {
                    binding.write(next, found->second);
                } catch (const std::exception& exception) {
                    error = binding.property.displayName + ": " + exception.what();
                    return false;
                }
            }
        }
        for (const auto& [message, predicate] : validators) {
            try {
                if (!predicate(next)) {
                    error = message;
                    return false;
                }
            } catch (const std::exception&) {
                error = message;
                return false;
            }
        }
        object = std::move(next);
        error.clear();
        return true;
    }

private:
    struct Binding {
        PropertyDescriptor property;
        std::function<PropertyValue(const Component&)> read;
        std::function<void(Component&, const PropertyValue&)> write;
    };
    SceneComponentTypeDescriptor descriptor;
    std::vector<Binding> bindings;
    std::vector<std::pair<std::string, std::function<bool(const Component&)>>> validators;
};
}  // namespace pipeframe
#endif

#ifndef PIPEFRAME_PROJECT_TYPES_H
#define PIPEFRAME_PROJECT_TYPES_H

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe {

using SceneObjectId = std::uint64_t;
using SceneObjectTypeId = std::string;

enum class PropertyKind : std::uint8_t {
    Boolean = 0,
    Integer = 1,
    Number = 2,
    String = 3,
    Vector2 = 4,
    Color = 5,
    AssetReference = 6,
    ObjectReference = 7,
    Enum = 8,
};

struct AssetReference {
    std::string assetId;
    bool operator==(const AssetReference &) const = default;
};

struct SceneObjectReference {
    SceneObjectId objectId{};
    bool operator==(const SceneObjectReference &) const = default;
};

// A scene object can participate in more than one prefab instance when a
// prefab contains another prefab. Links are ordered from the innermost prefab
// to the outermost prefab. sourceObjectId is stable inside the prefab source;
// instanceRootId identifies one concrete instance in the scene.
struct PrefabInstanceLink {
    std::string prefabId;
    std::uint32_t sourceRevision{1};
    SceneObjectId sourceObjectId{};
    SceneObjectId instanceRootId{};
    std::string variantId;
    bool operator==(const PrefabInstanceLink &) const = default;
};

using PropertyValue = std::variant<bool, std::int64_t, double, std::string, Vector2f,
                                   Color, AssetReference, SceneObjectReference>;

using PropertyMap = std::unordered_map<std::string, PropertyValue>;

struct PropertyDescriptor {
    std::string key;
    std::string displayName;
    PropertyKind kind = PropertyKind::Number;
    PropertyValue defaultValue = 0.0;
    bool editable = true;
    std::string unit;
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> step;
    std::vector<std::string> enumOptions;
    std::string editorHint;
};

inline bool IsPropertyValueCompatible(const PropertyKind kind, const PropertyValue &value) {
    switch (kind) {
    case PropertyKind::Boolean: return std::holds_alternative<bool>(value);
    case PropertyKind::Integer: return std::holds_alternative<std::int64_t>(value);
    case PropertyKind::Number: return std::holds_alternative<double>(value);
    case PropertyKind::String:
    case PropertyKind::Enum: return std::holds_alternative<std::string>(value);
    case PropertyKind::Vector2: return std::holds_alternative<Vector2f>(value);
    case PropertyKind::Color: return std::holds_alternative<Color>(value);
    case PropertyKind::AssetReference: return std::holds_alternative<AssetReference>(value);
    case PropertyKind::ObjectReference: return std::holds_alternative<SceneObjectReference>(value);
    }
    return false;
}

inline bool ValidatePropertyValue(const PropertyDescriptor &descriptor, const PropertyValue &value,
                                  std::string *error = nullptr) {
    const auto fail = [&](const char *message) { if (error) *error = message; return false; };
    if (!IsPropertyValueCompatible(descriptor.kind, value)) return fail("Property value has the wrong type.");
    if (const auto *vector=std::get_if<Vector2f>(&value);
        vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return fail("Vector must be finite.");
    if (const auto *number = std::get_if<double>(&value)) {
        if (!std::isfinite(*number)) return fail("Number must be finite.");
        if (descriptor.minimum && *number < *descriptor.minimum) return fail("Property value is below its minimum.");
        if (descriptor.maximum && *number > *descriptor.maximum) return fail("Property value is above its maximum.");
    }
    if (const auto *integer = std::get_if<std::int64_t>(&value)) {
        if (descriptor.minimum && static_cast<double>(*integer) < *descriptor.minimum) return fail("Property value is below its minimum.");
        if (descriptor.maximum && static_cast<double>(*integer) > *descriptor.maximum) return fail("Property value is above its maximum.");
    }
    if (descriptor.kind == PropertyKind::Enum &&
        std::ranges::find(descriptor.enumOptions, std::get<std::string>(value)) == descriptor.enumOptions.end())
        return fail("Property value is not a registered enum option.");
    return true;
}

struct SceneComponentData {
    std::string typeId;
    std::uint32_t schemaVersion{1};
    PropertyMap properties;
    bool enabled{true};
    bool editorOnly{false};
    bool operator==(const SceneComponentData &) const = default;
};

struct SceneAttachmentDescriptor {
    std::string id;
    std::string type;
    Transform2D localTransform{};
    std::vector<std::string> accepts;
    bool multiple{false};
};

struct SceneComponentTypeDescriptor {
    std::string typeId;
    std::string displayName;
    std::uint32_t schemaVersion{1};
    bool removable{true};
    bool editorOnly{false};
    std::vector<PropertyDescriptor> properties;
    std::vector<SceneAttachmentDescriptor> attachments;
};

struct SceneSettings {
    std::string lengthUnit{"m"};
    std::string angleUnit{"degrees"};
    std::string coordinateSystem{"right-handed-y-up"};
    bool operator==(const SceneSettings &) const = default;
};

enum class SceneConnectionKind : std::uint8_t { Mechanical, Power, Signal };

struct SceneConnectionEndpoint {
    SceneObjectId objectId{};
    std::string attachmentId;
    bool operator==(const SceneConnectionEndpoint &) const = default;
};

struct SceneConnectionData {
    std::uint64_t id{};
    SceneConnectionKind kind{SceneConnectionKind::Mechanical};
    SceneConnectionEndpoint from;
    SceneConnectionEndpoint to;
    bool operator==(const SceneConnectionData &) const = default;
};

struct SceneObjectTypeDescriptor {
    SceneObjectTypeId typeId;
    std::string displayName;
    std::vector<PropertyDescriptor> properties;
    std::vector<std::string> componentTypeIds;
};

struct SceneTransform {
    Vector2f position{0.0f, 0.0f};
    float rotation = 0.0f;
    Vector2f scale{1.0f, 1.0f};

    bool operator==(const SceneTransform &) const = default;
};

struct SceneObjectData {
    SceneObjectId id = 0;
    std::string name;
    SceneObjectTypeId typeId;
    SceneTransform transform;
    PropertyMap properties;
    SceneObjectId parentId{0};
    std::int32_t siblingOrder{0};
    std::string layer{"Default"};
    std::vector<std::string> tags;
    bool visible{true};
    bool locked{false};
    std::vector<SceneComponentData> components;
    std::vector<PrefabInstanceLink> prefabLinks;
    bool operator==(const SceneObjectData &) const = default;
};

inline constexpr const char *Transform2DComponentTypeId = "pipeframe.transform2d";

inline SceneComponentData MakeTransform2DComponent(const SceneTransform &transform = {}) {
    return {Transform2DComponentTypeId, 1,
            {{"position", transform.position}, {"rotation", static_cast<double>(transform.rotation)},
             {"scale", transform.scale}}, true, false};
}

inline void SynchronizeTransformComponent(SceneObjectData &object) {
    auto component = std::find_if(object.components.begin(), object.components.end(), [](const SceneComponentData &item) {
        return item.typeId == Transform2DComponentTypeId;
    });
    if (component == object.components.end()) {
        object.components.push_back(MakeTransform2DComponent(object.transform));
        return;
    }
    component->properties.insert_or_assign("position", object.transform.position);
    component->properties.insert_or_assign("rotation", static_cast<double>(object.transform.rotation));
    component->properties.insert_or_assign("scale", object.transform.scale);
}

// Compatibility mirror for the scene document's common Transform. Project runtimes
// use the typed schema for actual ECS storage and never dispatch on these field names.
inline void SynchronizeSceneTransform(SceneObjectData &object) {
    const auto component=std::ranges::find(object.components,std::string(Transform2DComponentTypeId),&SceneComponentData::typeId);
    if (component==object.components.end()) return;
    if (const auto found=component->properties.find("position"); found!=component->properties.end()) object.transform.position=std::get<Vector2f>(found->second);
    if (const auto found=component->properties.find("rotation"); found!=component->properties.end()) object.transform.rotation=static_cast<float>(std::get<double>(found->second));
    if (const auto found=component->properties.find("scale"); found!=component->properties.end()) object.transform.scale=std::get<Vector2f>(found->second);
}

} // namespace pipeframe

#endif

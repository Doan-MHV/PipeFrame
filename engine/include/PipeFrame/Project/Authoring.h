#ifndef PIPEFRAME_PROJECT_AUTHORING_H
#define PIPEFRAME_PROJECT_AUTHORING_H

#include <PipeFrame/Foundation/MathTypes.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace pipeframe::authoring {

using StableId = std::uint64_t;

struct AssetReference {
    std::string id;
    bool operator==(const AssetReference&) const = default;
};
struct ObjectReference {
    StableId id{};
    bool operator==(const ObjectReference&) const = default;
};
struct EnumValue {
    std::string value;
    bool operator==(const EnumValue&) const = default;
};

enum class FieldKind { Boolean, Integer, Number, String, Vector2, Color, Enum, Asset, Object, Telemetry };
using FieldValue =
    std::variant<bool, std::int64_t, double, std::string, Vector2f, Color, EnumValue, AssetReference, ObjectReference>;

struct FieldDescriptor {
    std::string serializationId;
    std::string displayName;
    FieldKind kind{FieldKind::Number};
    FieldValue defaultValue{0.0};
    std::string unit;
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> step;
    std::vector<std::string> enumOptions;
    std::string editorHint;
    bool editable{true};
};

struct AttachmentDescriptor {
    std::string id;
    std::string type;
    Transform2D localTransform{};
    std::vector<std::string> accepts;
    bool multiple{false};
};

struct ComponentDescriptor {
    std::string serializationId;
    std::string displayName;
    std::uint32_t schemaVersion{1};
    bool removable{true};
    bool editorOnly{false};
    std::vector<FieldDescriptor> fields;
    std::vector<AttachmentDescriptor> attachments;
};

class ComponentBuilder {
public:
    ComponentBuilder(std::string id, std::string name) {
        descriptor.serializationId = std::move(id);
        descriptor.displayName = std::move(name);
    }
    ComponentBuilder& Version(std::uint32_t value) {
        descriptor.schemaVersion = std::max(1u, value);
        return *this;
    }
    ComponentBuilder& Required() {
        descriptor.removable = false;
        return *this;
    }
    ComponentBuilder& EditorOnly() {
        descriptor.editorOnly = true;
        return *this;
    }
    ComponentBuilder& Field(FieldDescriptor value) {
        descriptor.fields.push_back(std::move(value));
        return *this;
    }
    ComponentBuilder& Attachment(AttachmentDescriptor value) {
        descriptor.attachments.push_back(std::move(value));
        return *this;
    }
    ComponentDescriptor Build() { return std::move(descriptor); }

private:
    ComponentDescriptor descriptor;
};

class ComponentRegistry {
public:
    bool Register(ComponentDescriptor descriptor, std::string* error = nullptr) {
        if (descriptor.serializationId.empty() || descriptor.displayName.empty())
            return Fail(error, "Component ID and name are required.");
        std::unordered_set<std::string> fields, attachments;
        for (const auto& field : descriptor.fields)
            if (field.serializationId.empty() || !fields.insert(field.serializationId).second)
                return Fail(error, "Component field IDs must be non-empty and unique.");
        for (const auto& point : descriptor.attachments)
            if (point.id.empty() || point.type.empty() || !attachments.insert(point.id).second)
                return Fail(error, "Attachment IDs must be unique and have a type.");
        return descriptors.emplace(descriptor.serializationId, std::move(descriptor)).second ||
               Fail(error, "Component ID is already registered.");
    }
    const ComponentDescriptor* Find(const std::string& id) const {
        const auto found = descriptors.find(id);
        return found == descriptors.end() ? nullptr : &found->second;
    }
    std::size_t Size() const { return descriptors.size(); }

private:
    static bool Fail(std::string* error, const char* message) {
        if (error) *error = message;
        return false;
    }
    std::unordered_map<std::string, ComponentDescriptor> descriptors;
};

enum class ConnectionKind { Mechanical, Power, Signal };
struct Endpoint {
    StableId objectId{};
    std::string attachmentId;
    bool operator==(const Endpoint&) const = default;
};
struct Connection {
    StableId id{};
    ConnectionKind kind{};
    Endpoint from;
    Endpoint to;
    bool operator==(const Connection&) const = default;
};

class ConnectionGraph {
public:
    bool Connect(Connection connection, const std::unordered_map<StableId, std::vector<AttachmentDescriptor>>& points,
                 std::string* error = nullptr) {
        if (connection.id == 0 || connection.from == connection.to)
            return Fail(error, "Connection endpoints and ID must be valid.");
        if (std::ranges::any_of(connections, [&](const auto& item) { return item.id == connection.id; }))
            return Fail(error, "Connection ID already exists.");
        const auto *from = FindPoint(points, connection.from), *to = FindPoint(points, connection.to);
        if (!from || !to) return Fail(error, "Connection references a missing attachment.");
        const auto accepts = [](const AttachmentDescriptor& a, const AttachmentDescriptor& b) {
            return a.type == b.type || std::ranges::find(a.accepts, b.type) != a.accepts.end();
        };
        if (!accepts(*from, *to) || !accepts(*to, *from)) return Fail(error, "Attachment types are incompatible.");
        if (std::ranges::any_of(connections, [&](const auto& item) {
                return item.kind == connection.kind && ((item.from == connection.from && item.to == connection.to) ||
                                                        (item.from == connection.to && item.to == connection.from));
            }))
            return Fail(error, "Connection already exists.");
        connections.push_back(std::move(connection));
        return true;
    }
    bool Remove(StableId id) {
        return std::erase_if(connections, [id](const auto& item) { return item.id == id; }) > 0;
    }
    const std::vector<Connection>& GetConnections() const { return connections; }
    void RemoveObject(StableId id) {
        std::erase_if(connections,
                      [id](const auto& item) { return item.from.objectId == id || item.to.objectId == id; });
    }

private:
    static const AttachmentDescriptor* FindPoint(
        const std::unordered_map<StableId, std::vector<AttachmentDescriptor>>& all, const Endpoint& endpoint) {
        const auto object = all.find(endpoint.objectId);
        if (object == all.end()) return nullptr;
        const auto point =
            std::ranges::find_if(object->second, [&](const auto& item) { return item.id == endpoint.attachmentId; });
        return point == object->second.end() ? nullptr : &*point;
    }
    static bool Fail(std::string* error, const char* message) {
        if (error) *error = message;
        return false;
    }
    std::vector<Connection> connections;
};

struct AssetRecord {
    std::string id, relativePath, importer, category;
    std::uint32_t importerVersion{1};
    std::vector<std::string> dependencies, tags;
    bool missing{false};
};

class AssetDatabase {
public:
    bool Upsert(AssetRecord record) {
        if (record.id.empty() || record.relativePath.empty()) return false;
        records.insert_or_assign(record.id, std::move(record));
        return true;
    }
    const AssetRecord* Find(const std::string& id) const {
        const auto it = records.find(id);
        return it == records.end() ? nullptr : &it->second;
    }
    bool Move(const std::string& id, std::string path) {
        auto it = records.find(id);
        if (it == records.end() || path.empty()) return false;
        it->second.relativePath = std::move(path);
        return true;
    }
    std::vector<std::string> Dependents(const std::string& id) const {
        std::vector<std::string> result;
        for (const auto& [key, value] : records)
            if (std::ranges::find(value.dependencies, id) != value.dependencies.end()) result.push_back(key);
        std::ranges::sort(result);
        return result;
    }
    std::size_t Size() const { return records.size(); }

private:
    std::unordered_map<std::string, AssetRecord> records;
};

struct PrefabOverride {
    StableId sourceObject{};
    std::string componentId, fieldId;
    FieldValue value{0.0};
};
struct PrefabRecord {
    std::string id;
    std::uint32_t revision{1};
    std::vector<StableId> sourceObjects;
    std::vector<PrefabOverride> overrides;
};
class PrefabLibrary {
public:
    bool Store(PrefabRecord prefab) {
        if (prefab.id.empty() || prefab.sourceObjects.empty()) return false;
        prefabs.insert_or_assign(prefab.id, std::move(prefab));
        return true;
    }
    const PrefabRecord* Find(const std::string& id) const {
        const auto it = prefabs.find(id);
        return it == prefabs.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, PrefabRecord> prefabs;
};

enum class ExtensionKind {
    Panel,
    Drawer,
    Tool,
    Gizmo,
    Menu,
    Command,
    Overlay,
    Importer,
    Setting,
    PartCategory,
    ConnectionType,
    EnvironmentBrush,
    Validator,
    Telemetry
};
struct Extension {
    std::string id, owner;
    ExtensionKind kind{};
};
class ExtensionRegistry {
public:
    bool Register(Extension value) {
        return !value.id.empty() && !value.owner.empty() && extensions.emplace(value.id, std::move(value)).second;
    }
    void RemoveOwner(const std::string& owner) {
        std::erase_if(extensions, [&](const auto& item) { return item.second.owner == owner; });
    }
    const Extension* Find(const std::string& id) const {
        const auto it = extensions.find(id);
        return it == extensions.end() ? nullptr : &it->second;
    }
    std::size_t Size() const { return extensions.size(); }

private:
    std::unordered_map<std::string, Extension> extensions;
};

struct DockPanelState {
    std::string id;
    Rectanglef bounds;
    bool visible{true};
    std::string tabGroup;
};
struct WorkspaceLayout {
    std::string name;
    std::vector<DockPanelState> panels;
    float dpiScale{1.0f};
};

}  // namespace pipeframe::authoring

// Optional Unreal-style spelling over the canonical typed descriptor API.
// Stable serialization IDs remain explicit and the macros never expose offsets.
#define PF_COMPONENT(serialization_id, display_name)                      \
    static constexpr const char* PipeFrameComponentId = serialization_id; \
    static constexpr const char* PipeFrameComponentName = display_name;   \
    static ::pipeframe::authoring::ComponentDescriptor PipeFrameDescribeComponent()

#define PF_PROPERTY(serialization_id, display_name, field_kind, ...)                     \
    ::pipeframe::authoring::FieldDescriptor {                                            \
        serialization_id, display_name, field_kind, ::pipeframe::authoring::FieldValue { \
            __VA_ARGS__                                                                  \
        }                                                                                \
    }

#endif

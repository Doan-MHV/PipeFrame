#include "SceneSerializer.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <type_traits>
#include <utility>

namespace pipeframe::editor {

namespace {

constexpr unsigned int CurrentSceneFormatVersion = 7;
constexpr unsigned int OldestSupportedSceneFormatVersion = 3;
constexpr const char *SceneFileHeader =
    "PIPEFRAME_SCENE";

void SetError(
    std::string *errorMessage,
    std::string message) {

    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

bool WriteProperty(
    std::ostream &output,
    const std::string &key,
    const PropertyValue &value) {

    output << std::quoted(key) << ' ';

    std::visit(
        [&output](const auto &typedValue) {
            using ValueType =
                std::decay_t<decltype(typedValue)>;

            if constexpr (
                std::is_same_v<ValueType, bool>) {

                output
                    << static_cast<int>(
                           PropertyKind::Boolean)
                    << ' '
                    << (typedValue ? 1 : 0);
            } else if constexpr (
                std::is_same_v<
                    ValueType,
                    std::int64_t>) {

                output
                    << static_cast<int>(
                           PropertyKind::Integer)
                    << ' '
                    << typedValue;
            } else if constexpr (
                std::is_same_v<ValueType, double>) {

                output
                    << static_cast<int>(
                           PropertyKind::Number)
                    << ' '
                    << typedValue;
            } else if constexpr (
                std::is_same_v<
                    ValueType,
                    std::string>) {

                output
                    << static_cast<int>(
                           PropertyKind::String)
                    << ' '
                    << std::quoted(typedValue);
            } else if constexpr (
                std::is_same_v<
                    ValueType,
                    pipeframe::Vector2f>) {

                output
                    << static_cast<int>(
                           PropertyKind::Vector2)
                    << ' '
                    << typedValue.x
                    << ' '
                      << typedValue.y;
            } else if constexpr (std::is_same_v<ValueType, pipeframe::Color>) {
                output << static_cast<int>(PropertyKind::Color) << ' '
                       << static_cast<int>(typedValue.red) << ' ' << static_cast<int>(typedValue.green) << ' '
                       << static_cast<int>(typedValue.blue) << ' ' << static_cast<int>(typedValue.alpha);
            } else if constexpr (std::is_same_v<ValueType, pipeframe::AssetReference>) {
                output << static_cast<int>(PropertyKind::AssetReference) << ' ' << std::quoted(typedValue.assetId);
            } else if constexpr (std::is_same_v<ValueType, pipeframe::SceneObjectReference>) {
                output << static_cast<int>(PropertyKind::ObjectReference) << ' ' << typedValue.objectId;
            }
        },
        value);

    output << '\n';

    return output.good();
}

std::optional<PropertyValue> ReadPropertyValue(
    std::istream &input,
    const int kindValue) {

    const PropertyKind kind =
        static_cast<PropertyKind>(kindValue);

    switch (kind) {
        case PropertyKind::Boolean: {
            int value = 0;

            if (!(input >> value) ||
                (value != 0 && value != 1)) {

                return std::nullopt;
            }

            return PropertyValue{value != 0};
        }

        case PropertyKind::Integer: {
            std::int64_t value = 0;

            if (!(input >> value)) {
                return std::nullopt;
            }

            return PropertyValue{value};
        }

        case PropertyKind::Number: {
            double value = 0.0;

            if (!(input >> value)) {
                return std::nullopt;
            }

            return PropertyValue{value};
        }

        case PropertyKind::String: {
            std::string value;

            if (!(input >> std::quoted(value))) {
                return std::nullopt;
            }

            return PropertyValue{std::move(value)};
        }

        case PropertyKind::Enum: {
            std::string value;
            if (!(input >> std::quoted(value))) return std::nullopt;
            return PropertyValue{std::move(value)};
        }

        case PropertyKind::Vector2: {
            pipeframe::Vector2f value;

            if (!(input >> value.x >> value.y)) {
                return std::nullopt;
            }

            return PropertyValue{value};
        }
        case PropertyKind::Color: {
            int r=0,g=0,b=0,a=0; if(!(input>>r>>g>>b>>a)||r<0||r>255||g<0||g>255||b<0||b>255||a<0||a>255)return std::nullopt;
            return PropertyValue{pipeframe::Color{static_cast<std::uint8_t>(r),static_cast<std::uint8_t>(g),static_cast<std::uint8_t>(b),static_cast<std::uint8_t>(a)}};
        }
        case PropertyKind::AssetReference: { std::string id; if(!(input>>std::quoted(id)))return std::nullopt; return PropertyValue{pipeframe::AssetReference{std::move(id)}}; }
        case PropertyKind::ObjectReference: { SceneObjectId id=0; if(!(input>>id))return std::nullopt; return PropertyValue{pipeframe::SceneObjectReference{id}}; }
    }

    return std::nullopt;
}

} // namespace

bool SceneSerializer::Save(
    const SceneDocument &document,
    const std::filesystem::path &path,
    std::string *errorMessage) {

    std::ofstream output(path);

    if (!output.is_open()) {
        SetError(
            errorMessage,
            "Could not open scene file for writing: " +
                path.string());

        return false;
    }

    output
        << SceneFileHeader
        << ' '
        << CurrentSceneFormatVersion
        << '\n';

    const auto &settings = document.GetSettings();
    output << std::quoted(settings.lengthUnit) << ' '
           << std::quoted(settings.angleUnit) << ' '
           << std::quoted(settings.coordinateSystem) << '\n';
    output << document.GetObjects().size() << ' '
           << document.GetConnections().size() << '\n';

    output << std::setprecision(
        std::numeric_limits<double>::max_digits10);

    for (const SceneObjectData &object :
         document.GetObjects()) {

        if (object.id == 0 ||
            object.typeId.empty()) {

            SetError(
                errorMessage,
                "Scene contains an invalid object.");

            return false;
        }

        output
            << object.id << ' '
            << object.transform.position.x << ' '
            << object.transform.position.y << ' '
            << object.transform.rotation << ' '
            << object.transform.scale.x << ' '
            << object.transform.scale.y << ' '
            << std::quoted(object.name) << ' '
            << std::quoted(object.typeId) << ' '
            << object.parentId << ' ' << object.siblingOrder << ' '
            << (object.visible?1:0) << ' ' << (object.locked?1:0) << ' '
            << std::quoted(object.layer) << ' '
            << object.tags.size() << ' ' << object.properties.size() << ' ' << object.components.size() << ' '
            << object.prefabLinks.size()
            << '\n';

        for (const auto &link : object.prefabLinks) {
            if (link.prefabId.empty() || link.sourceRevision == 0 ||
                link.sourceObjectId == 0 || link.instanceRootId == 0) {
                SetError(errorMessage, "Scene contains an invalid prefab instance link.");
                return false;
            }
            output << std::quoted(link.prefabId) << ' ' << link.sourceRevision << ' '
                   << link.sourceObjectId << ' ' << link.instanceRootId << ' '
                   << std::quoted(link.variantId) << '\n';
        }

        for (const auto &tag : object.tags) output << std::quoted(tag) << '\n';

        for (const auto &[key, value] :
             object.properties) {

            if (!WriteProperty(output, key, value)) {
                SetError(
                    errorMessage,
                    "Failed while writing scene property.");

                return false;
            }
        }

        for (const auto &component : object.components) {
            output << std::quoted(component.typeId) << ' ' << component.schemaVersion << ' '
                   << (component.enabled?1:0) << ' ' << (component.editorOnly?1:0) << ' '
                   << component.properties.size() << '\n';
            for (const auto &[key,value] : component.properties) if(!WriteProperty(output,key,value)) {
                SetError(errorMessage,"Failed while writing component property."); return false;
            }
        }
    }

    for (const auto &connection : document.GetConnections()) {
        output << connection.id << ' ' << static_cast<int>(connection.kind) << ' '
               << connection.from.objectId << ' ' << std::quoted(connection.from.attachmentId) << ' '
               << connection.to.objectId << ' ' << std::quoted(connection.to.attachmentId) << '\n';
    }

    if (!output.good()) {
        SetError(
            errorMessage,
            "Failed while writing scene file: " +
                path.string());

        return false;
    }

    return true;
}

std::optional<SceneDocument>
SceneSerializer::Load(
    const std::filesystem::path &path,
    std::string *errorMessage) {

    std::ifstream input(path);

    if (!input.is_open()) {
        SetError(
            errorMessage,
            "Could not open scene file: " +
                path.string());

        return std::nullopt;
    }

    std::string header;
    unsigned int version = 0;

    if (!(input >> header >> version) ||
        header != SceneFileHeader) {

        SetError(
            errorMessage,
            "Scene file header is invalid.");

        return std::nullopt;
    }

    if (version < OldestSupportedSceneFormatVersion ||
        version > CurrentSceneFormatVersion) {
        SetError(
            errorMessage,
            "Unsupported scene version. Supported versions are 3 through 7.");

        return std::nullopt;
    }

    SceneSettings settings;
    std::size_t objectCount = 0;
    std::size_t connectionCount = 0;
    if (version >= 6 && !(input >> std::quoted(settings.lengthUnit)
                               >> std::quoted(settings.angleUnit)
                               >> std::quoted(settings.coordinateSystem))) {
        SetError(errorMessage, "Scene unit and coordinate settings are invalid.");
        return std::nullopt;
    }
    if (!(input >> objectCount) || (version >= 6 && !(input >> connectionCount))) {
        SetError(
            errorMessage,
            "Scene object count is invalid.");

        return std::nullopt;
    }

    SceneDocument document;
    if (version >= 6 && settings != SceneSettings{}) document.SetSettings(std::move(settings));

    for (std::size_t objectIndex = 0;
         objectIndex < objectCount;
         ++objectIndex) {

        SceneObjectData object;
        std::size_t tagCount = 0;
        std::size_t propertyCount = 0;
        std::size_t componentCount = 0;
        std::size_t prefabLinkCount = 0;

        if (!(input
              >> object.id
              >> object.transform.position.x
              >> object.transform.position.y
              >> object.transform.rotation)) {

            SetError(
                errorMessage,
                "Scene object transform is incomplete.");

            return std::nullopt;
        }

        if (version >= 4 &&
            !(input >> object.transform.scale.x
                    >> object.transform.scale.y)) {

            SetError(
                errorMessage,
                "Scene object scale is incomplete.");

            return std::nullopt;
        }

        if (!(input >> std::quoted(object.name) >> std::quoted(object.typeId))) {

            SetError(
                errorMessage,
                "Scene object data is incomplete.");

            return std::nullopt;
        }

        if (version >= 5) {
            int visible=1,locked=0;
            if(!(input>>object.parentId>>object.siblingOrder>>visible>>locked>>std::quoted(object.layer)
                 >>tagCount>>propertyCount>>componentCount)||(visible!=0&&visible!=1)||(locked!=0&&locked!=1)) {
                SetError(errorMessage,"Scene object hierarchy data is invalid."); return std::nullopt;
            }
            if (version >= 7 && !(input >> prefabLinkCount)) {
                SetError(errorMessage, "Scene prefab link count is invalid.");
                return std::nullopt;
            }
            object.visible=visible!=0; object.locked=locked!=0;
            for (std::size_t linkIndex = 0; linkIndex < prefabLinkCount; ++linkIndex) {
                PrefabInstanceLink link;
                if (!(input >> std::quoted(link.prefabId) >> link.sourceRevision
                            >> link.sourceObjectId >> link.instanceRootId
                            >> std::quoted(link.variantId)) ||
                    link.prefabId.empty() || link.sourceRevision == 0 ||
                    link.sourceObjectId == 0 || link.instanceRootId == 0) {
                    SetError(errorMessage, "Scene prefab instance link is invalid.");
                    return std::nullopt;
                }
                object.prefabLinks.push_back(std::move(link));
            }
            for(std::size_t tagIndex=0;tagIndex<tagCount;++tagIndex){std::string tag;if(!(input>>std::quoted(tag))||tag.empty()){SetError(errorMessage,"Scene object tag is invalid.");return std::nullopt;}object.tags.push_back(std::move(tag));}
        } else if (!(input >> propertyCount)) {
            SetError(errorMessage,"Scene property count is invalid."); return std::nullopt;
        }

        if (object.id == 0 ||
            object.typeId.empty()) {

            SetError(
                errorMessage,
                "Scene object is invalid.");

            return std::nullopt;
        }

        for (std::size_t propertyIndex = 0;
             propertyIndex < propertyCount;
             ++propertyIndex) {

            std::string key;
            int kind = -1;

            if (!(input >> std::quoted(key) >> kind) ||
                key.empty()) {

                SetError(
                    errorMessage,
                    "Scene property header is invalid.");

                return std::nullopt;
            }

            std::optional<PropertyValue> value =
                ReadPropertyValue(input, kind);

            if (!value.has_value()) {
                SetError(
                    errorMessage,
                    "Scene property value is invalid.");

                return std::nullopt;
            }

            if (!object.properties.emplace(
                    std::move(key),
                    std::move(*value))
                     .second) {

                SetError(
                    errorMessage,
                    "Scene object contains duplicate "
                    "property keys.");

                return std::nullopt;
            }
        }

        for (std::size_t componentIndex = 0;
             componentIndex < componentCount;
             ++componentIndex) {
            SceneComponentData component;
            int enabled = 1;
            int editorOnly = 0;
            std::size_t count = 0;

            if (!(input >> std::quoted(component.typeId)
                        >> component.schemaVersion
                        >> enabled
                        >> editorOnly
                        >> count) ||
                component.typeId.empty() ||
                (enabled != 0 && enabled != 1) ||
                (editorOnly != 0 && editorOnly != 1)) {
                SetError(errorMessage, "Scene component header is invalid.");
                return std::nullopt;
            }

            component.enabled = enabled != 0;
            component.editorOnly = editorOnly != 0;

            for (std::size_t propertyIndex = 0;
                 propertyIndex < count;
                 ++propertyIndex) {
                std::string key;
                int kind = -1;
                if (!(input >> std::quoted(key) >> kind) || key.empty()) {
                    SetError(errorMessage, "Scene component property header is invalid.");
                    return std::nullopt;
                }
                auto value = ReadPropertyValue(input, kind);
                if (!value.has_value() ||
                    !component.properties.emplace(std::move(key), std::move(*value)).second) {
                    SetError(errorMessage, "Scene component property is invalid.");
                    return std::nullopt;
                }
            }

            object.components.push_back(std::move(component));
        }

        if (!document.RestoreObject(
                std::move(object))) {

            SetError(
                errorMessage,
                "Scene contains duplicate or invalid "
                "object IDs.");

            return std::nullopt;
        }
    }

    for (std::size_t connectionIndex = 0; connectionIndex < connectionCount; ++connectionIndex) {
        SceneConnectionData connection;
        int kind = -1;
        if (!(input >> connection.id >> kind
                    >> connection.from.objectId >> std::quoted(connection.from.attachmentId)
                    >> connection.to.objectId >> std::quoted(connection.to.attachmentId)) ||
            kind < static_cast<int>(SceneConnectionKind::Mechanical) ||
            kind > static_cast<int>(SceneConnectionKind::Signal)) {
            SetError(errorMessage, "Scene connection is invalid.");
            return std::nullopt;
        }
        connection.kind = static_cast<SceneConnectionKind>(kind);
        if (!document.AddConnection(std::move(connection))) {
            SetError(errorMessage, "Scene connection references invalid or duplicate endpoints.");
            return std::nullopt;
        }
    }

    document.MarkClean();

    const auto validationErrors=document.Validate();
    if(!validationErrors.empty()){SetError(errorMessage,validationErrors.front());return std::nullopt;}

    return document;
}

} // namespace pipeframe::editor

#pragma once
#include <PipeFrame/UI/View.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <charconv>
#include <sstream>
#include <iomanip>

namespace pipeframe::ui {
namespace detail {
inline std::string PropertyText(const PropertyValue &value) {
    return std::visit([](const auto &value) -> std::string {
        using T=std::decay_t<decltype(value)>;
        std::ostringstream out; out << std::setprecision(12);
        if constexpr (std::is_same_v<T,bool>) out << (value ? "true" : "false");
        else if constexpr (std::is_same_v<T,Vector2f>) out << value.x << ", " << value.y;
        else if constexpr (std::is_same_v<T,Color>) out << int(value.r) << ", " << int(value.g) << ", " << int(value.b) << ", " << int(value.a);
        else if constexpr (std::is_same_v<T,AssetReference>) out << value.assetId;
        else if constexpr (std::is_same_v<T,SceneObjectReference>) out << value.objectId;
        else out << value;
        return out.str();
    },value);
}
template<class T> T ReadNumber(const std::string &text) {
    T result{};
    const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if (parsed.ec != std::errc{} || parsed.ptr!=text.data()+text.size()) throw std::invalid_argument("Enter a valid number");
    if constexpr (std::is_floating_point_v<T>) if (!std::isfinite(result)) throw std::invalid_argument("Number must be finite");
    return result;
}
inline PropertyValue ParseProperty(PropertyKind kind, const std::string &text) {
    switch (kind) {
    case PropertyKind::Integer: return ReadNumber<std::int64_t>(text);
    case PropertyKind::Number: return ReadNumber<double>(text);
    case PropertyKind::ObjectReference: return SceneObjectReference{ReadNumber<SceneObjectId>(text)};
    case PropertyKind::AssetReference: return AssetReference{text};
    case PropertyKind::Boolean:
        if (text=="true") return true;
        if (text=="false") return false;
        throw std::invalid_argument("Enter true or false");
    case PropertyKind::Vector2: case PropertyKind::Color: {
        std::istringstream stream(text); char comma{};
        if (kind==PropertyKind::Vector2) {
            Vector2f result;
            if (!(stream>>result.x>>comma) || comma!=',' || !(stream>>result.y) ||
                !std::isfinite(result.x) || !std::isfinite(result.y)) throw std::invalid_argument("Enter x, y");
            stream>>std::ws; if (!stream.eof()) throw std::invalid_argument("Enter x, y");
            return result;
        }
        int channels[4];
        for (int i=0; i<4; ++i) {
            if (i && (!(stream>>comma) || comma!=',')) throw std::invalid_argument("Enter r, g, b, a (0–255)");
            if (!(stream>>channels[i]) || channels[i]<0 || channels[i]>255) throw std::invalid_argument("Enter r, g, b, a (0–255)");
        }
        stream>>std::ws; if (!stream.eof()) throw std::invalid_argument("Enter r, g, b, a (0–255)");
        return Color(channels[0],channels[1],channels[2],channels[3]);
    }
    default: return text;
    }
}
}
using PropertyCommit=std::function<void(const std::string &,const PropertyValue &)>;
using PropertyError=std::function<void(const std::string &,const std::string &)>;
// The caller owns the edit transaction (e.g. schema.Apply or editor undo command).
// Reference IDs are editable text here; asset/object pickers belong to editor integration.
inline View SchemaInspector(std::string key, const SceneComponentTypeDescriptor &schema,
                            const PropertyMap &values, PropertyCommit commit, PropertyError error={}) {
    std::vector<View> rows{views::Text("title",schema.displayName)};
    for (const auto &property : schema.properties) {
        const auto found=values.find(property.key);
        const auto value=found==values.end() ? property.defaultValue : found->second;
        if (!IsPropertyValueCompatible(property.kind,value)) throw std::invalid_argument("Inspector value does not match its schema");
        auto assign=[property,commit,error](const PropertyValue &value) {
            if (!property.editable) return;
            std::string message;
            if (!ValidatePropertyValue(property,value,&message)) { if (error) error(property.key,message); return; }
            if (commit) commit(property.key,value);
        };
        View control=views::Input("value",detail::PropertyText(value),[property,assign,error](const std::string &text) {
            PropertyValue parsed;
            try { parsed=detail::ParseProperty(property.kind,text); }
            catch (const std::invalid_argument &exception) { if (error) error(property.key,exception.what()); return; }
            assign(parsed);
        });
        if (property.kind==PropertyKind::Boolean)
            control=views::Toggle("value","Enabled",std::get<bool>(value),[assign](bool value) { assign(value); });
        control.Enabled(property.editable);
        auto label=property.displayName + (property.unit.empty() ? "" : " ("+property.unit+")");
        // Vertical field groups fit narrow Inspector docks without truncating labels.
        rows.push_back(views::Column(property.key, {views::Text("label",label).FitHeight(),std::move(control)}).Spacing(3));
    }
    return views::Column(std::move(key),std::move(rows)).Spacing(10).Padding(8);
}
}

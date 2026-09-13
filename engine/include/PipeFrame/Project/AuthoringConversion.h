#pragma once
#include <concepts>
#include <PipeFrame/Project/Authoring.h>
#include <PipeFrame/Project/ProjectTypes.h>

namespace pipeframe {
inline pipeframe::PropertyKind ToPropertyKind(const authoring::FieldKind value) {
    using K=pipeframe::PropertyKind;
    switch(value){
      case authoring::FieldKind::Boolean:return K::Boolean; case authoring::FieldKind::Integer:return K::Integer;
      case authoring::FieldKind::Number:case authoring::FieldKind::Telemetry:return K::Number;
      case authoring::FieldKind::String:return K::String; case authoring::FieldKind::Vector2:return K::Vector2;
      case authoring::FieldKind::Color:return K::Color; case authoring::FieldKind::Enum:return K::Enum;
      case authoring::FieldKind::Asset:return K::AssetReference; case authoring::FieldKind::Object:return K::ObjectReference;
    }
    return K::String;
}
inline pipeframe::PropertyValue ToPropertyValue(const pipeframe::authoring::FieldValue &value) {
    return std::visit([](const auto &v)->pipeframe::PropertyValue {
      using T=std::decay_t<decltype(v)>;
      if constexpr(std::same_as<T,pipeframe::authoring::EnumValue>)return v.value;
      else if constexpr(std::same_as<T,pipeframe::authoring::AssetReference>)return pipeframe::AssetReference{v.id};
      else if constexpr(std::same_as<T,pipeframe::authoring::ObjectReference>)return pipeframe::SceneObjectReference{v.id};
      else return v;
    },value);
}
inline pipeframe::SceneComponentTypeDescriptor ToSceneComponent(pipeframe::authoring::ComponentDescriptor source) {
    pipeframe::SceneComponentTypeDescriptor result{source.serializationId,source.displayName,
      source.schemaVersion,source.removable,source.editorOnly};
    for(auto &field:source.fields)result.properties.push_back({field.serializationId,field.displayName,
      ToPropertyKind(field.kind),ToPropertyValue(field.defaultValue),field.editable,field.unit,field.minimum,field.maximum,
      field.step,field.enumOptions,field.editorHint});
    for (const auto &attachment : source.attachments)
        result.attachments.push_back({attachment.id, attachment.type, attachment.localTransform,
                                      attachment.accepts, attachment.multiple});
    return result;
}
} // namespace pipeframe

#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
#include <PipeFrame/ECS/Scene.h>
#include <span>
#include <typeindex>

namespace pipeframe {
struct ComponentMutation {
    SceneObject object;
    std::string typeId;
    PropertyMap values;
};
// One typed registration joins actual ECS presence, schemas, serialization and edits.
// No raw component pointers survive a call; SceneObject validates lifetime/incarnation.
class ComponentRegistry {
    struct Entry {
        std::type_index type;
        SceneComponentTypeDescriptor descriptor;
        std::function<bool(SceneObject)> present;
        std::function<SceneComponentData(SceneObject)> serialize;
        std::function<bool(SceneObject,const PropertyMap &,std::function<void()> &,std::string &)> prepare;
    };
    std::vector<Entry> entries;
public:
    template<class T> void Register(ComponentSchema<T> schema) {
        const auto descriptor=schema.Describe();
        if (descriptor.typeId.empty() || std::ranges::any_of(entries,[&](const auto &entry) {
            return entry.type==std::type_index(typeid(T)) || entry.descriptor.typeId==descriptor.typeId;
        })) throw std::invalid_argument("Duplicate or empty typed component registration");
        entries.push_back({typeid(T),descriptor,
            [](SceneObject object) { return object.IsValid() && object.GetComponent<T>(); },
            [schema](SceneObject object) { return schema.Serialize(*object.GetComponent<T>()); },
            [schema](SceneObject object,const PropertyMap &values,std::function<void()> &commit,std::string &error) {
                const auto *current=object.GetComponent<T>();
                T next=current ? *current : T{};
                if (!current) {
                    PropertyMap defaults;
                    for (const auto &field : schema.Describe().properties) defaults.emplace(field.key,field.defaultValue);
                    if (!schema.Apply(next,defaults,error)) return false;
                }
                if (!schema.Apply(next,values,error)) return false;
                commit=[object,next=std::move(next)] {
                    if (auto *current=object.GetComponent<T>()) *current=next;
                    else object.AddComponent<T>(next);
                };
                return true;
            }});
    }
    std::vector<SceneComponentTypeDescriptor> Describe() const {
        std::vector<SceneComponentTypeDescriptor> result;
        for (const auto &entry : entries) result.push_back(entry.descriptor);
        return result;
    }
    std::vector<SceneComponentData> Inspect(SceneObject object) const {
        std::vector<SceneComponentData> result;
        if (!object.IsValid()) return result;
        for (const auto &entry : entries) if (entry.present(object)) result.push_back(entry.serialize(object));
        return result;
    }
    bool Contains(const std::string &typeId) const {
        return std::ranges::any_of(entries,[&](const auto &entry) { return entry.descriptor.typeId==typeId; });
    }
    bool Apply(std::span<const ComponentMutation> mutations,std::string &error,bool editing=true,bool validateOnly=false) const {
        // Callers combine fields into one mutation per entity/component. Reject ambiguity.
        std::vector<std::function<void()>> commits;
        for (std::size_t i=0;i<mutations.size();++i) {
            const auto &mutation=mutations[i];
            if (!mutation.object.IsValid()) { error="Selected entity no longer exists"; return false; }
            for (std::size_t j=0;j<i;++j)
                if (mutations[j].object==mutation.object && mutations[j].typeId==mutation.typeId) {
                    error="Duplicate component mutation"; return false;
                }
            const auto entry=std::ranges::find(entries,mutation.typeId,[](const Entry &entry) { return entry.descriptor.typeId; });
            if (entry==entries.end() || (editing && !entry->present(mutation.object))) { error="Component is not attached/registered"; return false; }
            for (const auto &[key,value] : mutation.values) {
                const auto field=std::ranges::find(entry->descriptor.properties,key,&PropertyDescriptor::key);
                if (field==entry->descriptor.properties.end()) { error="Unknown component property: "+key; return false; }
                if (editing && (!field->editable || field->editorHint=="telemetry")) { error="Property is read-only: "+key; return false; }
                if (!ValidatePropertyValue(*field,value,&error)) return false;
            }
            std::function<void()> commit;
            if (!entry->prepare(mutation.object,mutation.values,commit,error)) return false;
            commits.push_back(std::move(commit));
        }
        if (!validateOnly) for (const auto &commit : commits) commit();
        error.clear(); return true;
    }
};
}

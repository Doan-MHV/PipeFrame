#pragma once
#include <PipeFrame/Project/ComponentRegistry.h>
#include <map>
#include <typeindex>
#include <unordered_set>

namespace pipeframe {
// One registration supplies the editor descriptor and the ECS creation recipe.
// The caller owns the BehaviourScene, so domain systems and authored entities
// share one scheduler and lifetime rather than running in parallel worlds.
class EntityRegistry {
    template<class T> struct BehaviourAttachment {};
    struct Entry {
        SceneObjectTypeDescriptor descriptor;
        std::function<SceneObject(BehaviourScene &,const SceneObjectData &)> create;
        bool editorVisible{true};
        std::function<void(SceneObject,const SceneObjectData &)> restored;
    };
    std::map<SceneObjectTypeId,Entry> entries;
    std::unordered_set<std::type_index> registeredTypes;
    std::map<std::string,std::function<void(SceneObject)>> behaviours;
public:
    template<class T> void RegisterBehaviour(ComponentRegistry &components,std::string id,std::string displayName) {
        components.Register(ComponentSchema<BehaviourAttachment<T>>(id,std::move(displayName)));
        behaviours.emplace(std::move(id),[](SceneObject object){object.template Attach<T>();});
    }
    void AttachBehaviours(SceneObject object,const SceneObjectData &data) const {
        for(const auto &component:data.components)if(component.enabled) {
            const auto found=behaviours.find(component.typeId);
            if(found!=behaviours.end())found->second(object);
        }
    }
    template<class T> void Register(SceneObjectTypeDescriptor descriptor,bool editorVisible=true) {
        RegisterFactory<T>(std::move(descriptor),[](BehaviourScene &scene,const SceneObjectData &) -> SceneObject {
            if constexpr(std::is_default_constructible_v<T>)return scene.Instantiate(T{});
            else throw std::invalid_argument("This entity requires typed spawn arguments");
        },editorVisible);
    }
    template<class T,class Factory> void RegisterFactory(SceneObjectTypeDescriptor descriptor,Factory factory,bool editorVisible=true) {
        if(descriptor.typeId.empty() || entries.contains(descriptor.typeId))
            throw std::invalid_argument("Empty or duplicate entity type: "+descriptor.typeId);
        const auto id=descriptor.typeId;
        entries.emplace(id,Entry{std::move(descriptor),std::move(factory),editorVisible});
        registeredTypes.insert(typeid(T));
    }
    template<class T> SceneObject Spawn(BehaviourScene &scene,const T &recipe) const {
        if(!registeredTypes.contains(typeid(T)))throw std::invalid_argument("Unregistered typed entity recipe");
        return scene.Instantiate(recipe);
    }
    template<class Factory> void BindFactory(const SceneObjectTypeId &id,Factory factory,
        std::function<void(SceneObject,const SceneObjectData &)> restored={}) {
        entries.at(id).create=std::move(factory);
        entries.at(id).restored=std::move(restored);
    }
    bool Contains(const SceneObjectTypeId &id) const {return entries.contains(id);}
    std::vector<SceneObjectTypeDescriptor> Describe(bool includeRuntimeOnly=false) const {
        std::vector<SceneObjectTypeDescriptor> result;
        for(const auto &[id,entry]:entries)if(entry.editorVisible || includeRuntimeOnly)result.push_back(entry.descriptor);
        return result;
    }
    SceneObject Instantiate(BehaviourScene &scene,const SceneObjectData &data,const ComponentRegistry &components) const {
        const auto found=entries.find(data.typeId);
        if(found==entries.end())throw std::invalid_argument("Unregistered entity type: "+data.typeId);
        auto object=found->second.create(scene,data);
        try {Restore(object,data,components);
            if(found->second.restored)found->second.restored(object,data);
            AttachBehaviours(object,data);} catch(...) {object.Destroy();throw;}
        return object;
    }
    static void Restore(SceneObject object,const SceneObjectData &data,const ComponentRegistry &components) {
        std::vector<ComponentMutation> mutations;
        for(const auto &component:data.components) {
            if(!components.Contains(component.typeId))throw std::invalid_argument("Unregistered component: "+component.typeId);
            mutations.push_back({object,component.typeId,component.properties});
        }
        std::string error;
        if(!components.Apply(mutations,error,false))throw std::invalid_argument(error);
    }
};

// Authored-ID lookup and rollback of partially created batches, reusable inside
// specialized runtimes. Handles are non-owning; the supplied scene owns cleanup.
class RegisteredEntityObjects {
    std::map<SceneObjectId,SceneObject> objects;
public:
    RegisteredEntityObjects()=default;
    RegisteredEntityObjects(const RegisteredEntityObjects &)=delete;
    RegisteredEntityObjects &operator=(const RegisteredEntityObjects &)=delete;
    RegisteredEntityObjects(RegisteredEntityObjects &&)=default;
    RegisteredEntityObjects &operator=(RegisteredEntityObjects &&)=default;
    void Rebuild(BehaviourScene &scene,std::span<const SceneObjectData> source,
                 const EntityRegistry &types,const ComponentRegistry &components) {
        std::map<SceneObjectId,SceneObject> next;
        try {
            for(const auto &data:source) if(types.Contains(data.typeId)) {
                if(next.contains(data.id))throw std::invalid_argument("Duplicate authored entity ID");
                next.emplace(data.id,types.Instantiate(scene,data,components));
            }
        } catch(...) {for(auto &[id,object]:next)object.Destroy();throw;}
        for(auto &[id,object]:objects)if(object.IsValid())object.Destroy();
        objects=std::move(next);
    }
    SceneObject Resolve(SceneObjectId id) const {
        const auto found=objects.find(id);return found==objects.end()?SceneObject{}:found->second;
    }
    const auto &All() const {return objects;}
    void Clear() {for(auto &[id,object]:objects)if(object.IsValid())object.Destroy();objects.clear();}
};
}

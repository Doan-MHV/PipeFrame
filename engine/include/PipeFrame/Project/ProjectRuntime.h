#ifndef PIPEFRAME_PROJECT_RUNTIME_H
#define PIPEFRAME_PROJECT_RUNTIME_H

#include <PipeFrame/Editor/BrushTool.h>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Input/InputEvent.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Project/ComponentRegistry.h>
#include <PipeFrame/Project/PluginAPI.h>

class RenderContext;

namespace pipeframe {

struct ProjectRuntimeContext;

// Optional runtime features use this lifecycle instead of inventing unrelated
// project-local base classes. ProjectRuntime remains the application-level
// composition root; RuntimeModule is for reusable services owned by it.
class RuntimeModule {
  public:
    virtual ~RuntimeModule() = default;
    virtual std::string_view GetModuleId() const = 0;
    virtual bool Load(const ProjectRuntimeContext &context, std::string &error) = 0;
    virtual void Unload() = 0;
};

enum class ProjectRuntimeViewMode {
    Editor,
    Simulation,
    Zen,
};

struct ProjectRuntimeContext {
    std::filesystem::path projectDirectory;
    ServiceRegistry *services{};
    EventQueue *events{};
    RenderSubmissionQueue *renderSubmissions{};
    ProjectLogSink *log{};
};

struct ProjectRuntimeStatistics {
    bool available{false};
    bool usingQuads{false};

    std::size_t candidateObjectCount{0};
    std::size_t visibleObjectCount{0};
    std::size_t vertexCount{0};

    float movementTimeMs{0.0f};
    float spatialGridTimeMs{0.0f};
    float geometryTimeMs{0.0f};
};

enum class ProjectRuntimeSceneEditKind {
    CreateObject,
    ReplaceObjectType,
    RemoveObjectType,
    ReplaceObject,
    RemoveObject,
    SetComponentEnabled,
};

struct ProjectRuntimeSceneEdit {
    ProjectRuntimeSceneEditKind kind{ProjectRuntimeSceneEditKind::CreateObject};
    SceneObjectTypeId targetTypeId;
    SceneObjectData object;
    std::string componentTypeId;
    bool componentEnabled{true};
};

enum class ProjectRuntimeAuthoringState : std::uint8_t {
    Stopped,
    Paused,
    Playing,
};

struct ProjectRuntimeComponentEdit {
    SceneObjectId objectId{};
    std::string componentTypeId;
    std::string propertyKey;
    PropertyValue value;
};

class ProjectRuntime {
  public:
    virtual ~ProjectRuntime() = default;
    template<class T> void RegisterBrush(std::string category="Project") {
        const auto schema=T::Schema().Describe();
        for(const auto &brush:environmentBrushes)if(brush.id==schema.typeId)throw std::invalid_argument("Duplicate brush ID");
        environmentBrushes.push_back({schema.typeId,schema.displayName,std::move(category),[]{return std::make_shared<T>();}});
    }
    bool RegisterEnvironmentBrushes(PluginRegistrar &registrar,std::string &error)const{
        for(const auto &brush:environmentBrushes){
            ExtensionDescriptor extension{brush.id,{},brush.label,ExtensionPoint::EnvironmentBrush,brush.category};
            extension.createBrush=brush.create;
            if(!registrar.Extension(std::move(extension),&error))return false;
        }
        return true;
    }
private:
    std::vector<BrushToolDescriptor> environmentBrushes;
public:

    virtual const char *GetName() const = 0;

    virtual ProjectPluginDescriptor GetPluginDescriptor() const {
        return {GetName(), GetName()};
    }

    virtual bool RegisterPlugin(PluginRegistrar &, std::string &) { return true; }

    virtual ProjectReloadState CaptureReloadState() const { return {}; }
    virtual bool RestoreReloadState(const ProjectReloadState &, std::string &) { return true; }

    virtual bool Load(
        const ProjectRuntimeContext &context,
        std::string &errorMessage
    ) = 0;

    virtual std::span<const SceneObjectTypeDescriptor>
    GetSceneObjectTypes() const = 0;

    virtual std::span<const SceneComponentTypeDescriptor>
    GetSceneComponentTypes() const {
        return {};
    }

    std::vector<SceneComponentTypeDescriptor> GetAllSceneComponentTypes() const {
        static const std::vector<SceneComponentTypeDescriptor> common{
            {Transform2DComponentTypeId, "Transform", 1, false, false,
             {{"position", "Position", PropertyKind::Vector2, Vector2f{}, true, "world units"},
              {"rotation", "Rotation", PropertyKind::Number, 0.0, true, "degrees"},
              {"scale", "Scale", PropertyKind::Vector2, Vector2f{1.0f, 1.0f}, true}}},
            {"pipeframe.identity", "Identity", 1, false, false,
             {{"label", "Label", PropertyKind::String, std::string{}, true}}},
            {"pipeframe.physics-body2d", "Physics Body 2D", 1, true, false,
             {{"bodyType", "Body Type", PropertyKind::String, std::string{"dynamic"}, true},
              {"mass", "Mass", PropertyKind::Number, 1.0, true, "kg", 0.001},
              {"damping", "Damping", PropertyKind::Number, 0.0, true, "", 0.0, 1.0}}},
            {"pipeframe.collider2d", "Collider 2D", 1, true, false,
             {{"shape", "Shape", PropertyKind::String, std::string{"box"}, true},
              {"size", "Size", PropertyKind::Vector2, Vector2f{1.0f, 1.0f}, true, "world units"},
              {"trigger", "Is Trigger", PropertyKind::Boolean, false, true}}},
            {"pipeframe.renderer2d", "Renderer 2D", 1, true, false,
             {{"tint", "Tint", PropertyKind::Color, Color{255,255,255,255}, true},
              {"texture", "Texture", PropertyKind::AssetReference, AssetReference{}, true},
              {"sortingOrder", "Sorting Order", PropertyKind::Integer, std::int64_t{0}, true}}},
        };
        std::vector<SceneComponentTypeDescriptor> result = common;
        const auto project = GetSceneComponentTypes();
        result.insert(result.end(), project.begin(), project.end());
        if (const auto *registry=GetComponentRegistry()) {
            for (auto descriptor : registry->Describe()) {
                const auto found=std::ranges::find(result,descriptor.typeId,&SceneComponentTypeDescriptor::typeId);
                if (found==result.end()) result.push_back(std::move(descriptor)); else *found=std::move(descriptor);
            }
        }
        return result;
    }

    virtual SceneObjectData CreateDefaultObject(
        const SceneObjectTypeId &typeId
    ) const {
        SceneObjectData object;

        object.name = typeId;
        object.typeId = typeId;

        for (const SceneObjectTypeDescriptor &type :
             GetSceneObjectTypes()) {
            if (type.typeId != typeId) {
                continue;
            }

            object.name = type.displayName;

            for (const PropertyDescriptor &property :
                 type.properties) {
                object.properties.emplace(
                    property.key,
                    property.defaultValue
                );
            }

            for (const std::string &componentTypeId : type.componentTypeIds) {
                for (const SceneComponentTypeDescriptor &componentType : GetAllSceneComponentTypes()) {
                    if (componentType.typeId != componentTypeId) continue;
                    SceneComponentData component{componentType.typeId, componentType.schemaVersion, {}, true,
                                                 componentType.editorOnly};
                    for (const PropertyDescriptor &property : componentType.properties)
                        component.properties.emplace(property.key, property.defaultValue);
                    object.components.push_back(std::move(component));
                    break;
                }
            }
            break;
        }

        SynchronizeTransformComponent(object);

        return object;
    }

    // Legacy object properties migrate once into their declared typed components.
    void NormalizeObjectComponents(SceneObjectData &object) const {
        const auto types=GetAllSceneComponentTypes();
        for (const auto &objectType : GetSceneObjectTypes()) if (objectType.typeId==object.typeId) {
            for (const auto &id : objectType.componentTypeIds) {
                const auto type=std::ranges::find(types,id,&SceneComponentTypeDescriptor::typeId);
                if (type==types.end()) continue;
                auto component=std::ranges::find(object.components,id,&SceneComponentData::typeId);
                if (component==object.components.end()) {
                    object.components.push_back({id,type->schemaVersion}); component=std::prev(object.components.end());
                }
                for (const auto &field : type->properties) {
                    if (const auto legacy=object.properties.find(field.key); legacy!=object.properties.end()) {
                        component->properties.insert_or_assign(field.key,legacy->second); object.properties.erase(legacy);
                    } else component->properties.try_emplace(field.key,field.defaultValue);
                }
            }
        }
        SynchronizeTransformComponent(object);
    }

    // Common authored-scene services used by specialized and generated runtimes.
    bool TrySynchronizeComponentValues(std::span<const SceneObjectData> previous,
        std::span<const SceneObjectData> next,ProjectRuntimeAuthoringState state) {
        if(previous.size()!=next.size())return false;
        std::vector<ProjectRuntimeComponentEdit> edits;
        for(const auto &value:next) {
            const auto old=std::ranges::find(previous,value.id,&SceneObjectData::id);
            if(old==previous.end())return false;
            auto comparable=value;comparable.components=old->components;comparable.transform=old->transform;
            if(comparable!=*old || value.components.size()!=old->components.size())return false;
            for(const auto &component:value.components) {
                const auto before=std::ranges::find(old->components,component.typeId,&SceneComponentData::typeId);
                if(before==old->components.end())return false;
                auto same=component;same.properties=before->properties;
                if(same!=*before || component.properties.size()!=before->properties.size())return false;
                for(const auto &[key,field]:component.properties) {
                    const auto found=before->properties.find(key);
                    if(found==before->properties.end())return false;
                    if(found->second!=field)edits.push_back({value.id,component.typeId,key,field});
                }
            }
        }
        return edits.empty() || ApplyComponentEdits(edits,state);
    }
    bool ApplyAuthoredComponentEdits(std::vector<SceneObjectData> &authored,
        std::span<const ProjectRuntimeComponentEdit> edits,std::vector<ComponentMutation> &mutations) {
        const auto *registry=GetComponentRegistry();if(!registry)return false;
        auto next=authored;mutations.clear();
        for(const auto &edit:edits) {
            const auto entity=ResolveSceneObject(edit.objectId);
            const auto object=std::ranges::find(next,edit.objectId,&SceneObjectData::id);
            if(!entity.IsValid() || object==next.end())return false;
            const auto component=std::ranges::find(object->components,edit.componentTypeId,&SceneComponentData::typeId);
            if(component==object->components.end())return false;
            component->properties.insert_or_assign(edit.propertyKey,edit.value);
            auto mutation=std::ranges::find_if(mutations,[&](const auto &m){return m.object==entity && m.typeId==edit.componentTypeId;});
            if(mutation==mutations.end())mutations.push_back({entity,edit.componentTypeId,{{edit.propertyKey,edit.value}}});
            else mutation->values.insert_or_assign(edit.propertyKey,edit.value);
        }
        std::string error;if(!registry->Apply(mutations,error))return false;
        for(auto &object:next)SynchronizeSceneTransform(object);
        authored=std::move(next);return true;
    }
    static double ReadNumber(const pipeframe::SceneObjectData &object, const std::string &key,
                                        const double defaultValue) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);

    if (iterator == properties->end()) {
        return defaultValue;
    }

    if (const auto *number = std::get_if<double>(&iterator->second)) {
        return *number;
    }

    if (const auto *integer = std::get_if<std::int64_t>(&iterator->second)) {
        return static_cast<double>(*integer);
    }

    return defaultValue;
}

    static std::int64_t ReadInteger(const pipeframe::SceneObjectData &object, const std::string &key,
                                               const std::int64_t defaultValue) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);

    if (iterator == properties->end()) {
        return defaultValue;
    }

    if (const auto *integer = std::get_if<std::int64_t>(&iterator->second)) {
        return *integer;
    }

    if (const auto *number = std::get_if<double>(&iterator->second)) {
        return static_cast<std::int64_t>(*number);
    }

    return defaultValue;
}

    static bool ReadBoolean(const pipeframe::SceneObjectData &object, const std::string &key,
                                       const bool defaultValue) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);

    if (iterator == properties->end()) {
        return defaultValue;
    }

    if (const auto *value = std::get_if<bool>(&iterator->second)) {
        return *value;
    }

    if (const auto *integer = std::get_if<std::int64_t>(&iterator->second)) {
        return *integer != 0;
    }

    if (const auto *number = std::get_if<double>(&iterator->second)) {
        return *number != 0.0;
    }

    return defaultValue;
}

    static std::string ReadString(const pipeframe::SceneObjectData &object, const std::string &key,
                                             const std::string &defaultValue) {
    const pipeframe::PropertyMap *properties=&object.properties;
    if(!properties->contains(key))if(const auto component=std::ranges::find(object.components,object.typeId,&pipeframe::SceneComponentData::typeId);
       component!=object.components.end()&&component->properties.contains(key))properties=&component->properties;
    const auto iterator = properties->find(key);

    if (iterator == properties->end()) {
        return defaultValue;
    }

    if (const auto *value = std::get_if<std::string>(&iterator->second)) {
        return *value;
    }

    return defaultValue;
}

    virtual void SynchronizeScene(
        std::span<const SceneObjectData> objects
    ) = 0;

    // A runtime may consume authored component edits directly. This is useful for
    // physics-backed Transform/Body components whose runtime state is authoritative
    // while paused or playing. Returning false requests the normal full scene sync.
    virtual bool ApplyComponentEdits(
        std::span<const ProjectRuntimeComponentEdit> edits,
        ProjectRuntimeAuthoringState state
    ) {
        (void)edits;
        (void)state;
        return false;
    }

    // Transient simulation values are displayed by the Inspector but are never
    // written into the authored scene or its undo history.
    virtual std::vector<ProjectRuntimeComponentEdit> GetLiveComponentProperties() const {
        return {};
    }

    virtual void SetSelectedObject(
        std::optional<SceneObjectId> objectId
    ) = 0;

    virtual void SetViewMode(
        ProjectRuntimeViewMode mode
    ) {
        (void)mode;
    }

    virtual std::optional<SceneObjectId> HitTest(
        Vector2f worldPosition
    ) const = 0;

    virtual void Start() = 0;
    virtual void FixedUpdate(float fixedDeltaTime) = 0;
    virtual void Render(RenderContext &context) = 0;
    virtual void RenderScreen(RenderContext &context) {
        (void)context;
    }
    virtual bool ConsumesPointerAt(
        Vector2i screenPosition,
        const RenderContext &context
    ) const {
        (void)screenPosition;
        (void)context;
        return false;
    }
    // UI dispatch precedes editor shortcuts and world tools; true owns this event.
    virtual bool HandleUIEvent(const InputEvent &, RenderContext &) { return false; }
    // A project may reserve right-click for domain selection or placement.
    virtual bool UsesRightClickTool() const { return false; }
    virtual bool HasUIFocus() const { return false; }
    virtual bool HasWorldPointerCapture() const { return false; }
    virtual void Reset() = 0;
    virtual void Stop() = 0;
    virtual void Unload() = 0;

    virtual void HandleEvent(
        const InputEvent &event,
        RenderContext &context
    ) {
        (void)event;
        (void)context;
    }

    virtual std::vector<ProjectRuntimeSceneEdit> ConsumeSceneEdits() {
        return {};
    }

    // Generic live ECS inspection. Absence means this runtime uses the legacy authored-data path.
    virtual const ComponentRegistry *GetComponentRegistry() const { return nullptr; }
    virtual SceneObject ResolveSceneObject(SceneObjectId) const { return {}; }
    virtual bool ValidateComponentEdits(std::span<const ProjectRuntimeComponentEdit> edits,std::string &error) const {
        const auto *registry=GetComponentRegistry();
        if (!registry) return true;
        std::vector<ComponentMutation> mutations;
        for (const auto &edit : edits) {
            const auto entity=ResolveSceneObject(edit.objectId);
            if (!entity.IsValid()) continue; // Legacy authored-only object types keep their existing validation path.
            const auto found=std::ranges::find_if(mutations,[&](const auto &m) { return m.object==entity && m.typeId==edit.componentTypeId; });
            if (found==mutations.end()) mutations.push_back({entity,edit.componentTypeId,{{edit.propertyKey,edit.value}}});
            else found->values.insert_or_assign(edit.propertyKey,edit.value);
        }
        return registry->Apply(mutations,error,true,true);
    }
    std::optional<std::vector<SceneComponentData>> InspectObjectComponents(SceneObjectId id) const {
        const auto *registry=GetComponentRegistry();
        const auto object=ResolveSceneObject(id);
        if (!registry || !object.IsValid()) return std::nullopt;
        return registry->Inspect(object);
    }

    virtual ProjectRuntimeStatistics
    GetStatistics() const {
        return {};
    }
};

#if defined(_WIN32)
    #if defined(PIPEFRAME_BUILDING_PROJECT_RUNTIME)
        #define PIPEFRAME_PROJECT_RUNTIME_EXPORT __declspec(dllexport)
    #else
        #define PIPEFRAME_PROJECT_RUNTIME_EXPORT __declspec(dllimport)
    #endif
#else
    #define PIPEFRAME_PROJECT_RUNTIME_EXPORT \
        __attribute__((visibility("default")))
#endif

inline constexpr const char *
CreateProjectRuntimeSymbol =
    "PipeFrameCreateProjectRuntime";

inline constexpr const char *
DestroyProjectRuntimeSymbol =
    "PipeFrameDestroyProjectRuntime";

using CreateProjectRuntimeFunction =
    ProjectRuntime *(*)();

using DestroyProjectRuntimeFunction =
    void (*)(ProjectRuntime *);

} // namespace pipeframe

#endif

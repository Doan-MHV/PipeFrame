#ifndef PIPEFRAME_PROJECT_RUNTIME_HOST_H
#define PIPEFRAME_PROJECT_RUNTIME_HOST_H

#include <PipeFrame/Project/AssetDatabase.h>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Input/InputEvent.h>

#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/Project/ProjectRuntimeLibrary.h>
#include <PipeFrame/Project/PluginAPI.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Physics/PhysicsWorld2D.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <PipeFrame/Render/RenderServices2D.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

class RenderContext;

namespace pipeframe::editor {

class ProjectRuntimeHost final {
  public:
    ProjectRuntimeHost() = default;
    void SetBrushLifecycle(std::function<void()> release,std::function<void()> loaded){releaseBrushes=std::move(release);loadBrushes=std::move(loaded);}
    void SetAssetDatabase(assets::AssetDatabase &database) { assetDatabase=&database; }
    ~ProjectRuntimeHost();

    ProjectRuntimeHost(
        const ProjectRuntimeHost &) = delete;

    ProjectRuntimeHost &operator=(
        const ProjectRuntimeHost &) = delete;

    bool Load(
        const std::filesystem::path
            &projectDirectory,
        const std::filesystem::path
            &runtimeLibrary,
        std::string *errorMessage =
            nullptr);

    bool Reload(std::span<const SceneObjectData> objects,
                std::optional<SceneObjectId> selection,
                std::string *errorMessage = nullptr);
    bool ReloadFrom(const std::filesystem::path &candidateLibrary,
                    std::span<const SceneObjectData> objects,
                    std::optional<SceneObjectId> selection,
                    std::string *errorMessage = nullptr);

    void Unload();

    std::vector<BrushToolDescriptor> EnvironmentBrushes()const{
        std::vector<BrushToolDescriptor> result;
        for(const auto *extension:extensions.FindByPoint(ExtensionPoint::EnvironmentBrush))if(extension->createBrush)
            result.push_back({extension->id,extension->displayName,extension->context,extension->createBrush});
        return result;
    }
    bool HasRuntime() const;
    bool IsPreviewActive() const;

    std::span<
        const SceneObjectTypeDescriptor>
    GetSceneObjectTypes() const;

    std::span<const SceneComponentTypeDescriptor>
    GetSceneComponentTypes() const;

    SceneObjectData CreateDefaultObject(
        const SceneObjectTypeId
            &typeId) const;

    void SynchronizeScene(
        std::span<
            const SceneObjectData>
            objects);

    bool ValidateComponentEdits(std::span<const ProjectRuntimeComponentEdit> edits,std::string &error) const {
        return !runtime || runtime->ValidateComponentEdits(edits,error);
    }
    bool ApplyComponentEdits(
        std::span<const ProjectRuntimeComponentEdit> edits);

    std::vector<ProjectRuntimeComponentEdit> GetLiveComponentProperties() const;
    std::optional<std::vector<SceneComponentData>> InspectObjectComponents(SceneObjectId id) const {
        return runtime ? runtime->InspectObjectComponents(id) : std::nullopt;
    }

    void SetSelectedObject(
        std::optional<SceneObjectId>
            objectId);

    void SetViewMode(
        ProjectRuntimeViewMode mode);

    std::optional<SceneObjectId>
    HitTest(
        Vector2f
            worldPosition) const;

    ProjectRuntimeStatistics
    GetStatistics() const;

    void BeginPreview();
    void PausePreview();
    void ResumePreview();
    void EndPreview();
    void ResetPreview();

    bool HandleUIEvent(const InputEvent &event, RenderContext &context);
    bool UsesRightClickTool() const;
    bool HasUIFocus() const;
    bool HasWorldPointerCapture() const;
    void HandleEvent(
        const InputEvent &event,
        RenderContext &context);

    void FixedUpdate(
        float fixedDeltaTime);

    void VariableUpdate(float deltaTime);

    void Render(
        RenderContext &context);

    void RenderScreen(
        RenderContext &context);

    bool ConsumesPointerAt(
        Vector2i screenPosition,
        const RenderContext &context) const;

    std::vector<ProjectRuntimeSceneEdit>
    ConsumeSceneEdits();

    const ProjectPluginDescriptor &GetPluginDescriptor() const { return pluginDescriptor; }
    const ExtensionRegistry &GetExtensions() const { return extensions; }
    const ActionRegistry &GetActions() const { return actions; }
    const SystemRegistry &GetSystems() const { return systems; }
    bool InvokeAction(std::string_view id, const ActionInvocation &invocation = {},std::string *error=nullptr) const {
        return actions.Invoke(id, invocation,error);
    }
    bool InvokeExtension(std::string_view id,const ExtensionInvocation &invocation={},
                         std::string *error=nullptr) const { return extensions.Invoke(id,invocation,error); }
    const std::string &GetLastPluginError() const { return lastPluginError; }

  private:
    std::function<void()> releaseBrushes,loadBrushes;
    assets::AssetDatabase *assetDatabase{};
    class RuntimeComponentQuery final : public ComponentQuery {
    public:
        std::span<SceneObjectData> Objects() override { return objects; }
        std::span<const SceneObjectData> Objects() const override { return objects; }
        void Assign(std::span<const SceneObjectData> source) { objects.assign(source.begin(), source.end()); }
    private:
        std::vector<SceneObjectData> objects;
    };

    static void SetError(
        std::string *errorMessage,
        std::string message);
    void RegisterHostServices();
    bool ExecutePhase(SystemPhase phase,SystemContext &context);

    class HostLogSink final : public ProjectLogSink {
    public:
        void Write(std::string_view category,std::string_view message) override {
            entries.emplace_back(std::string(category)+": "+std::string(message));
        }
        void Clear(){entries.clear();}
        std::vector<std::string> entries;
    };

    ProjectRuntimeLibrary library;
    ProjectRuntime *runtime = nullptr;
    std::vector<SceneComponentTypeDescriptor> componentTypes;
    ProjectPluginDescriptor pluginDescriptor;
    ExtensionRegistry extensions;
    ActionRegistry actions;
    SystemRegistry systems;
    ServiceRegistry services;
    UniformSpatialIndex<SceneObjectId> spatialIndex;
    PhysicsWorld2D physicsWorld{{{-100000.0f,-100000.0f},{200000.0f,200000.0f}}};
    GraphicsResourceService resources;
    SurfaceRegistry surfaces;
    DebugDraw2D debugDraw;
    HostLogSink log;
    EventQueue events;
    RenderSubmissionQueue renderSubmissions;
    std::vector<GeometryCommand> screenRenderSubmissions;
    StructuralCommandBuffer commands;
    DeterministicRandom random{1};
    Profiler profiler;
    DeterministicJobQueue jobs;
    RuntimeComponentQuery componentQuery;
    std::uint64_t tick{};
    std::string lastPluginError;
    std::filesystem::path projectDirectory;
    std::filesystem::path shadowLibraryPath;
    std::optional<SceneObjectId> selectedObject;
    ProjectRuntimeViewMode viewMode{ProjectRuntimeViewMode::Editor};

    bool runtimeLoaded = false;
    bool previewActive = false;
    bool previewRunning = false;
};

} // namespace pipeframe::editor

#endif

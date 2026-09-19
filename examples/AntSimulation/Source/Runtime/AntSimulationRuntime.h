#ifndef ANT_SIMULATION_RUNTIME_H
#define ANT_SIMULATION_RUNTIME_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Environment/TilemapAssetModule.h>
#include <PipeFrame/Environment/VisualAssetModule.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/UI/View.h>
class SimulationDashboard;
#include "World/Runtime/ColonyHistory.h"
#include <PipeFrame/Project/EntityRegistry.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <map>

#include "Configuration/AntConfiguration.h"
#include "Editor/AntEditorTool.h"
#include "Editor/AntEditorToolRenderer.h"
#include "Editor/AntInspector.h"
#include "Editor/ColonyInspector.h"
#include "World/AntWorld.h"
#include "World/Rendering/AntDebugRenderer.h"
#include "World/Rendering/AntRenderingWorld.h"
#include "World/Rendering/EnvironmentRenderer.h"
#include "World/Rendering/ShadowRenderer.h"

namespace ant_simulation {

class AntSimulationRuntime final : public pipeframe::ProjectRuntime {
  public:
    using RenderStatistics = AntRenderingWorld::RenderStatistics;
    using RenderOptions = AntRenderingWorld::RenderOptions;

    AntSimulationRuntime();
    ~AntSimulationRuntime() override;
    template <class T> void RegisterComponent() {
        componentRegistry.Register(T::Schema());
        componentTypes.push_back(T::Schema().Describe());
    }
    template <class T> void RegisterEntity(pipeframe::SceneObjectTypeDescriptor descriptor) {
        entityRegistry.Register<T>(std::move(descriptor));
        objectTypes = entityRegistry.Describe();
    }
    template <class T> void RegisterBehaviour(std::string id, std::string name) {
        entityRegistry.RegisterBehaviour<T>(componentRegistry, std::move(id), std::move(name));
    }

    void CollectWorldDebug(pipeframe::WorldDebugDraw &, pipeframe::WorldDebugOptions) override;
    const char *GetName() const override;
    pipeframe::ProjectPluginDescriptor GetPluginDescriptor() const override;
    bool RegisterPlugin(pipeframe::PluginRegistrar &registrar, std::string &error) override;

    bool Load(const pipeframe::ProjectRuntimeContext &context, std::string &errorMessage) override;

    std::span<const pipeframe::SceneObjectTypeDescriptor> GetSceneObjectTypes() const override;
    std::span<const pipeframe::SceneComponentTypeDescriptor> GetSceneComponentTypes() const override;

    pipeframe::SceneObjectData CreateDefaultObject(const pipeframe::SceneObjectTypeId &typeId) const override;

    void SynchronizeScene(std::span<const pipeframe::SceneObjectData> objects) override;
    const pipeframe::ComponentRegistry *GetComponentRegistry() const override { return &componentRegistry; }
    pipeframe::SceneObject ResolveSceneObject(pipeframe::SceneObjectId id) const override;
    bool ApplyComponentEdits(std::span<const pipeframe::ProjectRuntimeComponentEdit> edits,
                             pipeframe::ProjectRuntimeAuthoringState state) override;
    std::vector<pipeframe::ProjectRuntimeComponentEdit> GetLiveComponentProperties() const override;

    void SetSelectedObject(std::optional<pipeframe::SceneObjectId> objectId) override;

    void SetViewMode(pipeframe::ProjectRuntimeViewMode mode) override;

    std::optional<pipeframe::SceneObjectId> HitTest(pipeframe::Vector2f worldPosition) const override;

    void Start() override;

    void FixedUpdate(float fixedDeltaTime) override;

    void Render(RenderContext &context) override;

    void RenderScreen(RenderContext &context) override;
    bool HandleUIEvent(const pipeframe::InputEvent &event, RenderContext &context) override;
    bool ConsumesPointerAt(pipeframe::Vector2i position, const RenderContext &context) const override;
    bool UsesRightClickTool() const override { return viewMode != pipeframe::ProjectRuntimeViewMode::Zen; }
    bool HasWorldPointerCapture() const override { return editorTool.IsStrokeActive(); }
    bool HasUIFocus() const override;
    SimulationDashboard *GetDashboard() const { return dashboard.get(); }
    void HandleEvent(const pipeframe::InputEvent &event, RenderContext &context) override;

    void Reset() override;
    void Stop() override;
    void Unload() override;

    pipeframe::ProjectRuntimeStatistics GetStatistics() const override;

    [[nodiscard]]
    const RenderStatistics &GetRenderStatistics() const;

    [[nodiscard]]
    const RenderOptions &GetRenderOptions() const;

    [[nodiscard]]
    AntWorld *GetSimulationWorld();

    [[nodiscard]]
    const AntWorld *GetSimulationWorld() const;

    [[nodiscard]]
    AntEditorTool &GetEditorTool();

    [[nodiscard]]
    bool SelectAntAt(pipeframe::Vector2f worldPosition, float selectionRadius);

    void ClearSelectedAnt();

    [[nodiscard]]
    const AntInspectorData &GetAntInspectorData() const;

    std::optional<std::int64_t> GetRemainingFood(pipeframe::SceneObjectId foodSourceId) const;

    std::optional<std::int64_t> GetDeliveredFood(pipeframe::SceneObjectId colonyId) const;

  private:
    bool RebuildSimulation(std::string *errorMessage = nullptr);

    bool LoadRendererAssets(const std::filesystem::path &assetRoot, std::string &errorMessage);

    void ApplyRenderOptions();
    void AdvanceSimulation(float fixedDeltaTime);
    void RefreshSimulationState(float fixedDeltaTime);

    void FitCamera(RenderContext &context);

    void DrawAuthoredSelection(RenderContext &context) const;

    [[nodiscard]]
    float GetRenderZoom(const RenderContext &context) const;

    [[nodiscard]]
    const pipeframe::SceneObjectData *FindAuthoredObject(pipeframe::SceneObjectId objectId) const;

    [[nodiscard]]
    std::int64_t GetFoodQuantityInRadius(pipeframe::Vector2f position, float radius) const;

    static float GetSelectionRadius(const pipeframe::SceneObjectData &object);

    static bool ContainsPoint(pipeframe::Vector2f center, float radius, pipeframe::Vector2f point);

    void BuildDashboard();
    pipeframe::RegisteredEntityObjects registeredObjects;
    pipeframe::EntityRegistry entityRegistry;
    void RefreshDashboard();
    pipeframe::ui::View BuildDashboardView(std::size_t panel);
    pipeframe::GraphicsResourceService uiResources;
    pipeframe::FontHandle uiFont;
    std::unique_ptr<SimulationDashboard> dashboard;
    std::map<ColonyId, ColonyHistory> histories;
    std::optional<ColonyId> selectedColony;
    std::shared_ptr<pipeframe::GraphicsResourceService> previewResources;
    pipeframe::TextureHandle previewBody, previewLeg, previewFood;
    AntGeometry previewGeometry;
    bool showAntComponents{};
    AntConfiguration configuration;

    std::unique_ptr<AntWorld> simulationWorld;

    std::shared_ptr<AntRenderingWorld> renderingWorld;

    AntEditorTool editorTool;

    AntInspector antInspector;
    ColonyInspector colonyInspector;

    pipeframe::ComponentRegistry componentRegistry;
    std::vector<pipeframe::SceneComponentTypeDescriptor> componentTypes;
    std::vector<pipeframe::SceneObjectTypeDescriptor> objectTypes;

    std::vector<pipeframe::SceneObjectData> authoredObjects;

    std::optional<pipeframe::SceneObjectId> selectedObjectId;

    pipeframe::TilemapAssetModule tilemapAssets;
    pipeframe::VisualAssetModule environmentVisuals;
    pipeframe::ProjectLogSink *environmentLog{};
    pipeframe::AssetReference authoredMap;
    std::uint64_t authoredMapRevision{};
    void RefreshAuthoredEnvironment();
    std::filesystem::path projectDirectory;

    RenderStatistics renderStatistics;

    RenderOptions renderOptions;

    pipeframe::ProjectRuntimeAuthoringState liveAuthoringState{pipeframe::ProjectRuntimeAuthoringState::Stopped};
    bool loaded{false};
    bool playing{false};
    bool selectedAntWasAvailable{false};
    bool cameraInitialized{false};
    float simulationElapsedTime{0.0f};
    pipeframe::ProjectRuntimeViewMode viewMode{pipeframe::ProjectRuntimeViewMode::Editor};
};

} // namespace ant_simulation

#endif

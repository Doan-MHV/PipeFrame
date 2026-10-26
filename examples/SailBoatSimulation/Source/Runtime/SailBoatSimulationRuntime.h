#ifndef SAILBOAT_SIMULATION_RUNTIME_H
#define SAILBOAT_SIMULATION_RUNTIME_H

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <PipeFrame/Audio/AudioService.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/UI/SimulationDashboard.h>
#include <PipeFrame/UI/Chart.h>
#include <PipeFrame/UI/TableView.h>
#include <PipeFrame/UI/NetworkView.h>

#include "Configuration/SailBoatConfiguration.h"
#include "Rendering/SailBoatRenderer.h"
#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include "Training/SailBoatPopulationTrainer.h"
#include "Training/SailBoatRaceTask.h"
#include "World/RaceCourse.h"

namespace sailboat_simulation {

class SailBoatSimulationRuntime final : public pipeframe::ProjectRuntime {
public:
    const char *GetName() const override;
    pipeframe::ProjectPluginDescriptor GetPluginDescriptor() const override;
    bool RegisterPlugin(pipeframe::PluginRegistrar &registrar, std::string &error) override;

    bool Load(const pipeframe::ProjectRuntimeContext &context, std::string &errorMessage) override;
    std::span<const pipeframe::SceneObjectTypeDescriptor> GetSceneObjectTypes() const override;
    std::span<const pipeframe::SceneComponentTypeDescriptor> GetSceneComponentTypes() const override;
    pipeframe::SceneObjectData CreateDefaultObject(const pipeframe::SceneObjectTypeId &typeId) const override;
    void SynchronizeScene(std::span<const pipeframe::SceneObjectData> objects) override;
    void SetSelectedObject(std::optional<pipeframe::SceneObjectId> objectId) override;
    void SetViewMode(pipeframe::ProjectRuntimeViewMode mode) override;
    std::optional<pipeframe::SceneObjectId> HitTest(pipeframe::Vector2f worldPosition) const override;

    void Start() override;
    void FixedUpdate(float fixedDeltaTime) override;
    void Render(RenderContext &context) override;
    void RenderScreen(RenderContext &context) override;
    bool ConsumesPointerAt(pipeframe::Vector2i screenPosition, const RenderContext &context) const override;
    bool HandleUIEvent(const pipeframe::InputEvent &event, RenderContext &context) override;
    bool UsesRightClickTool() const override { return viewMode!=pipeframe::ProjectRuntimeViewMode::Zen && raceEditorTool!=RaceEditorTool::None; }
    bool HasUIFocus() const override { return dashboard && dashboard->HasKeyboardFocus(); }
    SimulationDashboard *GetDashboard() const { return dashboard.get(); }
    void HandleEvent(const pipeframe::InputEvent &event, RenderContext &context) override;
    std::vector<pipeframe::ProjectRuntimeSceneEdit> ConsumeSceneEdits() override;
    pipeframe::ProjectRuntimeStatistics GetStatistics() const override;
    void Reset() override;
    void Stop() override;
    void Unload() override;

    [[nodiscard]] const SailBoatConfiguration &GetConfiguration() const;
    [[nodiscard]] const RaceCourse &GetRaceCourse() const;
    [[nodiscard]] const Boat &GetPreviewBoat() const;
    [[nodiscard]] const SailBoatRaceTask &GetPreviewRaceTask() const;
    [[nodiscard]] const SailBoatPopulationTrainer &GetPopulationTrainer() const;
    [[nodiscard]] bool HasPreviewBoat() const;
    [[nodiscard]] bool IsPlaying() const;

private:
    void AdvancePreview(float fixedDeltaTime);
    void AdvanceTraining(float fixedDeltaTime);
    void AdvancePresentation(float fixedDeltaTime);
    static std::vector<pipeframe::SceneObjectTypeDescriptor> CreateObjectTypes();
    static std::vector<pipeframe::SceneComponentTypeDescriptor> CreateComponentTypes(
        std::span<const pipeframe::SceneObjectTypeDescriptor> objectTypes);

    void RebuildDomainState();
    void ResetPreviewBoat();
    void FitCamera(RenderContext &context);
    void DrawRaceObject(sf::RenderTarget &target, const pipeframe::SceneObjectData &object) const;
    void DrawPreviewBoat(sf::RenderTarget &target) const;
    void DrawPreviewTrajectory(sf::RenderTarget &target) const;
    void DrawRaceEditorPreview(sf::RenderTarget &target) const;
    void BuildDashboard();
    void RefreshDashboard();
    void RefreshCheckpoints();
    void ExecuteRaceAction(std::size_t index);
    void HandleRaceEditorRightClick(pipeframe::Vector2f worldPosition);
    void HandleShortcut(const pipeframe::KeyInput &event);
    void UpdateMarkAudio(std::size_t targetIndex);
    void QueueDirectedObject(const pipeframe::SceneObjectTypeId &typeId,
                             pipeframe::Vector2f first, pipeframe::Vector2f second);
    void QueueLoadedRace(std::vector<pipeframe::SceneObjectData> objects);

    [[nodiscard]] const pipeframe::SceneObjectData *FindObject(pipeframe::SceneObjectId objectId) const;

    static double ReadNumber(const pipeframe::SceneObjectData &object, const std::string &key, double fallback);
    static std::int64_t ReadInteger(const pipeframe::SceneObjectData &object, const std::string &key,
                                    std::int64_t fallback);
    static bool ReadBoolean(const pipeframe::SceneObjectData &object, const std::string &key, bool fallback);
    static pipeframe::Vector2f ReadVector(const pipeframe::SceneObjectData &object, const std::string &key,
                                          pipeframe::Vector2f fallback);
    static RaceSegment MakeSegment(const pipeframe::SceneObjectData &object, float fallbackLength);

    std::vector<pipeframe::SceneObjectTypeDescriptor> objectTypes = CreateObjectTypes();
    std::vector<pipeframe::SceneObjectData> authoredObjects;
    std::optional<pipeframe::SceneObjectId> selectedObjectId;

    SailBoatConfiguration configuration;
    RaceCourse raceCourse;
    BoatEnvironment boatEnvironment;
    Boat previewBoat;
    SailBoatRaceTask previewRaceTask;
    SailBoatPopulationTrainer populationTrainer;
    SailBoatRenderer renderer;
    pipeframe::GraphicsResourceService uiResources;
    pipeframe::FontHandle hudFont;
    std::unique_ptr<SimulationDashboard> dashboard;
    MetricCard *windCard=nullptr, *timerCard=nullptr, *resultCard=nullptr, *boatCard=nullptr;
    Label *raceStatus=nullptr, *boatDetails=nullptr, *networkDetails=nullptr, *historyDetails=nullptr, *modelStatus=nullptr, *performanceDetails=nullptr;
    TimeSeriesChart *scoreChart=nullptr, *completionChart=nullptr;
    TableView *historyTable=nullptr, *modelTable=nullptr;
    NetworkView *networkView=nullptr;
    std::array<TextButton *,6> raceActions{};
    std::array<TextButton *,3> workflowActions{};
    std::optional<std::filesystem::path> selectedCheckpoint;
    std::string checkpointStatus;

    pipeframe::AudioService audio;
    pipeframe::AudioClipHandle markSound;
    std::filesystem::path projectDirectory;
    std::filesystem::path trainingRunDirectory;
    std::filesystem::path raceSavePath;
    pipeframe::ProjectRuntimeViewMode viewMode{pipeframe::ProjectRuntimeViewMode::Editor};

    enum class RaceEditorTool {
        None,
        AddWaypoint,
        SetStartFirst,
        SetStartDirection,
        SetFinishFirst,
        SetFinishDirection,
    };

    RaceEditorTool raceEditorTool{RaceEditorTool::None};
    pipeframe::Vector2f raceEditorFirstPoint{};
    pipeframe::Vector2f raceEditorPointerWorld{};
    bool raceEditorPointerValid{false};
    std::vector<pipeframe::ProjectRuntimeSceneEdit> pendingSceneEdits;
    std::string raceEditorStatus{"Choose a tool, then right-click the water."};

    bool loaded{false};
    bool playing{false};
    bool cameraInitialized{false};
    bool previewBoatInitialized{false};
    bool hudFontLoaded{false};
    bool markSoundLoaded{false};
    bool hudVisible{true};
    bool worldVisible{true};
    bool followBestBoat{false};
    std::size_t lastAudibleTarget{0};
    float lastSimulationTimeMs{0.0f};
    float lastRenderTimeMs{0.0f};
    float elapsedSimulationTime{0.0f};
};

} // namespace sailboat_simulation

#endif

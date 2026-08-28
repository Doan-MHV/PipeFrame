#include "WorkbenchScene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <system_error>
#include <vector>

#include "DemoAgent.h"
#include "DiagnosticsOverlay.h"
#include "Editor/HierarchyPanel.h"
#include "Editor/PopulationBoundsRenderer.h"
#include "Editor/SceneDocument.h"
#include "Editor/SceneSerializer.h"
#include "Editor/SimulationPanel.h"
#include "Editor/ViewportToolbar.h"
#include "Runtime/RuntimeAgentPopulation.h"
#include "Runtime/RuntimePopulationPointRenderer.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <PipeFrame/Render/CameraController2D.h>

#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/Button.h>
#include <PipeFrame/UI/Label.h>
#include <PipeFrame/UI/Panel.h>
#include <PipeFrame/UI/StackPanel.h>
#include <PipeFrame/UI/UIManager.h>

class TestScene final : public Scene {
  public:
    void SetMetricsVisible(bool newVisible) {
        metricsVisible = newVisible;

        diagnosticsOverlay.SetVisible(metricsVisible);

        if (metricsInputBlocker != nullptr) {
            metricsInputBlocker->SetVisible(metricsVisible);
        }

        if (viewportToolbar != nullptr) {
            viewportToolbar->SetMetricsVisible(metricsVisible);
        }
    }

    void LayoutEditor(sf::Vector2u windowSize, RenderContext &context) {
        if (simulationPanel == nullptr || hierarchyPanel == nullptr) {
            return;
        }

        constexpr float HierarchyLeft = 24.0f;
        constexpr float HierarchyWidth = 220.0f;
        constexpr float HierarchyViewportGap = 16.0f;

        constexpr float InspectorWidth = 300.0f;
        constexpr float InspectorRightMargin = 80.0f;
        constexpr float MinimumInspectorLeft = 12.0f;

        constexpr float ViewportTop = 80.0f;
        constexpr float ViewportBottom = 80.0f;
        constexpr float ViewportInspectorGap = 16.0f;

        constexpr float ToolbarHeight = 68.0f;
        constexpr float ToolbarGap = 8.0f;

        const float windowWidth = static_cast<float>(windowSize.x);
        const float windowHeight = static_cast<float>(windowSize.y);

        const float viewportHeight = std::max(1.0f, windowHeight - ViewportTop - ViewportBottom);

        // Left Hierarchy panel.
        hierarchyPanel->SetPosition({HierarchyLeft, ViewportTop});
        hierarchyPanel->SetSize({HierarchyWidth, viewportHeight});

        // Right Inspector panel.
        const float inspectorX = std::max(MinimumInspectorLeft, windowWidth - InspectorWidth - InspectorRightMargin);

        simulationPanel->SetPosition({inspectorX, ViewportTop});

        // Center Scene View.
        const float viewportLeft = HierarchyLeft + HierarchyWidth + HierarchyViewportGap;

        const float viewportRight = inspectorX - ViewportInspectorGap;

        const float viewportWidth = std::max(1.0f, viewportRight - viewportLeft);

        viewportBorder.setPosition({viewportLeft, ViewportTop});
        viewportBorder.setSize({viewportWidth, viewportHeight});

        if (viewportToolbar != nullptr) {
            const sf::Vector2f toolbarPosition{viewportLeft, ViewportTop - ToolbarHeight - ToolbarGap};

            const sf::Vector2f toolbarSize{viewportWidth, ToolbarHeight};

            viewportToolbar->SetPosition(toolbarPosition);
            viewportToolbar->SetSize(toolbarSize);
        }

        const sf::Vector2f metricsPosition{viewportLeft + 8.0f, ViewportTop + 8.0f};

        diagnosticsOverlay.SetPosition(metricsPosition);

        if (metricsInputBlocker != nullptr) {
            metricsInputBlocker->SetPosition(metricsPosition);
            metricsInputBlocker->SetSize(diagnosticsOverlay.GetSize());
        }

        Camera2D &camera = context.GetCamera();

        camera.SetSize({viewportWidth, viewportHeight});

        const float safeWindowWidth = std::max(1.0f, windowWidth);
        const float safeWindowHeight = std::max(1.0f, windowHeight);

        camera.SetViewport({{viewportLeft / safeWindowWidth, ViewportTop / safeWindowHeight},
                            {viewportWidth / safeWindowWidth, viewportHeight / safeWindowHeight}});
    }

    void OnResize(sf::Vector2u newSize, RenderContext &context) override { LayoutEditor(newSize, context); }

    void Load() override {
        InitializeSceneDocument();

        const std::filesystem::path fontPath =
            std::filesystem::path(PIPEFRAME_ASSET_DIR) / "fonts/roboto_mono_semi.ttf";

        if (!diagnosticsOverlay.Load(fontPath)) {
            std::cerr << "Unable to load diagnostics font: " << fontPath << '\n';
        }

        if (!uiManager.LoadDefaultFont(fontPath)) {
            std::cerr << "Unable to load UI font: " << fontPath << '\n';
        }

        viewportBorder.setFillColor(sf::Color::Transparent);

        viewportBorder.setOutlineColor(sf::Color(78, 86, 104));

        viewportBorder.setOutlineThickness(1.0f);

        worldCursor.setRadius(8.0f);
        worldCursor.setOrigin({8.0f, 8.0f});
        worldCursor.setFillColor(sf::Color::Transparent);
        worldCursor.setOutlineColor(sf::Color::Green);
        worldCursor.setOutlineThickness(2.0f);

        viewportToolbar = &uiManager.CreateRoot<ViewportToolbar>(uiManager.GetDefaultFont());

        viewportToolbar->SetOnMetrics([this]() { SetMetricsVisible(!metricsVisible); });

        viewportToolbar->SetOnUndo([this]() { Undo(); });

        viewportToolbar->SetOnRedo([this]() { Redo(); });

        viewportToolbar->SetOnSave([this]() { SaveScene(); });

        viewportToolbar->SetOnLoad([this]() { LoadScene(); });

        metricsInputBlocker = &uiManager.CreateRoot<Panel>();

        metricsInputBlocker->SetFillColor(sf::Color::Transparent);

        metricsInputBlocker->SetOutlineColor(sf::Color::Transparent);

        metricsInputBlocker->SetOutlineThickness(0.0f);

        SetMetricsVisible(false);

        hierarchyPanel = &uiManager.CreateRoot<HierarchyPanel>(uiManager.GetDefaultFont());

        hierarchyPanel->SetOnAdd([this]() { CreateAgentPopulation(); });

        hierarchyPanel->SetOnDelete([this]() { DeleteSelectedObject(); });

        hierarchyPanel->SetOnSelectionChanged(
            [this](pipeframe::editor::SceneObjectId objectId) { SetSelectedObject(objectId); });

        RebuildHierarchyFromDocument();

        simulationPanel = &uiManager.CreateRoot<SimulationPanel>(uiManager.GetDefaultFont());

        simulationPanel->SetOnPlayPause([this]() { ToggleSimulationPlayPause(); });

        simulationPanel->SetOnSingleStep([this]() { RequestSingleSimulationStep(); });

        simulationPanel->SetOnReset([this]() { ResetRuntimePreview(); });

        simulationPanel->SetOnPositionXCommitted([this](float newPositionX) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                sf::Vector2f position = agent->GetPosition();
                position.x = newPositionX;

                agent->SetPosition(position);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        simulationPanel->SetOnPositionYCommitted([this](float newPositionY) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                sf::Vector2f position = agent->GetPosition();
                position.y = newPositionY;

                agent->SetPosition(position);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        simulationPanel->SetOnRotationCommitted([this](float newRotation) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                agent->SetRotation(newRotation);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        simulationPanel->SetOnAgentCountCommitted([this](float value) {
            ModifySelectedPopulation([value](pipeframe::editor::AgentPopulationSettings &settings) {
                settings.agentCount = ToUnsignedInteger(value, 1);
            });
        });

        simulationPanel->SetOnSpawnWidthCommitted([this](float value) {
            ModifySelectedPopulation([value](pipeframe::editor::AgentPopulationSettings &settings) {
                settings.spawnAreaSize.x = std::max(1.0f, value);
            });
        });

        simulationPanel->SetOnSpawnHeightCommitted([this](float value) {
            ModifySelectedPopulation([value](pipeframe::editor::AgentPopulationSettings &settings) {
                settings.spawnAreaSize.y = std::max(1.0f, value);
            });
        });

        simulationPanel->SetOnRandomSeedCommitted([this](float value) {
            ModifySelectedPopulation([value](pipeframe::editor::AgentPopulationSettings &settings) {
                settings.randomSeed = ToUnsignedInteger(value, 0);
            });
        });

        RefreshSimulationControls();
        SetSelectedObject(std::nullopt);
        RefreshInspector();
        RefreshHistoryControls();
    }

    void FixedUpdate(float fixedDeltaTime) override {
        if (!simulationController.ConsumeTick()) {
            return;
        }

        for (DemoAgent &agent : demoAgents) {
            agent.SetRotation(agent.GetRotation() + 90.0f * fixedDeltaTime);
        }

        float populationMovementTimeMs = 0.0f;
        float populationGridRebuildTimeMs = 0.0f;

        for (pipeframe::runtime::RuntimeAgentPopulation &population : runtimePopulations) {
            population.Update(fixedDeltaTime);

            pendingPopulationMovementTimeMs += population.GetLastUpdateStats().movementTimeMs;
        }

        constexpr float MoveSpeed = 300.0f;

        sf::Vector2f movement{0.0f, 0.0f};

        if (!uiManager.HasKeyboardFocus()) {
            if (Input::IsKeyDown(Key::W)) {
                movement.y -= MoveSpeed * fixedDeltaTime;
            }

            if (Input::IsKeyDown(Key::S)) {
                movement.y += MoveSpeed * fixedDeltaTime;
            }

            if (Input::IsKeyDown(Key::A)) {
                movement.x -= MoveSpeed * fixedDeltaTime;
            }

            if (Input::IsKeyDown(Key::D)) {
                movement.x += MoveSpeed * fixedDeltaTime;
            }
        }

        if (DemoAgent *selected = GetSelectedAgent()) {
            selected->Move(movement);
        }
    }

    void Update(float deltaTime) override {
        if (!uiManager.HasKeyboardFocus()) {
            if (Input::WasKeyPressed(Key::P)) {
                ToggleSimulationPlayPause();
            }

            if (Input::WasKeyPressed(Key::Period)) {
                RequestSingleSimulationStep();
            }
        }

        float populationGridRebuildTimeMs = 0.0f;

        for (pipeframe::runtime::RuntimeAgentPopulation &population : runtimePopulations) {
            population.RebuildSpatialGrid();

            populationGridRebuildTimeMs += population.GetLastUpdateStats().spatialGridRebuildTimeMs;
        }

        diagnosticsOverlay.SetPopulationSimulationStats(pendingPopulationMovementTimeMs, populationGridRebuildTimeMs);

        pendingPopulationMovementTimeMs = 0.0f;

        RefreshSimulationControls();
        RefreshInspector();

        for (DemoAgent &agent : demoAgents) {
            agent.SetSimulationPlaying(simulationController.IsPlaying());
        }

        diagnosticsOverlay.Update(deltaTime);
    }

    void Render(RenderContext &context) override {
        context.BeginWorld();

        auto &window = context.GetWindow();

        populationBoundsRenderer.Render(window, sceneDocument, selectedObjectId);

        const Camera2D &camera = context.GetCamera();

        const sf::Vector2f cameraSize = camera.GetSize();
        const sf::Vector2f cameraCenter = camera.GetCenter();

        const sf::FloatRect worldViewport{
            cameraCenter - cameraSize * 0.5f,
            cameraSize,
        };

        runtimePopulationPointRenderer.BeginFrame();

        for (const pipeframe::runtime::RuntimeAgentPopulation &population : runtimePopulations) {

            runtimePopulationPointRenderer.Render(window, population, worldViewport);
        }

        const auto &populationRenderStats = runtimePopulationPointRenderer.GetFrameStats();

        diagnosticsOverlay.SetPopulationRenderStats(populationRenderStats.candidateAgentCount,
                                                    populationRenderStats.visibleAgentCount,
                                                    populationRenderStats.geometryBuildTimeMs);

        for (const DemoAgent &agent : demoAgents) {
            agent.Render(window);
        }

        if (pointerInsideViewport) {
            window.draw(worldCursor);
        }

        context.BeginScreen();

        RefreshViewportStatus(context);

        window.draw(viewportBorder);
        uiManager.Render(window);

        diagnosticsOverlay.Render(window, simulationController, context.GetCamera(), worldCursor.getPosition());
    }

    void HandleEvent(const sf::Event &event, RenderContext &context) override {
        if (event.is<sf::Event::MouseLeft>() || event.is<sf::Event::FocusLost>()) {
            pointerInsideViewport = false;
            CancelObjectDrag(true);
        }

        // Continue an object drag even if the cursor moves
        // over another editor panel.
        if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
            if (draggingSelectedObject) {
                pointerInsideViewport = context.IsInsideWorldViewport(moved->position);

                const sf::Vector2f mouseWorldPosition = context.ScreenToWorld(moved->position);

                if (DemoAgent *selected = GetSelectedAgent()) {
                    selected->SetPosition(mouseWorldPosition + selectedDragOffset);
                }

                RefreshInspector();

                if (pointerInsideViewport) {
                    worldCursor.setPosition(mouseWorldPosition);
                }

                return;
            }
        }

        // Finish the object drag.
        if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (draggingSelectedObject && released->button == sf::Mouse::Button::Left) {
                if (DemoAgent *selected = GetSelectedAgent(); selected != nullptr && dragStartTransform.has_value()) {
                    RecordTransformChange(selected->GetId(), *dragStartTransform, CaptureTransform(*selected));
                }

                draggingSelectedObject = false;
                dragStartTransform.reset();

                RefreshInspector();
                return;
            }
        }

        const bool uiConsumedEvent = uiManager.HandleEvent(event);

        if (uiConsumedEvent) {
            if (event.is<sf::Event::MouseMoved>()) {
                pointerInsideViewport = false;
            }

            if (event.is<sf::Event::MouseButtonReleased>()) {
                cameraController.HandleEvent(event, context);
            }

            return;
        }

        if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            // Command on macOS, Control on Windows/Linux.
            const bool primaryModifier = keyPressed->system || keyPressed->control;

            if (primaryModifier && keyPressed->code == sf::Keyboard::Key::Z) {
                CancelObjectDrag(true);

                if (keyPressed->shift) {
                    Redo();
                } else {
                    Undo();
                }

                return;
            }

            const bool deleteKey =
                keyPressed->code == sf::Keyboard::Key::Delete || keyPressed->code == sf::Keyboard::Key::Backspace;

            if (!primaryModifier && !keyPressed->alt && deleteKey) {
                DeleteSelectedObject();
                return;
            }

            if (!primaryModifier && !keyPressed->alt && keyPressed->code == sf::Keyboard::Key::Escape) {
                if (draggingSelectedObject) {
                    CancelObjectDrag(true);
                } else {
                    SetSelectedObject(std::nullopt);
                }

                return;
            }
        }

        if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
            pointerInsideViewport = context.IsInsideWorldViewport(moved->position);

            cameraController.HandleEvent(event, context);

            if (pointerInsideViewport) {
                worldCursor.setPosition(context.ScreenToWorld(moved->position));
            }

            return;
        }

        if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (!context.IsInsideWorldViewport(pressed->position)) {
                return;
            }

            const bool selectionClick = pressed->button == sf::Mouse::Button::Left && !Input::IsKeyDown(Key::Space);

            if (selectionClick) {
                const sf::Vector2f worldPoint = context.ScreenToWorld(pressed->position);

                const std::optional<DemoAgentId> hitAgent = HitTestAgents(worldPoint);

                SetSelectedObject(hitAgent);

                dragStartTransform.reset();

                if (DemoAgent *selected = GetSelectedAgent(); selected != nullptr && CanAuthorScene()) {

                    draggingSelectedObject = true;

                    selectedDragOffset = selected->GetPosition() - worldPoint;

                    dragStartTransform = CaptureTransform(*selected);
                }
            }

            cameraController.HandleEvent(event, context);
            return;
        }

        if (event.is<sf::Event::MouseButtonReleased>()) {
            cameraController.HandleEvent(event, context);
            return;
        }

        if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            pointerInsideViewport = context.IsInsideWorldViewport(wheel->position);

            if (pointerInsideViewport) {
                cameraController.HandleEvent(event, context);

                worldCursor.setPosition(context.ScreenToWorld(wheel->position));
            }
        }
    }

  private:
    using TransformState = pipeframe::editor::SceneTransform;

    enum class EditorCommandType { Transform, PopulationSettings, CreateObject, DeleteObject };

    struct EditorCommand {
        EditorCommandType type;
        pipeframe::editor::SceneObjectData object;
        TransformState before;
        TransformState after;

        std::optional<pipeframe::editor::AgentPopulationSettings> populationBefore;
        std::optional<pipeframe::editor::AgentPopulationSettings> populationAfter;
    };

    std::filesystem::path GetDefaultScenePath() const {
        return std::filesystem::path(PIPEFRAME_ASSET_DIR).parent_path() / "scenes" / "workbench.pfscene";
    }

    void LoadScene() {
        if (!CanAuthorScene()) {
            return;
        }

        const std::filesystem::path scenePath = GetDefaultScenePath();

        std::string errorMessage;

        std::optional<pipeframe::editor::SceneDocument> loadedDocument =
            pipeframe::editor::SceneSerializer::Load(scenePath, &errorMessage);

        if (!loadedDocument.has_value()) {
            std::cerr << errorMessage << '\n';
            return;
        }

        // Do not let an unfinished drag survive across documents.f
        CancelObjectDrag(false);

        sceneDocument = std::move(*loadedDocument);

        RebuildRuntimeAgentsFromDocument();
        runtimePopulations.clear();
        RebuildHierarchyFromDocument();

        // Commands from the previous document must never affect
        // objects in the newly loaded document.
        undoHistory.clear();
        redoHistory.clear();

        SetSelectedObject(std::nullopt);

        // Continue generated names after the greatest loaded ID.
        nextAgentNameNumber = 1;

        for (const pipeframe::editor::SceneObjectData &object : sceneDocument.GetObjects()) {

            nextAgentNameNumber = std::max(nextAgentNameNumber, static_cast<std::size_t>(object.id) + 1);
        }

        viewportStatusInitialized = false;

        RefreshHistoryControls();
        RefreshDeleteControl();

        std::cout << "Loaded scene: " << scenePath << '\n';
    }

    void RestoreRuntimeFromDocument() {
        const std::optional<pipeframe::editor::SceneObjectId> previousSelection = selectedObjectId;

        RebuildRuntimeAgentsFromDocument();

        if (runtimePreviewActive) {
            RebuildRuntimePopulationsFromDocument();
        } else {
            runtimePopulations.clear();
        }

        if (previousSelection.has_value() && sceneDocument.FindObject(*previousSelection) != nullptr) {
            SetSelectedObject(previousSelection);
        } else {
            SetSelectedObject(std::nullopt);
        }
    }

    bool CanAuthorScene() const { return !simulationController.IsPlaying() && !runtimePreviewActive; }

    void RequestSingleSimulationStep() {
        if (simulationController.IsPlaying()) {
            return;
        }

        CancelObjectDrag(true);

        if (!runtimePreviewActive) {
            runtimePreviewActive = true;
            RestoreRuntimeFromDocument();
        }

        simulationController.RequestSingleStep();

        viewportStatusInitialized = false;

        RefreshSimulationControls();
        RefreshInspector();
    }

    void ResetRuntimePreview() {
        if (simulationController.IsPlaying()) {
            return;
        }

        CancelObjectDrag(true);

        runtimePreviewActive = false;

        RestoreRuntimeFromDocument();

        viewportStatusInitialized = false;

        RefreshSimulationControls();
        RefreshInspector();
    }

    void ToggleSimulationPlayPause() {
        CancelObjectDrag(true);

        if (simulationController.IsPlaying()) {
            simulationController.Pause();

            runtimePreviewActive = false;

            // Discard temporary Play-mode changes.
            RestoreRuntimeFromDocument();
        } else {
            runtimePreviewActive = true;

            // Every Play session begins from authored document data.
            RestoreRuntimeFromDocument();

            simulationController.Play();
        }

        viewportStatusInitialized = false;

        RefreshSimulationControls();
        RefreshInspector();
    }

    void SaveScene() {
        if (!CanAuthorScene()) {
            return;
        }

        const std::filesystem::path scenePath = GetDefaultScenePath();

        std::error_code directoryError;

        std::filesystem::create_directories(scenePath.parent_path(), directoryError);

        if (directoryError) {
            std::cerr << "Unable to create scene directory: " << directoryError.message() << '\n';

            return;
        }

        std::string errorMessage;

        if (!pipeframe::editor::SceneSerializer::Save(sceneDocument, scenePath, &errorMessage)) {

            std::cerr << errorMessage << '\n';
            return;
        }

        sceneDocument.MarkClean();

        // Force the toolbar to refresh its SAVED/UNSAVED state.
        viewportStatusInitialized = false;

        std::cout << "Saved scene: " << scenePath << '\n';
    }

    void InitializeSceneDocument() {
        if (!sceneDocument.GetObjects().empty()) {
            return;
        }

        sceneDocument.CreateObject("DEMO AGENT A", pipeframe::editor::SceneObjectType::DemoAgent,
                                   {{-140.0f, 0.0f}, 0.0f});

        sceneDocument.CreateObject("DEMO AGENT B", pipeframe::editor::SceneObjectType::DemoAgent,
                                   {{140.0f, 60.0f}, 0.0f});

        // These objects are the initial scene, not user edits.
        sceneDocument.MarkClean();

        RebuildRuntimeAgentsFromDocument();
        if (runtimePreviewActive) {
            RebuildRuntimePopulationsFromDocument();
        }
    }

    void RebuildHierarchyFromDocument() {
        if (hierarchyPanel == nullptr) {
            return;
        }

        std::vector<HierarchyPanel::Item> items;
        items.reserve(sceneDocument.GetObjects().size());

        for (const pipeframe::editor::SceneObjectData &object : sceneDocument.GetObjects()) {
            items.push_back({
                object.id,
                object.name,
            });
        }

        hierarchyPanel->SetItems(items);
        hierarchyPanel->SetSelectedObject(selectedObjectId);
    }

    void RebuildRuntimeAgentsFromDocument() {
        demoAgents.clear();

        for (const pipeframe::editor::SceneObjectData &object : sceneDocument.GetObjects()) {
            if (object.type != pipeframe::editor::SceneObjectType::DemoAgent) {
                continue;
            }

            demoAgents.emplace_back(object.id, object.name, object.transform.position);

            DemoAgent &agent = demoAgents.back();

            agent.SetRotation(object.transform.rotation);
            agent.SetSimulationPlaying(simulationController.IsPlaying());
        }
    }

    void RebuildRuntimePopulationsFromDocument() {
        runtimePopulations.clear();

        for (const pipeframe::editor::SceneObjectData &object : sceneDocument.GetObjects()) {

            if (object.type != pipeframe::editor::SceneObjectType::AgentPopulation) {
                continue;
            }

            runtimePopulations.emplace_back();

            if (!runtimePopulations.back().Initialize(object)) {
                runtimePopulations.pop_back();
                continue;
            }

            std::cout << "Compiled runtime population: " << object.name << " (" << runtimePopulations.back().GetCount()
                      << " agents)\n";
        }
    }

    void CancelObjectDrag(bool restoreTransform) {
        const bool hadActiveDrag = draggingSelectedObject;

        if (restoreTransform && draggingSelectedObject && dragStartTransform.has_value()) {
            if (DemoAgent *selected = GetSelectedAgent()) {
                ApplyTransform(*selected, *dragStartTransform);
            }
        }

        draggingSelectedObject = false;
        dragStartTransform.reset();

        if (hadActiveDrag) {
            RefreshInspector();
        }
    }

    void DeleteSelectedObject() {
        if (!CanAuthorScene() || !selectedObjectId.has_value()) {
            return;
        }

        CancelObjectDrag(true);

        const pipeframe::editor::SceneObjectData *selectedObject = sceneDocument.FindObject(*selectedObjectId);

        if (selectedObject == nullptr) {
            return;
        }

        const pipeframe::editor::SceneObjectId objectId = selectedObject->id;

        const EditorCommand command{
            EditorCommandType::DeleteObject,
            *selectedObject,
            selectedObject->transform,
            {},
        };

        if (RemoveObjectFromScene(objectId)) {
            RecordEditorCommand(command);
        }
    }

    void CreateDemoAgent() {
        if (!CanAuthorScene()) {
            return;
        }

        const std::string name = std::string("DEMO AGENT ") + std::to_string(nextAgentNameNumber++);

        const float offset = static_cast<float>(demoAgents.size()) * 35.0f;

        const TransformState initialTransform{
            {offset - 70.0f, offset - 70.0f},
            0.0f,
        };

        const DemoAgentId newId =
            sceneDocument.CreateObject(name, pipeframe::editor::SceneObjectType::DemoAgent, initialTransform);

        demoAgents.emplace_back(newId, name, initialTransform.position);

        DemoAgent &newAgent = demoAgents.back();

        newAgent.SetRotation(initialTransform.rotation);
        newAgent.SetSimulationPlaying(false);

        RebuildHierarchyFromDocument();
        SetSelectedObject(newId);

        const pipeframe::editor::SceneObjectData *createdObject = sceneDocument.FindObject(newId);

        if (createdObject != nullptr) {
            RecordEditorCommand({
                EditorCommandType::CreateObject,
                *createdObject,
                {},
                initialTransform,
            });
        }
    }

    void CreateAgentPopulation() {
        if (!CanAuthorScene()) {
            return;
        }

        const std::string name = std::string("ANT POPULATION ") + std::to_string(nextAgentNameNumber++);

        const TransformState initialTransform{
            {0.0f, 0.0f},
            0.0f,
        };

        const pipeframe::editor::AgentPopulationSettings settings{
            1'000'000,
            {4000.0f, 4000.0f},
            1,
        };

        const pipeframe::editor::SceneObjectId newId = sceneDocument.CreatePopulation(name, initialTransform, settings);

        RebuildHierarchyFromDocument();
        SetSelectedObject(newId);

        const pipeframe::editor::SceneObjectData *createdObject = sceneDocument.FindObject(newId);

        if (createdObject != nullptr) {
            RecordEditorCommand({
                EditorCommandType::CreateObject,
                *createdObject,
                {},
                initialTransform,
            });
        }
    }

    std::optional<pipeframe::editor::SceneObjectId>
    FindSelectionAfterRemoval(pipeframe::editor::SceneObjectId removedObjectId) const {

        std::optional<pipeframe::editor::SceneObjectId> previousObject;
        bool foundRemovedObject = false;

        for (const pipeframe::editor::SceneObjectData &object : sceneDocument.GetObjects()) {

            if (object.id == removedObjectId) {
                foundRemovedObject = true;
                continue;
            }

            if (foundRemovedObject) {
                return object.id;
            }

            previousObject = object.id;
        }

        return previousObject;
    }

    bool RemoveObjectFromScene(pipeframe::editor::SceneObjectId objectId) {

        if (sceneDocument.FindObject(objectId) == nullptr) {
            return false;
        }

        const bool removingSelectedObject = selectedObjectId.has_value() && *selectedObjectId == objectId;

        const std::optional<pipeframe::editor::SceneObjectId> nextSelection =
            removingSelectedObject ? FindSelectionAfterRemoval(objectId) : selectedObjectId;

        if (!sceneDocument.RemoveObject(objectId)) {
            return false;
        }

        std::erase_if(demoAgents, [objectId](const DemoAgent &agent) { return agent.GetId() == objectId; });

        RebuildHierarchyFromDocument();

        if (removingSelectedObject) {
            SetSelectedObject(nextSelection);
        } else if (hierarchyPanel != nullptr) {
            hierarchyPanel->SetSelectedObject(selectedObjectId);
        }

        return true;
    }

    bool RestoreObjectFromCommand(const EditorCommand &command, const TransformState &transform) {

        pipeframe::editor::SceneObjectData object = command.object;
        object.transform = transform;

        if (!sceneDocument.RestoreObject(object)) {
            return false;
        }

        if (object.type == pipeframe::editor::SceneObjectType::DemoAgent) {

            demoAgents.emplace_back(object.id, object.name, object.transform.position);

            DemoAgent &agent = demoAgents.back();

            agent.SetRotation(object.transform.rotation);
            agent.SetSimulationPlaying(simulationController.IsPlaying());
        }

        RebuildHierarchyFromDocument();
        SetSelectedObject(object.id);

        return true;
    }

    TransformState CaptureTransform(const DemoAgent &agent) const { return {agent.GetPosition(), agent.GetRotation()}; }

    void ApplyTransform(DemoAgent &agent, const TransformState &state) {
        agent.SetPosition(state.position);
        agent.SetRotation(state.rotation);

        sceneDocument.SetTransform(agent.GetId(), state);
    }

    bool AreTransformsEqual(const TransformState &first, const TransformState &second) const {
        return first.position == second.position && first.rotation == second.rotation;
    }

    bool ArePopulationSettingsEqual(const pipeframe::editor::AgentPopulationSettings &first,
                                    const pipeframe::editor::AgentPopulationSettings &second) const {

        return first.agentCount == second.agentCount && first.spawnAreaSize == second.spawnAreaSize &&
               first.randomSeed == second.randomSeed;
    }

    static std::uint32_t ToUnsignedInteger(float value, std::uint32_t minimumValue) {

        if (!std::isfinite(value)) {
            return minimumValue;
        }

        const double roundedValue = std::round(static_cast<double>(value));

        const double boundedValue = std::clamp(roundedValue, static_cast<double>(minimumValue),
                                               static_cast<double>(std::numeric_limits<std::uint32_t>::max()));

        return static_cast<std::uint32_t>(boundedValue);
    }

    void RecordPopulationChange(pipeframe::editor::SceneObjectId objectId,
                                const pipeframe::editor::AgentPopulationSettings &before,
                                const pipeframe::editor::AgentPopulationSettings &after) {

        if (!CanAuthorScene() || ArePopulationSettingsEqual(before, after)) {
            return;
        }

        if (!sceneDocument.SetPopulationSettings(objectId, after)) {
            return;
        }

        const pipeframe::editor::SceneObjectData *object = sceneDocument.FindObject(objectId);

        if (object == nullptr) {
            return;
        }

        RecordEditorCommand({
            EditorCommandType::PopulationSettings,
            *object,
            {},
            {},
            before,
            after,
        });
    }

    void
    ModifySelectedPopulation(const std::function<void(pipeframe::editor::AgentPopulationSettings &)> &modification) {

        if (!CanAuthorScene() || !selectedObjectId.has_value()) {
            RefreshInspector();
            return;
        }

        const pipeframe::editor::SceneObjectData *object = sceneDocument.FindObject(*selectedObjectId);

        if (object == nullptr || object->type != pipeframe::editor::SceneObjectType::AgentPopulation ||
            !object->population.has_value()) {

            RefreshInspector();
            return;
        }

        const pipeframe::editor::AgentPopulationSettings before = *object->population;

        pipeframe::editor::AgentPopulationSettings after = before;

        modification(after);

        RecordPopulationChange(object->id, before, after);
        RefreshInspector();
    }

    void RecordEditorCommand(const EditorCommand &command) {
        undoHistory.push_back(command);
        redoHistory.clear();

        RefreshHistoryControls();
    }

    void RecordTransformChange(DemoAgentId agentId, const TransformState &before, const TransformState &after) {

        if (!CanAuthorScene()) {
            return;
        }

        if (AreTransformsEqual(before, after)) {
            return;
        }

        sceneDocument.SetTransform(agentId, after);

        const pipeframe::editor::SceneObjectData *object = sceneDocument.FindObject(agentId);

        if (object == nullptr) {
            return;
        }

        RecordEditorCommand({
            EditorCommandType::Transform,
            *object,
            before,
            after,
        });
    }

    void Undo() {
        if (undoHistory.empty() || !CanAuthorScene()) {
            return;
        }

        const EditorCommand command = undoHistory.back();
        undoHistory.pop_back();

        bool commandApplied = false;

        switch (command.type) {
        case EditorCommandType::Transform:
            if (DemoAgent *agent = FindAgent(command.object.id)) {

                ApplyTransform(*agent, command.before);
                SetSelectedObject(command.object.id);
                commandApplied = true;
            }
            break;

        case EditorCommandType::PopulationSettings:
            if (command.populationBefore.has_value()) {
                commandApplied = sceneDocument.SetPopulationSettings(command.object.id, *command.populationBefore);

                if (commandApplied) {
                    SetSelectedObject(command.object.id);
                }
            }
            break;

        case EditorCommandType::DeleteObject:
            commandApplied = RestoreObjectFromCommand(command, command.before);
            break;

        case EditorCommandType::CreateObject:
            commandApplied = RemoveObjectFromScene(command.object.id);
            break;
        }

        if (commandApplied) {
            redoHistory.push_back(command);
        }

        RefreshHistoryControls();
    }

    void Redo() {
        if (redoHistory.empty() || !CanAuthorScene()) {
            return;
        }

        const EditorCommand command = redoHistory.back();
        redoHistory.pop_back();

        bool commandApplied = false;

        switch (command.type) {
        case EditorCommandType::Transform:
            if (DemoAgent *agent = FindAgent(command.object.id)) {

                ApplyTransform(*agent, command.after);
                SetSelectedObject(command.object.id);
                commandApplied = true;
            }
            break;

        case EditorCommandType::PopulationSettings:
            if (command.populationAfter.has_value()) {
                commandApplied = sceneDocument.SetPopulationSettings(command.object.id, *command.populationAfter);

                if (commandApplied) {
                    SetSelectedObject(command.object.id);
                }
            }
            break;

        case EditorCommandType::CreateObject:
            commandApplied = RestoreObjectFromCommand(command, command.after);
            break;

        case EditorCommandType::DeleteObject:
            commandApplied = RemoveObjectFromScene(command.object.id);
            break;
        }

        if (commandApplied) {
            undoHistory.push_back(command);
        }

        RefreshHistoryControls();
    }

    void RefreshHistoryControls() {
        if (viewportToolbar == nullptr) {
            return;
        }

        const bool canEdit = CanAuthorScene();

        viewportToolbar->SetHistoryEnabled(canEdit && !undoHistory.empty(), canEdit && !redoHistory.empty());
    }

    void RefreshInspector() {
        if (simulationPanel == nullptr) {
            return;
        }

        const pipeframe::editor::SceneObjectData *selectedObject =
            selectedObjectId.has_value() ? sceneDocument.FindObject(*selectedObjectId) : nullptr;

        if (selectedObject == nullptr) {
            simulationPanel->SetInspectorMode(InspectorMode::None);
            simulationPanel->SetTransformEnabled(false);
            simulationPanel->SetPopulationEnabled(false);
            return;
        }

        if (selectedObject->type == pipeframe::editor::SceneObjectType::AgentPopulation &&
            selectedObject->population.has_value()) {

            const pipeframe::editor::AgentPopulationSettings &settings = *selectedObject->population;

            simulationPanel->SetInspectorMode(InspectorMode::Population);

            simulationPanel->SetPopulationEnabled(CanAuthorScene());

            simulationPanel->SetPopulationValues(static_cast<float>(settings.agentCount), settings.spawnAreaSize,
                                                 static_cast<float>(settings.randomSeed));

            return;
        }

        simulationPanel->SetInspectorMode(InspectorMode::Transform);

        DemoAgent *selectedAgent = GetSelectedAgent();

        const bool canEditTransform = selectedAgent != nullptr && CanAuthorScene();

        simulationPanel->SetTransformEnabled(canEditTransform);

        if (selectedAgent != nullptr) {
            simulationPanel->SetTransformValues(selectedAgent->GetPosition(), selectedAgent->GetRotation());
        }
    }

    void RefreshViewportStatus(const RenderContext &context) {
        if (viewportToolbar == nullptr) {
            return;
        }

        const Camera2D &camera = context.GetCamera();

        const sf::Vector2f cameraCenter = camera.GetCenter();
        const float cameraZoom = camera.GetZoom();
        const bool sceneDirty = sceneDocument.IsDirty();

        if (viewportStatusInitialized && displayedCameraCenter == cameraCenter && displayedCameraZoom == cameraZoom &&
            displayedSceneDirty == sceneDirty) {

            return;
        }

        viewportStatusInitialized = true;
        displayedCameraCenter = cameraCenter;
        displayedCameraZoom = cameraZoom;
        displayedSceneDirty = sceneDirty;

        std::ostringstream stream;

        stream << std::fixed << std::setprecision(2) << "CAMERA " << cameraCenter.x << ", " << cameraCenter.y
               << "  |  ZOOM " << cameraZoom << "x  |  " << (sceneDirty ? "UNSAVED" : "SAVED");

        viewportToolbar->SetStatusText(stream.str());
    }

    void RefreshDeleteControl() {
        if (hierarchyPanel == nullptr) {
            return;
        }

        hierarchyPanel->SetSelectedObject(selectedObjectId);
        hierarchyPanel->SetAuthoringEnabled(CanAuthorScene());
    }

    void RefreshSimulationControls() {
        if (simulationPanel == nullptr) {
            return;
        }

        const bool playing = simulationController.IsPlaying();
        const bool previewing = runtimePreviewActive;

        if (controlsInitialized && displayedPlayingState == playing && displayedRuntimePreviewActive == previewing) {

            return;
        }

        controlsInitialized = true;
        displayedPlayingState = playing;
        displayedRuntimePreviewActive = previewing;

        simulationPanel->SetSimulationState(playing, previewing);

        const bool canAuthor = CanAuthorScene();

        if (hierarchyPanel != nullptr) {
            hierarchyPanel->SetAuthoringEnabled(canAuthor);
        }

        if (viewportToolbar != nullptr) {
            viewportToolbar->SetAuthoringEnabled(canAuthor);
        }

        RefreshDeleteControl();
        RefreshHistoryControls();
    }

    DemoAgent *FindAgent(DemoAgentId id) {
        for (DemoAgent &agent : demoAgents) {
            if (agent.GetId() == id) {
                return &agent;
            }
        }

        return nullptr;
    }

    DemoAgent *GetSelectedAgent() {
        if (!selectedObjectId.has_value()) {
            return nullptr;
        }

        return FindAgent(*selectedObjectId);
    }

    std::optional<DemoAgentId> HitTestAgents(sf::Vector2f worldPoint) const {
        // Reverse order means the last rendered object is selected first.
        for (auto iterator = demoAgents.rbegin(); iterator != demoAgents.rend(); ++iterator) {
            if (iterator->Contains(worldPoint)) {
                return iterator->GetId();
            }
        }

        return std::nullopt;
    }

    void SetSelectedObject(std::optional<pipeframe::editor::SceneObjectId> newSelection) {

        if (newSelection.has_value() && sceneDocument.FindObject(*newSelection) == nullptr) {
            newSelection.reset();
        }

        selectedObjectId = newSelection;

        for (DemoAgent &agent : demoAgents) {
            const bool selected = selectedObjectId.has_value() && agent.GetId() == *selectedObjectId;

            agent.SetSelected(selected);
        }

        if (hierarchyPanel != nullptr) {
            hierarchyPanel->SetSelectedObject(selectedObjectId);
        }

        if (simulationPanel != nullptr) {
            const pipeframe::editor::SceneObjectData *selectedObject =
                selectedObjectId.has_value() ? sceneDocument.FindObject(*selectedObjectId) : nullptr;

            simulationPanel->SetSelectionName(selectedObject != nullptr ? selectedObject->name : "");
        }

        RefreshInspector();
        RefreshDeleteControl();
    }

    SimulationController simulationController;
    CameraController2D cameraController;
    DiagnosticsOverlay diagnosticsOverlay;

    sf::CircleShape worldCursor;
    sf::RectangleShape viewportBorder;

    UIManager uiManager;

    ViewportToolbar *viewportToolbar = nullptr;
    SimulationPanel *simulationPanel = nullptr;
    HierarchyPanel *hierarchyPanel = nullptr;

    // SimulationController starts in Playing mode.
    bool runtimePreviewActive = true;
    bool displayedRuntimePreviewActive = false;

    pipeframe::editor::SceneDocument sceneDocument;
    pipeframe::editor::PopulationBoundsRenderer populationBoundsRenderer;
    float pendingPopulationMovementTimeMs = 0.0f;

    std::size_t nextAgentNameNumber = 3;

    std::vector<DemoAgent> demoAgents;
    std::vector<pipeframe::runtime::RuntimeAgentPopulation> runtimePopulations;

    pipeframe::runtime::RuntimePopulationPointRenderer runtimePopulationPointRenderer;

    std::optional<pipeframe::editor::SceneObjectId> selectedObjectId;

    bool controlsInitialized = false;
    bool displayedPlayingState = false;

    bool pointerInsideViewport = false;

    bool viewportStatusInitialized = false;

    sf::Vector2f displayedCameraCenter{0.0f, 0.0f};

    float displayedCameraZoom = 0.0f;
    bool displayedSceneDirty = false;

    Panel *metricsInputBlocker = nullptr;

    bool metricsVisible = false;

    bool draggingSelectedObject = false;
    sf::Vector2f selectedDragOffset{0.0f, 0.0f};

    std::vector<EditorCommand> undoHistory;
    std::vector<EditorCommand> redoHistory;

    std::optional<TransformState> dragStartTransform;
};

std::unique_ptr<Scene> CreateSimulationWorkbenchScene() { return std::make_unique<TestScene>(); }
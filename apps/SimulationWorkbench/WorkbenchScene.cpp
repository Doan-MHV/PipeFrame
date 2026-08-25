#include "WorkbenchScene.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <system_error>
#include <vector>

#include "DemoAgent.h"
#include "DiagnosticsOverlay.h"
#include "Editor/SceneDocument.h"
#include "Editor/SceneSerializer.h"
#include "PipeFrame/UI/TextButton.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <PipeFrame/Render/CameraController2D.h>

#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/Button.h>
#include <PipeFrame/UI/Label.h>
#include <PipeFrame/UI/LabeledNumericField.h>
#include <PipeFrame/UI/NumericField.h>
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

        if (metricsButton != nullptr) {
            metricsButton->SetText(metricsVisible ? "CLOSE" : "METRICS");
        }
    }

    void LayoutEditor(sf::Vector2u windowSize, RenderContext &context) {
        if (editorPanel == nullptr || hierarchyPanel == nullptr) {
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

        if (hierarchyListPanel != nullptr) {
            hierarchyListPanel->SetSize({HierarchyWidth - 24.0f, std::max(1.0f, viewportHeight - 84.0f)});
        }

        // Right Inspector panel.
        const float inspectorX = std::max(MinimumInspectorLeft, windowWidth - InspectorWidth - InspectorRightMargin);

        editorPanel->SetPosition({inspectorX, ViewportTop});

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

            constexpr float ControlsWidth = 372.0f;
            constexpr float RowHeight = 28.0f;

            // First row: editor buttons on the left and title in remaining space.
            if (viewportTitleLabel != nullptr) {
                const float titleWidth = std::max(1.0f, viewportWidth - ControlsWidth - 4.0f);

                viewportTitleLabel->SetPosition({ControlsWidth, 4.0f});

                viewportTitleLabel->SetSize({titleWidth, RowHeight});

                // On extremely narrow windows, hiding the title is cleaner.
                viewportTitleLabel->SetVisible(viewportWidth >= 500.0f);
            }

            // Second row: camera, zoom, and save status.
            if (viewportStatusLabel != nullptr) {
                viewportStatusLabel->SetPosition({4.0f, 36.0f});

                viewportStatusLabel->SetSize({std::max(1.0f, viewportWidth - 8.0f), RowHeight});
            }
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

        viewportToolbar = &uiManager.CreateRoot<Panel>();

        viewportToolbar->SetFillColor(sf::Color(30, 34, 43, 245));

        viewportToolbar->SetOutlineColor(sf::Color(78, 86, 104));

        viewportToolbar->SetOutlineThickness(1.0f);

        viewportTitleLabel = &viewportToolbar->CreateChild<Label>(uiManager.GetDefaultFont());

        viewportTitleLabel->SetText("SCENE VIEW");
        viewportTitleLabel->SetCharacterSize(13);
        viewportTitleLabel->SetAlignment(LabelAlignment::Center);

        viewportStatusLabel = &viewportToolbar->CreateChild<Label>(uiManager.GetDefaultFont());

        viewportStatusLabel->SetCharacterSize(12);
        viewportStatusLabel->SetAlignment(LabelAlignment::Right);

        viewportStatusLabel->SetHorizontalPadding(12.0f);

        metricsButton = &viewportToolbar->CreateChild<TextButton>(uiManager.GetDefaultFont());

        metricsButton->SetPosition({4.0f, 4.0f});
        metricsButton->SetSize({92.0f, 28.0f});
        metricsButton->SetText("METRICS");
        metricsButton->SetTextCharacterSize(11);
        metricsButton->SetOnClick([this]() { SetMetricsVisible(!metricsVisible); });

        undoButton = &viewportToolbar->CreateChild<TextButton>(uiManager.GetDefaultFont());

        undoButton->SetPosition({100.0f, 4.0f});
        undoButton->SetSize({68.0f, 28.0f});
        undoButton->SetText("UNDO");
        undoButton->SetTextCharacterSize(10);
        undoButton->SetOnClick([this]() { Undo(); });

        redoButton = &viewportToolbar->CreateChild<TextButton>(uiManager.GetDefaultFont());

        redoButton->SetPosition({172.0f, 4.0f});
        redoButton->SetSize({68.0f, 28.0f});
        redoButton->SetText("REDO");
        redoButton->SetTextCharacterSize(10);
        redoButton->SetOnClick([this]() { Redo(); });

        saveButton = &viewportToolbar->CreateChild<TextButton>(uiManager.GetDefaultFont());

        saveButton->SetPosition({244.0f, 4.0f});
        saveButton->SetSize({60.0f, 28.0f});
        saveButton->SetText("SAVE");
        saveButton->SetTextCharacterSize(10);
        saveButton->SetOnClick([this]() { SaveScene(); });

        loadButton = &viewportToolbar->CreateChild<TextButton>(uiManager.GetDefaultFont());

        loadButton->SetPosition({308.0f, 4.0f});
        loadButton->SetSize({60.0f, 28.0f});
        loadButton->SetText("LOAD");
        loadButton->SetTextCharacterSize(10);
        loadButton->SetOnClick([this]() { LoadScene(); });

        metricsInputBlocker = &uiManager.CreateRoot<Panel>();

        metricsInputBlocker->SetFillColor(sf::Color::Transparent);

        metricsInputBlocker->SetOutlineColor(sf::Color::Transparent);

        metricsInputBlocker->SetOutlineThickness(0.0f);

        SetMetricsVisible(false);

        hierarchyPanel = &uiManager.CreateRoot<Panel>();

        hierarchyPanel->SetSize({220.0f, 560.0f});
        hierarchyPanel->SetFillColor(sf::Color(24, 27, 34, 245));
        hierarchyPanel->SetOutlineColor(sf::Color(78, 86, 104));
        hierarchyPanel->SetOutlineThickness(1.0f);

        Panel &hierarchyHeader = hierarchyPanel->CreateChild<Panel>();

        hierarchyHeader.SetPosition({12.0f, 12.0f});
        hierarchyHeader.SetSize({196.0f, 48.0f});
        hierarchyHeader.SetFillColor(sf::Color(42, 47, 59));
        hierarchyHeader.SetOutlineColor(sf::Color(90, 100, 120));
        hierarchyHeader.SetOutlineThickness(1.0f);

        Label &hierarchyHeaderLabel = hierarchyHeader.CreateChild<Label>(uiManager.GetDefaultFont());

        hierarchyHeaderLabel.SetPosition({0.0f, 0.0f});
        hierarchyHeaderLabel.SetSize({100.0f, hierarchyHeader.GetSize().y});
        hierarchyHeaderLabel.SetText("HIERARCHY");
        hierarchyHeaderLabel.SetCharacterSize(14);
        hierarchyHeaderLabel.SetAlignment(LabelAlignment::Left);
        hierarchyHeaderLabel.SetHorizontalPadding(12.0f);

        hierarchyListPanel = &hierarchyPanel->CreateChild<StackPanel>();

        hierarchyListPanel->SetPosition({12.0f, 72.0f});
        hierarchyListPanel->SetSize({196.0f, 476.0f});
        hierarchyListPanel->SetFillColor(sf::Color(31, 34, 43));
        hierarchyListPanel->SetOutlineColor(sf::Color(65, 72, 88));
        hierarchyListPanel->SetOutlineThickness(1.0f);
        hierarchyListPanel->SetOrientation(StackOrientation::Vertical);
        hierarchyListPanel->SetPadding(Thickness{8.0f});
        hierarchyListPanel->SetSpacing(6.0f);

        addAgentButton = &hierarchyHeader.CreateChild<Button>();

        addAgentButton->SetPosition({148.0f, 6.0f});
        addAgentButton->SetSize({42.0f, 36.0f});

        addAgentButton->SetOnClick([this]() { CreateDemoAgent(); });

        deleteAgentButton = &hierarchyHeader.CreateChild<Button>();

        deleteAgentButton->SetPosition({104.0f, 6.0f});
        deleteAgentButton->SetSize({38.0f, 36.0f});
        deleteAgentButton->SetEnabled(false);

        deleteAgentButton->SetOnClick([this]() { DeleteSelectedAgent(); });

        Label &deleteAgentLabel = deleteAgentButton->CreateChild<Label>(uiManager.GetDefaultFont());

        deleteAgentLabel.SetPosition({0.0f, 0.0f});
        deleteAgentLabel.SetSize(deleteAgentButton->GetSize());
        deleteAgentLabel.SetText("-");
        deleteAgentLabel.SetCharacterSize(18);
        deleteAgentLabel.SetAlignment(LabelAlignment::Center);
        deleteAgentLabel.SetHitTestVisible(false);

        Label &addAgentLabel = addAgentButton->CreateChild<Label>(uiManager.GetDefaultFont());

        addAgentLabel.SetPosition({0.0f, 0.0f});
        addAgentLabel.SetSize(addAgentButton->GetSize());
        addAgentLabel.SetText("+");
        addAgentLabel.SetCharacterSize(18);
        addAgentLabel.SetAlignment(LabelAlignment::Center);
        addAgentLabel.SetHitTestVisible(false);

        for (DemoAgent &agent : demoAgents) {
            CreateHierarchyEntry(*hierarchyListPanel, agent);
        }

        editorPanel = &uiManager.CreateRoot<Panel>();

        editorPanel->SetSize({300.0f, 560.0f});
        editorPanel->SetFillColor(sf::Color(24, 27, 34, 245));
        editorPanel->SetOutlineColor(sf::Color(78, 86, 104));
        editorPanel->SetOutlineThickness(1.0f);

        Panel &headerPanel = editorPanel->CreateChild<Panel>();

        headerPanel.SetPosition({12.0f, 12.0f});
        headerPanel.SetSize({276.0f, 48.0f});
        headerPanel.SetFillColor(sf::Color(42, 47, 59));
        headerPanel.SetOutlineColor(sf::Color(90, 100, 120));
        headerPanel.SetOutlineThickness(1.0f);

        Label &headerLabel = headerPanel.CreateChild<Label>(uiManager.GetDefaultFont());

        headerLabel.SetPosition({0.0f, 0.0f});
        headerLabel.SetSize(headerPanel.GetSize());
        headerLabel.SetText("SIMULATION");
        headerLabel.SetCharacterSize(15);
        headerLabel.SetAlignment(LabelAlignment::Left);
        headerLabel.SetHorizontalPadding(12.0f);

        StackPanel &contentPanel = editorPanel->CreateChild<StackPanel>();

        contentPanel.SetPosition({12.0f, 72.0f});
        contentPanel.SetSize({276.0f, 476.0f});
        contentPanel.SetFillColor(sf::Color(31, 34, 43));
        contentPanel.SetOutlineColor(sf::Color(65, 72, 88));
        contentPanel.SetOutlineThickness(1.0f);

        contentPanel.SetOrientation(StackOrientation::Vertical);
        contentPanel.SetPadding(Thickness{12.0f});
        contentPanel.SetSpacing(8.0f);

        playPauseButton = &contentPanel.CreateChild<TextButton>(uiManager.GetDefaultFont());

        playPauseButton->SetSize({0.0f, 44.0f});
        playPauseButton->SetTextCharacterSize(14);
        playPauseButton->SetOnClick([this]() { ToggleSimulationPlayPause(); });

        singleStepButton = &contentPanel.CreateChild<TextButton>(uiManager.GetDefaultFont());

        singleStepButton->SetSize({0.0f, 44.0f});
        singleStepButton->SetText("SINGLE STEP");
        singleStepButton->SetTextCharacterSize(14);
        singleStepButton->SetOnClick([this]() { RequestSingleSimulationStep(); });

        resetPreviewButton = &contentPanel.CreateChild<TextButton>(uiManager.GetDefaultFont());

        resetPreviewButton->SetSize({0.0f, 44.0f});
        resetPreviewButton->SetText("RESET");
        resetPreviewButton->SetTextCharacterSize(14);
        resetPreviewButton->SetOnClick([this]() { ResetRuntimePreview(); });

        selectionStatusLabel = &contentPanel.CreateChild<Label>(uiManager.GetDefaultFont());

        selectionStatusLabel->SetSize({0.0f, 36.0f});
        selectionStatusLabel->SetCharacterSize(12);
        selectionStatusLabel->SetAlignment(LabelAlignment::Left);
        selectionStatusLabel->SetHorizontalPadding(8.0f);
        selectionStatusLabel->SetColor(sf::Color(180, 190, 210));

        Label &transformHeader = contentPanel.CreateChild<Label>(uiManager.GetDefaultFont());

        transformHeader.SetSize({0.0f, 28.0f});
        transformHeader.SetText("TRANSFORM");
        transformHeader.SetCharacterSize(12);
        transformHeader.SetAlignment(LabelAlignment::Left);
        transformHeader.SetHorizontalPadding(8.0f);
        transformHeader.SetColor(sf::Color(225, 230, 240));

        positionXField = &contentPanel.CreateChild<LabeledNumericField>(uiManager.GetDefaultFont(), "POSITION X");

        positionXField->SetSize({0.0f, 36.0f});

        positionYField = &contentPanel.CreateChild<LabeledNumericField>(uiManager.GetDefaultFont(), "POSITION Y");

        positionYField->SetSize({0.0f, 36.0f});

        rotationField = &contentPanel.CreateChild<LabeledNumericField>(uiManager.GetDefaultFont(), "ROTATION");

        rotationField->SetSize({0.0f, 36.0f});

        positionXField->SetOnValueCommitted([this](float newPositionX) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                sf::Vector2f position = agent->GetPosition();

                position.x = newPositionX;
                agent->SetPosition(position);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        positionYField->SetOnValueCommitted([this](float newPositionY) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                sf::Vector2f position = agent->GetPosition();

                position.y = newPositionY;
                agent->SetPosition(position);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        rotationField->SetOnValueCommitted([this](float newRotation) {
            if (DemoAgent *agent = GetSelectedAgent()) {
                const TransformState before = CaptureTransform(*agent);

                agent->SetRotation(newRotation);

                RecordTransformChange(agent->GetId(), before, CaptureTransform(*agent));
            }

            RefreshInspector();
        });

        RefreshSimulationControls();
        SetSelectedAgent(std::nullopt);
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
                DeleteSelectedAgent();
                return;
            }

            if (!primaryModifier && !keyPressed->alt && keyPressed->code == sf::Keyboard::Key::Escape) {
                if (draggingSelectedObject) {
                    CancelObjectDrag(true);
                } else {
                    SetSelectedAgent(std::nullopt);
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

                SetSelectedAgent(hitAgent);

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
    struct HierarchyEntry {
        DemoAgentId agentId;
        Button *button = nullptr;
        Label *label = nullptr;
    };

    using TransformState = pipeframe::editor::SceneTransform;

    enum class EditorCommandType { Transform, CreateAgent, DeleteAgent };

    struct EditorCommand {
        EditorCommandType type;
        DemoAgentId agentId;
        std::string agentName;
        TransformState before;
        TransformState after;
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

        // Do not let an unfinished drag survive across documents.
        CancelObjectDrag(false);

        sceneDocument = std::move(*loadedDocument);

        RebuildRuntimeAgentsFromDocument();
        RebuildHierarchyFromRuntimeAgents();

        // Commands from the previous document must never affect
        // objects in the newly loaded document.
        undoHistory.clear();
        redoHistory.clear();

        SetSelectedAgent(std::nullopt);

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
        const std::optional<DemoAgentId> previousSelection = selectedAgentId;

        RebuildRuntimeAgentsFromDocument();

        if (previousSelection.has_value() && FindAgent(*previousSelection) != nullptr) {

            SetSelectedAgent(previousSelection);
        } else {
            SetSelectedAgent(std::nullopt);
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
    }

    void RebuildHierarchyFromRuntimeAgents() {
        if (hierarchyListPanel == nullptr) {
            return;
        }

        // Old retained UI children cannot currently be destroyed,
        // so hide every old entry first.
        for (HierarchyEntry &entry : hierarchyEntries) {
            if (entry.button != nullptr) {
                entry.button->SetVisible(false);
            }
        }

        for (DemoAgent &agent : demoAgents) {
            if (HierarchyEntry *entry = FindHierarchyEntry(agent.GetId())) {
                entry->button->SetVisible(true);

                if (entry->label != nullptr) {
                    entry->label->SetText(agent.GetName());
                }
            } else {
                CreateHierarchyEntry(*hierarchyListPanel, agent);
            }
        }

        hierarchyListPanel->RefreshLayout();
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

    void DeleteSelectedAgent() {
        if (!CanAuthorScene()) {
            return;
        }

        CancelObjectDrag(true);

        DemoAgent *agent = GetSelectedAgent();

        if (agent == nullptr) {
            return;
        }

        const EditorCommand command{
            EditorCommandType::DeleteAgent, agent->GetId(), agent->GetName(), CaptureTransform(*agent), {},
        };

        if (RemoveAgentFromScene(command.agentId)) {
            RecordEditorCommand(command);
        }
    }

    void CreateDemoAgent() {
        if (!CanAuthorScene() || hierarchyListPanel == nullptr) {
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

        CreateHierarchyEntry(*hierarchyListPanel, newAgent);
        SetSelectedAgent(newId);

        RecordEditorCommand({
            EditorCommandType::CreateAgent,
            newId,
            name,
            {},
            initialTransform,
        });
    }

    void CreateHierarchyEntry(StackPanel &list, DemoAgent &agent) {
        Button &button = list.CreateChild<Button>();

        button.SetSize({0.0f, 38.0f});

        const DemoAgentId id = agent.GetId();

        button.SetOnClick([this, id]() { SetSelectedAgent(id); });

        Label &label = button.CreateChild<Label>(uiManager.GetDefaultFont());

        label.SetPosition({0.0f, 0.0f});
        label.SetSize(button.GetSize());
        label.SetText(agent.GetName());
        label.SetCharacterSize(11);
        label.SetAlignment(LabelAlignment::Left);
        label.SetHorizontalPadding(10.0f);
        label.SetHitTestVisible(false);

        hierarchyEntries.push_back({id, &button, &label});
    }

    HierarchyEntry *FindHierarchyEntry(DemoAgentId agentId) {
        for (HierarchyEntry &entry : hierarchyEntries) {
            if (entry.agentId == agentId) {
                return &entry;
            }
        }

        return nullptr;
    }

    std::optional<DemoAgentId> FindSelectionAfterRemoval(DemoAgentId removedAgentId) const {
        std::optional<DemoAgentId> previousVisibleAgent;
        bool foundRemovedAgent = false;

        for (const HierarchyEntry &entry : hierarchyEntries) {
            if (entry.button == nullptr || !entry.button->IsVisible()) {
                continue;
            }

            if (entry.agentId == removedAgentId) {
                foundRemovedAgent = true;
                continue;
            }

            // Prefer the next visible entry.
            if (foundRemovedAgent) {
                return entry.agentId;
            }

            previousVisibleAgent = entry.agentId;
        }

        // The removed entry was last, so use the previous one.
        return previousVisibleAgent;
    }

    bool RemoveAgentFromScene(DemoAgentId agentId) {
        if (FindAgent(agentId) == nullptr) {
            return false;
        }

        if (!sceneDocument.RemoveObject(agentId)) {
            return false;
        }

        const bool removingSelectedAgent = selectedAgentId.has_value() && *selectedAgentId == agentId;

        const std::optional<DemoAgentId> nextSelection =
            removingSelectedAgent ? FindSelectionAfterRemoval(agentId) : selectedAgentId;

        std::erase_if(demoAgents, [agentId](const DemoAgent &agent) { return agent.GetId() == agentId; });

        if (HierarchyEntry *entry = FindHierarchyEntry(agentId)) {
            entry->button->SetVisible(false);
        }

        if (hierarchyListPanel != nullptr) {
            hierarchyListPanel->RefreshLayout();
        }

        if (removingSelectedAgent) {
            SetSelectedAgent(nextSelection);
        } else {
            RefreshHierarchy();
        }

        return true;
    }

    bool RestoreAgentFromCommand(const EditorCommand &command, const TransformState &transform) {
        if (FindAgent(command.agentId) != nullptr || hierarchyListPanel == nullptr) {
            return false;
        }

        const pipeframe::editor::SceneObjectData object{
            command.agentId,
            command.agentName,
            pipeframe::editor::SceneObjectType::DemoAgent,
            transform,
        };

        if (!sceneDocument.RestoreObject(object)) {
            return false;
        }

        demoAgents.emplace_back(command.agentId, command.agentName, transform.position);

        DemoAgent &agent = demoAgents.back();

        agent.SetRotation(transform.rotation);
        agent.SetSimulationPlaying(simulationController.IsPlaying());

        if (HierarchyEntry *entry = FindHierarchyEntry(command.agentId)) {
            entry->button->SetVisible(true);
        } else {
            CreateHierarchyEntry(*hierarchyListPanel, agent);
        }

        hierarchyListPanel->RefreshLayout();
        SetSelectedAgent(command.agentId);

        return true;
    }

    void RefreshHierarchy() {
        for (HierarchyEntry &entry : hierarchyEntries) {
            const bool selected = selectedAgentId.has_value() && entry.agentId == *selectedAgentId;

            entry.button->SetNormalColor(selected ? sf::Color(48, 73, 110) : sf::Color(37, 41, 51));

            entry.button->SetOutlineColor(selected ? sf::Color(245, 179, 103) : sf::Color(76, 84, 102));
        }
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

        RecordEditorCommand({
            EditorCommandType::Transform,
            agentId,
            {},
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
            if (DemoAgent *agent = FindAgent(command.agentId)) {
                ApplyTransform(*agent, command.before);
                SetSelectedAgent(command.agentId);
                commandApplied = true;
            }
            break;

        case EditorCommandType::DeleteAgent:
            commandApplied = RestoreAgentFromCommand(command, command.before);
            break;

        case EditorCommandType::CreateAgent:
            commandApplied = RemoveAgentFromScene(command.agentId);
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
            if (DemoAgent *agent = FindAgent(command.agentId)) {
                ApplyTransform(*agent, command.after);
                SetSelectedAgent(command.agentId);
                commandApplied = true;
            }
            break;

        case EditorCommandType::CreateAgent:
            commandApplied = RestoreAgentFromCommand(command, command.after);
            break;

        case EditorCommandType::DeleteAgent:
            commandApplied = RemoveAgentFromScene(command.agentId);
            break;
        }

        if (commandApplied) {
            undoHistory.push_back(command);
        }

        RefreshHistoryControls();
    }

    void RefreshHistoryControls() {
        const bool canEdit = CanAuthorScene();

        if (undoButton != nullptr) {
            undoButton->SetEnabled(canEdit && !undoHistory.empty());
        }

        if (redoButton != nullptr) {
            redoButton->SetEnabled(canEdit && !redoHistory.empty());
        }
    }

    void RefreshInspector() {
        if (positionXField == nullptr || positionYField == nullptr || rotationField == nullptr) {
            return;
        }

        DemoAgent *selected = GetSelectedAgent();

        const bool hasSelection = selected != nullptr;

        const bool canEditTransform = hasSelection && CanAuthorScene();

        positionXField->SetEnabled(canEditTransform);
        positionYField->SetEnabled(canEditTransform);
        rotationField->SetEnabled(canEditTransform);

        if (selected == nullptr) {
            return;
        }

        const sf::Vector2f position = selected->GetPosition();

        if (!positionXField->IsEditing()) {
            positionXField->SetValue(position.x);
        }

        if (!positionYField->IsEditing()) {
            positionYField->SetValue(position.y);
        }

        if (!rotationField->IsEditing()) {
            rotationField->SetValue(selected->GetRotation());
        }
    }

    void RefreshViewportStatus(const RenderContext &context) {
        if (viewportStatusLabel == nullptr) {
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

        viewportStatusLabel->SetText(stream.str());
    }

    void RefreshDeleteControl() {
        if (deleteAgentButton == nullptr) {
            return;
        }

        const bool canDelete = CanAuthorScene() && selectedAgentId.has_value();

        deleteAgentButton->SetEnabled(canDelete);
    }

    void RefreshSimulationControls() {
        if (playPauseButton == nullptr || singleStepButton == nullptr || resetPreviewButton == nullptr) {

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

        playPauseButton->SetText(playing ? "PAUSE" : "PLAY");

        // Multiple preview steps are allowed.
        singleStepButton->SetEnabled(!playing);

        // Reset only makes sense after one or more preview steps.
        resetPreviewButton->SetEnabled(!playing && previewing);

        const bool canAuthor = CanAuthorScene();

        if (addAgentButton != nullptr) {
            addAgentButton->SetEnabled(canAuthor);
        }

        if (saveButton != nullptr) {
            saveButton->SetEnabled(canAuthor);
        }

        if (loadButton != nullptr) {
            loadButton->SetEnabled(canAuthor);
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
        if (!selectedAgentId.has_value()) {
            return nullptr;
        }

        return FindAgent(*selectedAgentId);
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

    void SetSelectedAgent(std::optional<DemoAgentId> newSelection) {
        selectedAgentId = newSelection;

        for (DemoAgent &agent : demoAgents) {
            agent.SetSelected(selectedAgentId.has_value() && agent.GetId() == *selectedAgentId);
        }

        RefreshHierarchy();

        if (selectionStatusLabel != nullptr) {
            if (DemoAgent *selected = GetSelectedAgent()) {
                selectionStatusLabel->SetText(std::string("SELECTED: ") + selected->GetName());
            } else {
                selectionStatusLabel->SetText("SELECTED: NONE");
            }
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

    Panel *editorPanel = nullptr;
    LabeledNumericField *positionXField = nullptr;
    LabeledNumericField *positionYField = nullptr;
    LabeledNumericField *rotationField = nullptr;
    Button *addAgentButton = nullptr;
    Button *deleteAgentButton = nullptr;
    TextButton *undoButton = nullptr;
    TextButton *redoButton = nullptr;
    TextButton *saveButton = nullptr;
    TextButton *loadButton = nullptr;
    TextButton *playPauseButton = nullptr;
    TextButton *singleStepButton = nullptr;
    TextButton *resetPreviewButton = nullptr;
    TextButton *metricsButton = nullptr;

    // SimulationController starts in Playing mode.
    bool runtimePreviewActive = true;
    bool displayedRuntimePreviewActive = false;

    pipeframe::editor::SceneDocument sceneDocument;

    std::size_t nextAgentNameNumber = 3;

    std::vector<DemoAgent> demoAgents;

    std::optional<DemoAgentId> selectedAgentId;

    bool controlsInitialized = false;
    bool displayedPlayingState = false;

    bool pointerInsideViewport = false;

    Panel *viewportToolbar = nullptr;

    Label *viewportTitleLabel = nullptr;
    Label *viewportStatusLabel = nullptr;

    bool viewportStatusInitialized = false;

    sf::Vector2f displayedCameraCenter{0.0f, 0.0f};

    float displayedCameraZoom = 0.0f;
    bool displayedSceneDirty = false;

    Panel *metricsInputBlocker = nullptr;

    bool metricsVisible = false;

    Label *selectionStatusLabel = nullptr;

    bool draggingSelectedObject = false;
    sf::Vector2f selectedDragOffset{0.0f, 0.0f};

    Panel *hierarchyPanel = nullptr;
    StackPanel *hierarchyListPanel = nullptr;

    std::vector<HierarchyEntry> hierarchyEntries;

    std::vector<EditorCommand> undoHistory;
    std::vector<EditorCommand> redoHistory;

    std::optional<TransformState> dragStartTransform;
};

std::unique_ptr<Scene> CreateSimulationWorkbenchScene() { return std::make_unique<TestScene>(); }
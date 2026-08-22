#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include "DemoAgent.h"
#include "DiagnosticsOverlay.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <PipeFrame/Render/CameraController2D.h>

#include <PipeFrame/Core/Application.h>
#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/Button.h>
#include <PipeFrame/UI/Label.h>
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

        if (metricsButtonLabel != nullptr) {
            metricsButtonLabel->SetText(metricsVisible ? "CLOSE" : "METRICS");
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

        constexpr float ToolbarHeight = 36.0f;
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

            if (viewportTitleLabel != nullptr) {
                viewportTitleLabel->SetPosition({0.0f, 0.0f});
                viewportTitleLabel->SetSize(toolbarSize);
            }

            if (viewportStatusLabel != nullptr) {
                viewportStatusLabel->SetPosition({0.0f, 0.0f});
                viewportStatusLabel->SetSize(toolbarSize);
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

        metricsButton = &viewportToolbar->CreateChild<Button>();
        metricsButton->SetPosition({4.0f, 4.0f});
        metricsButton->SetSize({92.0f, 28.0f});

        metricsButton->SetOnClick([this]() { SetMetricsVisible(!metricsVisible); });

        metricsButtonLabel = &metricsButton->CreateChild<Label>(uiManager.GetDefaultFont());

        metricsButtonLabel->SetPosition({0.0f, 0.0f});
        metricsButtonLabel->SetSize(metricsButton->GetSize());

        metricsButtonLabel->SetText("METRICS");
        metricsButtonLabel->SetCharacterSize(11);
        metricsButtonLabel->SetAlignment(LabelAlignment::Center);

        undoButton = &viewportToolbar->CreateChild<Button>();

        undoButton->SetPosition({100.0f, 4.0f});
        undoButton->SetSize({68.0f, 28.0f});
        undoButton->SetOnClick([this]() { Undo(); });

        Label &undoLabel = undoButton->CreateChild<Label>(uiManager.GetDefaultFont());

        undoLabel.SetPosition({0.0f, 0.0f});
        undoLabel.SetSize(undoButton->GetSize());
        undoLabel.SetText("UNDO");
        undoLabel.SetCharacterSize(10);
        undoLabel.SetAlignment(LabelAlignment::Center);
        undoLabel.SetHitTestVisible(false);

        redoButton = &viewportToolbar->CreateChild<Button>();

        redoButton->SetPosition({172.0f, 4.0f});
        redoButton->SetSize({68.0f, 28.0f});
        redoButton->SetOnClick([this]() { Redo(); });

        Label &redoLabel = redoButton->CreateChild<Label>(uiManager.GetDefaultFont());

        redoLabel.SetPosition({0.0f, 0.0f});
        redoLabel.SetSize(redoButton->GetSize());
        redoLabel.SetText("REDO");
        redoLabel.SetCharacterSize(10);
        redoLabel.SetAlignment(LabelAlignment::Center);
        redoLabel.SetHitTestVisible(false);

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
        hierarchyHeaderLabel.SetSize(hierarchyHeader.GetSize());
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

        Button &pauseButton = contentPanel.CreateChild<Button>();

        pauseButton.SetSize({0.0f, 44.0f});
        pauseButton.SetOnClick([this]() { simulationController.TogglePlayPause(); });

        playPauseLabel = &pauseButton.CreateChild<Label>(uiManager.GetDefaultFont());

        playPauseLabel->SetPosition({0.0f, 0.0f});
        playPauseLabel->SetSize(pauseButton.GetSize());
        playPauseLabel->SetCharacterSize(14);
        playPauseLabel->SetAlignment(LabelAlignment::Center);

        singleStepButton = &contentPanel.CreateChild<Button>();

        singleStepButton->SetSize({0.0f, 44.0f});

        singleStepButton->SetOnClick([this]() { simulationController.RequestSingleStep(); });

        Label &stepLabel = singleStepButton->CreateChild<Label>(uiManager.GetDefaultFont());

        stepLabel.SetPosition({0.0f, 0.0f});
        stepLabel.SetSize(singleStepButton->GetSize());
        stepLabel.SetText("SINGLE STEP");
        stepLabel.SetCharacterSize(14);
        stepLabel.SetAlignment(LabelAlignment::Center);

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

        auto createTransformField = [this, &contentPanel](const char *captionText) -> NumericField & {
            StackPanel &row = contentPanel.CreateChild<StackPanel>();

            row.SetSize({0.0f, 36.0f});
            row.SetFillColor(sf::Color::Transparent);
            row.SetOutlineColor(sf::Color::Transparent);
            row.SetOutlineThickness(0.0f);
            row.SetHitTestVisible(false);

            row.SetOrientation(StackOrientation::Horizontal);
            row.SetSpacing(8.0f);

            Label &caption = row.CreateChild<Label>(uiManager.GetDefaultFont());

            caption.SetSize({82.0f, 0.0f});
            caption.SetText(captionText);
            caption.SetCharacterSize(11);
            caption.SetAlignment(LabelAlignment::Left);
            caption.SetHorizontalPadding(4.0f);
            caption.SetColor(sf::Color(150, 160, 180));

            NumericField &field = row.CreateChild<NumericField>(uiManager.GetDefaultFont());

            field.SetSize({154.0f, 0.0f});

            return field;
        };

        positionXField = &createTransformField("POSITION X");
        positionYField = &createTransformField("POSITION Y");
        rotationField = &createTransformField("ROTATION");

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
                simulationController.TogglePlayPause();
            }

            if (Input::WasKeyPressed(Key::Period)) {
                simulationController.RequestSingleStep();
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
            draggingSelectedObject = false;
            dragStartTransform.reset();
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

                if (DemoAgent *selected = GetSelectedAgent()) {
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

    struct TransformState {
        sf::Vector2f position{0.0f, 0.0f};
        float rotation = 0.0f;
    };

    struct TransformCommand {
        DemoAgentId agentId;
        TransformState before;
        TransformState after;
    };

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
    }

    bool AreTransformsEqual(const TransformState &first, const TransformState &second) const {
        return first.position == second.position && first.rotation == second.rotation;
    }

    void RecordTransformChange(DemoAgentId agentId, const TransformState &before, const TransformState &after) {
        if (AreTransformsEqual(before, after)) {
            return;
        }

        undoHistory.push_back({agentId, before, after});

        redoHistory.clear();

        RefreshHistoryControls();
    }

    void Undo() {
        if (undoHistory.empty()) {
            return;
        }

        const TransformCommand command = undoHistory.back();

        undoHistory.pop_back();

        if (DemoAgent *agent = FindAgent(command.agentId)) {
            ApplyTransform(*agent, command.before);
            SetSelectedAgent(command.agentId);

            redoHistory.push_back(command);
        }

        RefreshHistoryControls();
    }

    void Redo() {
        if (redoHistory.empty()) {
            return;
        }

        const TransformCommand command = redoHistory.back();

        redoHistory.pop_back();

        if (DemoAgent *agent = FindAgent(command.agentId)) {
            ApplyTransform(*agent, command.after);
            SetSelectedAgent(command.agentId);

            undoHistory.push_back(command);
        }

        RefreshHistoryControls();
    }

    void RefreshHistoryControls() {
        if (undoButton != nullptr) {
            undoButton->SetEnabled(!undoHistory.empty());
        }

        if (redoButton != nullptr) {
            redoButton->SetEnabled(!redoHistory.empty());
        }
    }

    void RefreshInspector() {
        if (positionXField == nullptr || positionYField == nullptr || rotationField == nullptr) {
            return;
        }

        DemoAgent *selected = GetSelectedAgent();

        const bool hasSelection = selected != nullptr;

        positionXField->SetEnabled(hasSelection);
        positionYField->SetEnabled(hasSelection);
        rotationField->SetEnabled(hasSelection && !simulationController.IsPlaying());

        if (selected == nullptr) {
            return;
        }

        const sf::Vector2f position = selected->GetPosition();

        if (!positionXField->HasKeyboardFocus()) {
            positionXField->SetValue(position.x);
        }

        if (!positionYField->HasKeyboardFocus()) {
            positionYField->SetValue(position.y);
        }

        if (!rotationField->HasKeyboardFocus()) {
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

        if (viewportStatusInitialized && displayedCameraCenter == cameraCenter && displayedCameraZoom == cameraZoom) {
            return;
        }

        viewportStatusInitialized = true;
        displayedCameraCenter = cameraCenter;
        displayedCameraZoom = cameraZoom;

        std::ostringstream stream;

        stream << std::fixed << std::setprecision(2) << "CAMERA " << cameraCenter.x << ", " << cameraCenter.y
               << "  |  ZOOM " << cameraZoom << "x";

        viewportStatusLabel->SetText(stream.str());
    }

    void RefreshSimulationControls() {
        if (playPauseLabel == nullptr || singleStepButton == nullptr) {
            return;
        }

        const bool playing = simulationController.IsPlaying();

        if (controlsInitialized && displayedPlayingState == playing) {
            return;
        }

        controlsInitialized = true;
        displayedPlayingState = playing;

        playPauseLabel->SetText(playing ? "PAUSE" : "PLAY");

        singleStepButton->SetEnabled(!playing);
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
    }

    SimulationController simulationController;
    CameraController2D cameraController;
    DiagnosticsOverlay diagnosticsOverlay;

    sf::CircleShape worldCursor;
    sf::RectangleShape viewportBorder;

    UIManager uiManager;

    Panel *editorPanel = nullptr;
    Label *playPauseLabel = nullptr;
    Button *singleStepButton = nullptr;
    NumericField *positionXField = nullptr;
    NumericField *positionYField = nullptr;
    NumericField *rotationField = nullptr;
    Button *undoButton = nullptr;
    Button *redoButton = nullptr;

    std::vector<DemoAgent> demoAgents{DemoAgent{1, "DEMO AGENT A", {-140.0f, 0.0f}},
                                      DemoAgent{2, "DEMO AGENT B", {140.0f, 60.0f}}};

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

    Button *metricsButton = nullptr;
    Label *metricsButtonLabel = nullptr;

    Panel *metricsInputBlocker = nullptr;

    bool metricsVisible = false;

    Label *selectionStatusLabel = nullptr;

    bool draggingSelectedObject = false;
    sf::Vector2f selectedDragOffset{0.0f, 0.0f};

    Panel *hierarchyPanel = nullptr;
    StackPanel *hierarchyListPanel = nullptr;

    std::vector<HierarchyEntry> hierarchyEntries;

    std::vector<TransformCommand> undoHistory;
    std::vector<TransformCommand> redoHistory;

    std::optional<TransformState> dragStartTransform;
};

int main() {
    Application app(1280, 720, "PipeFrame - Simulation Workbench");
    app.SetScene(std::make_unique<TestScene>());
    app.Run();

    return 0;
}

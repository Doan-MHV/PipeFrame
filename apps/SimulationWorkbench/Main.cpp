#include <algorithm>
#include <iostream>
#include <memory>

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
#include <PipeFrame/UI/Panel.h>
#include <PipeFrame/UI/StackPanel.h>
#include <PipeFrame/UI/UIManager.h>

class TestScene final : public Scene {
  public:
    void LayoutEditor(sf::Vector2u windowSize) {
        if (editorPanel == nullptr) {
            return;
        }

        constexpr float PanelWidth = 300.0f;
        constexpr float RightMargin = 80.0f;
        constexpr float MinimumLeftMargin = 12.0f;
        constexpr float TopMargin = 80.0f;

        const float windowWidth = static_cast<float>(windowSize.x);

        const float panelX = std::max(MinimumLeftMargin, windowWidth - PanelWidth - RightMargin);

        editorPanel->SetPosition({panelX, TopMargin});
    }

    void OnResize(sf::Vector2u newSize, RenderContext &context) override {
        (void)context;
        LayoutEditor(newSize);
    }

    void Load() override {
        const std::filesystem::path fontPath =
            std::filesystem::path(PIPEFRAME_ASSET_DIR) / "fonts/roboto_mono_semi.ttf";

        if (!diagnosticsOverlay.Load(fontPath)) {
            std::cerr << "Unable to load diagnostics font: " << fontPath << '\n';
        }

        if (!uiManager.LoadDefaultFont(fontPath)) {
            std::cerr << "Unable to load UI font: " << fontPath << '\n';
        }

        worldCursor.setRadius(8.0f);
        worldCursor.setOrigin({8.0f, 8.0f});
        worldCursor.setFillColor(sf::Color::Transparent);
        worldCursor.setOutlineColor(sf::Color::Green);
        worldCursor.setOutlineThickness(2.0f);

        circle.setRadius(48.0f);
        circle.setOrigin({48.0f, 48.0f});
        circle.setPosition({0.0f, 0.0f});
        circle.setFillColor(sf::Color(232, 91, 116));

        panel.setSize({220.0f, 120.0f});
        panel.setOrigin({110.0f, 60.0f});
        panel.setPosition({0.0f, 0.0f});
        panel.setFillColor(sf::Color(55, 55, 55));
        panel.setOutlineThickness(4.0f);
        panel.setOutlineColor(sf::Color(245, 179, 103));

        editorPanel = &uiManager.CreateRoot<Panel>();

        editorPanel->SetPosition({900.0f, 80.0f});
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

        RefreshSimulationControls();
    }

    void FixedUpdate(float fixedDeltaTime) override {
        if (!simulationController.ConsumeTick()) {
            return;
        }

        rotation += 90.0f * fixedDeltaTime;
        panel.setRotation(sf::degrees(rotation));

        constexpr float MoveSpeed = 300.0f;

        sf::Vector2f movement{0.0f, 0.0f};

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

        circle.move(movement);
    }

    void Update(float deltaTime) override {
        if (Input::WasKeyPressed(Key::P)) {
            simulationController.TogglePlayPause();
        }

        if (Input::WasKeyPressed(Key::Period)) {
            simulationController.RequestSingleStep();
        }

        RefreshSimulationControls();

        if (simulationController.IsPlaying()) {
            panel.setFillColor(sf::Color(55, 55, 55));
        } else {
            panel.setFillColor(sf::Color(45, 70, 110));
        }

        diagnosticsOverlay.Update(deltaTime);
    }

    void Render(RenderContext &context) override {
        context.BeginWorld();

        auto &window = context.GetWindow();
        window.draw(panel);
        window.draw(circle);
        window.draw(worldCursor);

        context.BeginScreen();

        uiManager.Render(window);

        diagnosticsOverlay.Render(window, simulationController, context.GetCamera(), worldCursor.getPosition());
    }

    void HandleEvent(const sf::Event &event, RenderContext &context) override {
        const bool uiConsumedEvent = uiManager.HandleEvent(event);

        if (uiConsumedEvent) {
            // Always allow the camera to observe releases so an existing
            // drag cannot become stuck when released over the UI.
            if (event.is<sf::Event::MouseButtonReleased>()) {
                cameraController.HandleEvent(event, context);
            }

            return;
        }

        cameraController.HandleEvent(event, context);

        if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
            worldCursor.setPosition(context.ScreenToWorld(moved->position));
        }

        if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            worldCursor.setPosition(context.ScreenToWorld(wheel->position));
        }
    }

  private:
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

    SimulationController simulationController;
    CameraController2D cameraController;
    DiagnosticsOverlay diagnosticsOverlay;

    sf::CircleShape circle;
    sf::CircleShape worldCursor;
    sf::RectangleShape panel;

    UIManager uiManager;

    Panel *editorPanel = nullptr;
    Label *playPauseLabel = nullptr;
    Button *singleStepButton = nullptr;

    bool controlsInitialized = false;
    bool displayedPlayingState = false;

    float rotation = 0.0f;
};

int main() {
    Application app(1280, 720, "PipeFrame - Simulation Workbench");
    app.SetScene(std::make_unique<TestScene>());
    app.Run();

    return 0;
}

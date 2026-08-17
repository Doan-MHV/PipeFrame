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

class TestScene final : public Scene {
  public:
    void Load() override {
        const std::filesystem::path fontPath =
            std::filesystem::path(PIPEFRAME_ASSET_DIR) / "fonts/roboto_mono_semi.ttf";

        if (!diagnosticsOverlay.Load(fontPath)) {
            std::cerr << "Unable to load diagnostics font: " << fontPath << '\n';
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

        diagnosticsOverlay.Render(window, simulationController, context.GetCamera(), worldCursor.getPosition());
    }

    void HandleEvent(const sf::Event &event, RenderContext &context) override {
        cameraController.HandleEvent(event, context);

        if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
            worldCursor.setPosition(context.ScreenToWorld(moved->position));
        }

        if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            worldCursor.setPosition(context.ScreenToWorld(wheel->position));
        }
    }

  private:
    SimulationController simulationController;
    CameraController2D cameraController;
    DiagnosticsOverlay diagnosticsOverlay;

    sf::CircleShape circle;
    sf::CircleShape worldCursor;
    sf::RectangleShape panel;

    float rotation = 0.0f;
};

int main() {
    Application app(1280, 720, "PipeFrame - Simulation Workbench");
    app.SetScene(std::make_unique<TestScene>());
    app.Run();

    return 0;
}

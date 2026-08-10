#include <memory>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "EditorCameraController.h"

#include <PipeFrame/Core/Application.h>
#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Render/RenderContext.h>

class TestScene final : public Scene {
  public:
    void Load() override {
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

    void Update(float deltaTime) override {
        rotation += 90.0f * deltaTime;
        panel.setRotation(sf::degrees(rotation));

        const float moveSpeed = 300.0f;

        if (Input::IsKeyDown(Key::W)) {
            circle.move({0.0f, -moveSpeed * deltaTime});
        }

        if (Input::IsKeyDown(Key::S)) {
            circle.move({0.0f, moveSpeed * deltaTime});
        }

        if (Input::IsKeyDown(Key::A)) {
            circle.move({-moveSpeed * deltaTime, 0.0f});
        }

        if (Input::IsKeyDown(Key::D)) {
            circle.move({moveSpeed * deltaTime, 0.0f});
        }

        if (Input::WasKeyPressed(Key::Space)) {
            circle.setFillColor(sf::Color(252, 211, 94));
        }

        if (Input::WasKeyReleased(Key::Space)) {
            circle.setFillColor(sf::Color(232, 91, 116));
        }
    }

    void Render(RenderContext &context) override {
        context.BeginWorld();

        auto &window = context.GetWindow();
        window.draw(panel);
        window.draw(circle);
        window.draw(worldCursor);

        context.BeginScreen();
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
    EditorCameraController cameraController;

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

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
#include <PipeFrame/UI/Panel.h>
#include <PipeFrame/UI/UIManager.h>

class ClickablePanel final : public Panel {
  protected:
    void OnPointerEntered() override {
        pointerInside = true;

        if (!pressed) {
            RefreshColor();
        }
    }

    void OnPointerExited() override {
        pointerInside = false;

        if (!pressed) {
            RefreshColor();
        }
    }

    bool OnEvent(const sf::Event &event) override {
        if (const auto *mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button != sf::Mouse::Button::Left) {
                return false;
            }

            pressed = true;
            SetFillColor(PressedColor);

            return true;
        }

        if (const auto *mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            if (!pressed) {
                return false;
            }

            const sf::Vector2f mousePosition{static_cast<float>(mouseMoved->position.x),
                                             static_cast<float>(mouseMoved->position.y)};

            pointerInside = Contains(mousePosition);

            if (pointerInside) {
                SetFillColor(PressedColor);
            } else {
                RefreshColor();
            }

            return true;
        }

        if (const auto *mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (mouseReleased->button != sf::Mouse::Button::Left || !pressed) {
                return false;
            }

            const sf::Vector2f mousePosition{static_cast<float>(mouseReleased->position.x),
                                             static_cast<float>(mouseReleased->position.y)};

            pointerInside = Contains(mousePosition);
            pressed = false;

            // A click only happens when press and release are both
            // associated with this panel.
            if (pointerInside) {
                selected = !selected;
            }

            RefreshColor();

            return true;
        }

        return false;
    }

  private:
    void RefreshColor() {
        if (selected) {
            SetFillColor(pointerInside ? SelectedHoverColor : SelectedColor);

            return;
        }

        SetFillColor(pointerInside ? HoverColor : NormalColor);
    }

    inline static const sf::Color NormalColor{37, 41, 51};

    inline static const sf::Color HoverColor{52, 59, 74};

    inline static const sf::Color PressedColor{82, 96, 128};

    inline static const sf::Color SelectedColor{62, 75, 108};

    inline static const sf::Color SelectedHoverColor{74, 89, 126};

    bool pointerInside = false;
    bool pressed = false;
    bool selected = false;
};

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

        Panel &editorPanel = uiManager.CreateRoot<Panel>();

        editorPanel.SetPosition({900.0f, 80.0f});
        editorPanel.SetSize({300.0f, 560.0f});
        editorPanel.SetFillColor(sf::Color(24, 27, 34, 245));
        editorPanel.SetOutlineColor(sf::Color(78, 86, 104));
        editorPanel.SetOutlineThickness(1.0f);

        Panel &headerPanel = editorPanel.CreateChild<Panel>();

        headerPanel.SetPosition({12.0f, 12.0f});
        headerPanel.SetSize({276.0f, 48.0f});
        headerPanel.SetFillColor(sf::Color(42, 47, 59));
        headerPanel.SetOutlineColor(sf::Color(90, 100, 120));
        headerPanel.SetOutlineThickness(1.0f);

        Panel &contentPanel = editorPanel.CreateChild<Panel>();

        contentPanel.SetPosition({12.0f, 72.0f});
        contentPanel.SetSize({276.0f, 476.0f});
        contentPanel.SetFillColor(sf::Color(31, 34, 43));
        contentPanel.SetOutlineColor(sf::Color(65, 72, 88));
        contentPanel.SetOutlineThickness(1.0f);

        ClickablePanel &propertyGroup = contentPanel.CreateChild<ClickablePanel>();

        propertyGroup.SetPosition({12.0f, 12.0f});
        propertyGroup.SetSize({252.0f, 100.0f});
        propertyGroup.SetFillColor(sf::Color(37, 41, 51));
        propertyGroup.SetOutlineColor(sf::Color(76, 84, 102));
        propertyGroup.SetOutlineThickness(1.0f);
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
    SimulationController simulationController;
    CameraController2D cameraController;
    DiagnosticsOverlay diagnosticsOverlay;

    sf::CircleShape circle;
    sf::CircleShape worldCursor;
    sf::RectangleShape panel;

    UIManager uiManager;

    float rotation = 0.0f;
};

int main() {
    Application app(1280, 720, "PipeFrame - Simulation Workbench");
    app.SetScene(std::make_unique<TestScene>());
    app.Run();

    return 0;
}

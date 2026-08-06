#include <memory>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "Core/Application.h"
#include "Core/Scene.h"
#include "Render/RenderContext.h"

class TestScene final : public Scene
{
public:
    void Load() override
    {
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

    void Update(float deltaTime) override
    {
        rotation += 90.0f * deltaTime;
        circle.setRotation(sf::degrees(rotation));
    }

    void Render(RenderContext& context) override
    {
        context.BeginWorld();

        auto& window = context.GetWindow();
        window.draw(panel);
        window.draw(circle);

        context.BeginScreen();
    }

private:
    sf::CircleShape circle;
    sf::RectangleShape panel;
    float rotation = 0.0f;
};

int main()
{
    Application app(1280, 720, "PipeFrame - Simulation Workbench");
    app.SetScene(std::make_unique<TestScene>());
    app.Run();

    return 0;
}

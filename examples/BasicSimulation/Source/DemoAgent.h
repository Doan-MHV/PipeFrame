#ifndef BASIC_SIMULATION_DEMO_AGENT_H
#define BASIC_SIMULATION_DEMO_AGENT_H

#include <string>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <PipeFrame/Project/ProjectTypes.h>

namespace basic_simulation {

class DemoAgent final {
public:
    DemoAgent(
        pipeframe::SceneObjectId objectId,
        std::string name,
        sf::Vector2f position);

    pipeframe::SceneObjectId GetId() const;

    const std::string &GetName() const;

    void SetPosition(sf::Vector2f position);
    sf::Vector2f GetPosition() const;

    void SetRotation(float degrees);
    float GetRotation() const;

    void SetAngularSpeed(float degreesPerSecond);
    float GetAngularSpeed() const;

    void SetSelected(bool selected);
    void SetSimulationPlaying(bool playing);

    bool Contains(sf::Vector2f worldPoint) const;

    void FixedUpdate(float fixedDeltaTime);

    void Render(sf::RenderTarget &target) const;

private:
    static float NormalizeDegrees(float degrees);

    pipeframe::SceneObjectId objectId = 0;

    std::string name;

    sf::Vector2f position{
        0.0f,
        0.0f,
    };

    float rotation = 0.0f;
    float angularSpeed = 90.0f;

    sf::RectangleShape body;
    sf::CircleShape marker;
};

} // namespace basic_simulation

#endif
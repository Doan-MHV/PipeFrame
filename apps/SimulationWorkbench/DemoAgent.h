#ifndef PIPEFRAME_DEMO_AGENT_H
#define PIPEFRAME_DEMO_AGENT_H

#include <string>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include "Editor/SceneTypes.h"

using DemoAgentId = pipeframe::editor::SceneObjectId;

class DemoAgent {
  public:
    DemoAgent(DemoAgentId objectId, std::string name, sf::Vector2f initialPosition);

    DemoAgentId GetId() const;

    const std::string &GetName() const;

    void SetPosition(sf::Vector2f newPosition);
    sf::Vector2f GetPosition() const;
    void Move(sf::Vector2f offset);

    void SetRotation(float degrees);
    float GetRotation() const;

    void SetSelected(bool selected);
    void SetSimulationPlaying(bool playing);

    bool Contains(sf::Vector2f worldPoint) const;

    void Render(sf::RenderTarget &target) const;

  private:
    static float NormalizeDegrees(float degrees);

    DemoAgentId objectId;

    std::string name;

    sf::Vector2f position{0.0f, 0.0f};
    float rotation = 0.0f;

    sf::RectangleShape body;
    sf::CircleShape marker;
};

#endif
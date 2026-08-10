

#ifndef PIPEFRAME_EDITOR_CAMERA_CONTROLLER_H
#define PIPEFRAME_EDITOR_CAMERA_CONTROLLER_H

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include <optional>

class RenderContext;

class EditorCameraController {
  public:
    void HandleEvent(const sf::Event &event, RenderContext &context);

  private:
    std::optional<sf::Mouse::Button> dragButton;
    sf::Vector2i previousMousePosition{0, 0};

    static constexpr float MinimumZoom = 0.1f;
    static constexpr float MaximumZoom = 10.0f;
    static constexpr float ZoomFactor = 0.85f;
};

#endif

#ifndef PIPEFRAME_DIAGNOSTICS_OVERLAY_H
#define PIPEFRAME_DIAGNOSTICS_OVERLAY_H

#include <filesystem>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

class Camera2D;
class SimulationController;

class DiagnosticsOverlay {
  public:
    DiagnosticsOverlay();

    bool Load(const std::filesystem::path &fontPath);

    void Update(float frameDeltaTime);

    void Render(sf::RenderTarget &target, const SimulationController &simulation, const Camera2D &camera,
                sf::Vector2f mouseWorldPosition);

  private:
    sf::Font font;
    sf::Text text;
    sf::RectangleShape background;

    float sampleElapsedTime = 0.0f;
    unsigned int sampleFrameCount = 0;

    float framesPerSecond = 0.0f;
    float averageFrameTimeMs = 0.0f;
};

#endif
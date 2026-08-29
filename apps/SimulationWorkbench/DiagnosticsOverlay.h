#ifndef PIPEFRAME_DIAGNOSTICS_OVERLAY_H
#define PIPEFRAME_DIAGNOSTICS_OVERLAY_H

#include <cstddef>
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

    void SetPopulationRenderStats(std::size_t candidateAgentCount, std::size_t visibleAgentCount,
                                  std::size_t vertexCount, float geometryBuildTimeMs, bool usingQuads);

    void SetPopulationSimulationStats(float movementTimeMs, float spatialGridRebuildTimeMs);

    void SetVisible(bool newVisible);
    bool IsVisible() const;

    void SetPosition(sf::Vector2f position);
    sf::Vector2f GetSize() const;

  private:
    sf::Font font;
    sf::Text text;
    sf::RectangleShape background;

    bool visible = false;

    float sampleElapsedTime = 0.0f;
    unsigned int sampleFrameCount = 0;

    float framesPerSecond = 0.0f;
    float averageFrameTimeMs = 0.0f;

    std::size_t populationCandidateAgentCount = 0;
    std::size_t populationVisibleAgentCount = 0;
    std::size_t populationVertexCount = 0;

    float populationGeometryBuildTimeMs = 0.0f;
    float populationMovementTimeMs = 0.0f;
    float populationGridRebuildTimeMs = 0.0f;

    bool populationUsesQuads = false;
};

#endif
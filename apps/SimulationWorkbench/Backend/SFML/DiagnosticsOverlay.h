#ifndef PIPEFRAME_DIAGNOSTICS_OVERLAY_H
#define PIPEFRAME_DIAGNOSTICS_OVERLAY_H

#include <cstddef>
#include <filesystem>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/Backend/SFML/UI/MetricCard.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <SFML/System/Vector2.hpp>

class Camera2D;
class SimulationController;

class DiagnosticsOverlay : public pipeframe::ui::NativeViewPanel {
  public:
    explicit DiagnosticsOverlay(const sf::Font &font);
    void Refresh(const SimulationController &simulation, const Camera2D &camera, sf::Vector2f mouseWorldPosition);

    void SetPopulationRenderStats(std::size_t candidateAgentCount, std::size_t visibleAgentCount,
                                  std::size_t vertexCount, float geometryBuildTimeMs, bool usingQuads);

    void SetPopulationSimulationStats(float movementTimeMs, float spatialGridRebuildTimeMs);

  protected:
    void OnUpdate(float frameDeltaTime) override;
  private:
    pipeframe::ui::View BuildNativeView() override;
    std::string detailText;

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
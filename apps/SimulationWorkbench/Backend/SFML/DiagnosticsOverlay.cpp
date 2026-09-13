#include "DiagnosticsOverlay.h"

#include <iomanip>
#include <sstream>

#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Simulation/SimulationController.h>

DiagnosticsOverlay::DiagnosticsOverlay(const sf::Font &font) : NativeViewPanel(font) { SetSize({300,350}); }
pipeframe::ui::View DiagnosticsOverlay::BuildNativeView() {
    using namespace pipeframe::ui;
    return views::Scroll("diagnostics",views::Column("content",{
        views::Card("frame","PERFORMANCE",std::to_string(static_cast<int>(framesPerSecond))+" FPS",std::to_string(averageFrameTimeMs).substr(0,5)+" ms / frame"),
        views::Text("details",detailText).FitHeight()}).Padding(12)).FillHeight();
}

void DiagnosticsOverlay::OnUpdate(const float frameDeltaTime) {

    NativeViewPanel::OnUpdate(frameDeltaTime);
    sampleElapsedTime += frameDeltaTime;
    ++sampleFrameCount;

    constexpr float SamplePeriod = 0.25f;

    if (sampleElapsedTime >= SamplePeriod) {
        framesPerSecond = static_cast<float>(sampleFrameCount) / sampleElapsedTime;

        averageFrameTimeMs = sampleElapsedTime / static_cast<float>(sampleFrameCount) * 1000.0f;

        sampleElapsedTime = 0.0f;
        sampleFrameCount = 0;
    }
}

void DiagnosticsOverlay::Refresh(const SimulationController &simulation,
                                const Camera2D &camera, const sf::Vector2f mouseWorldPosition) {

    if (!IsVisible()) {
        return;
    }

    const auto cameraPosition = camera.GetCenter();

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2);

    stream << (simulation.IsPlaying() ? "PLAYING" : "PAUSED") << " | " << simulation.GetSpeedName() << '\n'
           << "Tick: " << simulation.GetTickCount() << '\n'
           << "Camera: " << cameraPosition.x << ", " << cameraPosition.y << '\n'
           << "Zoom: " << camera.GetZoom() << '\n'
           << "Mouse: " << mouseWorldPosition.x << ", " << mouseWorldPosition.y << '\n'
           << "Renderer: " << (populationUsesQuads ? "QUADS" : "POINTS") << '\n'
           << "Movement: " << populationMovementTimeMs << " ms\n"
           << "Grid rebuild: " << populationGridRebuildTimeMs << " ms\n"
           << "Candidates: " << populationCandidateAgentCount << '\n'
           << "Visible: " << populationVisibleAgentCount << '\n'
           << "Vertices: " << populationVertexCount << '\n'
           << "Geometry: " << populationGeometryBuildTimeMs << " ms";

    detailText=stream.str(); InvalidateView();
}

void DiagnosticsOverlay::SetPopulationRenderStats(const std::size_t candidateAgentCount,
                                                  const std::size_t visibleAgentCount, const std::size_t vertexCount,
                                                  const float geometryBuildTimeMs, const bool usingQuads) {

    populationCandidateAgentCount = candidateAgentCount;

    populationVisibleAgentCount = visibleAgentCount;

    populationVertexCount = vertexCount;

    populationGeometryBuildTimeMs = geometryBuildTimeMs;

    populationUsesQuads = usingQuads;
}

void DiagnosticsOverlay::SetPopulationSimulationStats(const float movementTimeMs,
                                                      const float spatialGridRebuildTimeMs) {

    populationMovementTimeMs = movementTimeMs;

    populationGridRebuildTimeMs = spatialGridRebuildTimeMs;
}

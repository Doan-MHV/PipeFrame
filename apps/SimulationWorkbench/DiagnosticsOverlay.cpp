#include "DiagnosticsOverlay.h"

#include <iomanip>
#include <sstream>

#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Simulation/SimulationController.h>

DiagnosticsOverlay::DiagnosticsOverlay() : text(font, "", 16) {

    background.setSize({300.0f, 300.0f});
    background.setPosition({12.0f, 12.0f});
    background.setFillColor(sf::Color(20, 22, 28, 220));
    background.setOutlineColor(sf::Color(75, 80, 95));
    background.setOutlineThickness(1.0f);

    text.setPosition({24.0f, 22.0f});
    text.setFillColor(sf::Color(225, 230, 240));
}

bool DiagnosticsOverlay::Load(const std::filesystem::path &fontPath) { return font.openFromFile(fontPath); }

void DiagnosticsOverlay::Update(const float frameDeltaTime) {

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

void DiagnosticsOverlay::Render(sf::RenderTarget &target, const SimulationController &simulation,
                                const Camera2D &camera, const sf::Vector2f mouseWorldPosition) {

    if (!visible) {
        return;
    }

    const sf::Vector2f cameraPosition = camera.GetCenter();

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2);

    stream << (simulation.IsPlaying() ? "PLAYING" : "PAUSED") << '\n'
           << "FPS: " << framesPerSecond << '\n'
           << "Frame: " << averageFrameTimeMs << " ms\n"
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

    text.setString(stream.str());

    target.draw(background);
    target.draw(text);
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

void DiagnosticsOverlay::SetVisible(const bool newVisible) { visible = newVisible; }

bool DiagnosticsOverlay::IsVisible() const { return visible; }

void DiagnosticsOverlay::SetPosition(const sf::Vector2f position) {

    background.setPosition(position);

    text.setPosition(position + sf::Vector2f{12.0f, 10.0f});
}

sf::Vector2f DiagnosticsOverlay::GetSize() const { return background.getSize(); }
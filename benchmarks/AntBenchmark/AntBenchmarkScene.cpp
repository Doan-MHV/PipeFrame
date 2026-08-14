#include "AntBenchmarkScene.h"

#include <iomanip>
#include <sstream>

#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Input/Key.h>
#include <PipeFrame/Render/RenderContext.h>

namespace {
const char *GetRenderModeName(AntRenderMode mode) {
    if (mode == AntRenderMode::Quads) {
        return "QUADS";
    }

    return "POINTS";
}
} // namespace

void AntBenchmarkScene::Load() {
    population.Initialize(InitialAntCount, InitialSeed);

    spatialGrid.Initialize({-AntPopulation::WorldHalfWidth, -AntPopulation::WorldHalfHeight},
                           {AntPopulation::WorldHalfWidth * 2.0f, AntPopulation::WorldHalfHeight * 2.0f},
                           SpatialCellSize, population.GetCount());

    spatialGrid.Rebuild(population);

    antRenderer.Initialize(population.GetCount(), AntRenderMode::Points);

    worldBounds.setSize({AntPopulation::WorldHalfWidth * 2.0f, AntPopulation::WorldHalfHeight * 2.0f});

    worldBounds.setPosition({-AntPopulation::WorldHalfWidth, -AntPopulation::WorldHalfHeight});

    worldBounds.setFillColor(sf::Color::Transparent);
    worldBounds.setOutlineColor(sf::Color(90, 100, 120));
    worldBounds.setOutlineThickness(2.0f);
}

void AntBenchmarkScene::HandleEvent(const sf::Event &event, RenderContext &context) {
    cameraController.HandleEvent(event, context);
}

void AntBenchmarkScene::FixedUpdate(float fixedDeltaTime) {
    if (!simulationController.ConsumeTick()) {
        return;
    }

    updateClock.restart();

    population.Update(fixedDeltaTime);

    lastUpdateTimeMs = updateClock.getElapsedTime().asSeconds() * 1000.0f;

    gridBuildClock.restart();

    spatialGrid.Rebuild(population);

    lastGridBuildTimeMs = gridBuildClock.getElapsedTime().asSeconds() * 1000.0f;

    // lastUpdateTimeMs = updateClock.getElapsedTime().asSeconds() * 1000.0f;
}

void AntBenchmarkScene::Update(float deltaTime) {
    if (Input::WasKeyPressed(Key::A)) {
        automaticLodEnabled = !automaticLodEnabled;
    }

    if (Input::WasKeyPressed(Key::L)) {
        automaticLodEnabled = false;

        const AntRenderMode nextMode =
            antRenderer.GetMode() == AntRenderMode::Quads ? AntRenderMode::Points : AntRenderMode::Quads;

        antRenderer.SetMode(nextMode, population.GetCount());
    }

    if (Input::WasKeyPressed(Key::R)) {
        renderingEnabled = !renderingEnabled;
    }

    if (Input::WasKeyPressed(Key::P)) {
        simulationController.TogglePlayPause();
    }

    if (Input::WasKeyPressed(Key::Period)) {
        simulationController.RequestSingleStep();
    }

    metricsElapsedTime += deltaTime;
    ++metricsFrameCount;

    constexpr float MetricsSamplePeriod = 0.25f;

    if (metricsElapsedTime >= MetricsSamplePeriod) {
        framesPerSecond = static_cast<float>(metricsFrameCount) / metricsElapsedTime;

        metricsElapsedTime = 0.0f;
        metricsFrameCount = 0;
        titleUpdateRequested = true;
    }
}

void AntBenchmarkScene::Render(RenderContext &context) {
    context.BeginWorld();

    sf::RenderWindow &window = context.GetWindow();
    const Camera2D &camera = context.GetCamera();

    UpdateAutomaticLod(camera.GetZoom());

    if (renderingEnabled) {
        geometryClock.restart();

        const sf::Vector2f cameraCenter = camera.GetCenter();
        const sf::Vector2f cameraSize = camera.GetSize();

        const sf::Vector2f viewportPosition{cameraCenter.x - cameraSize.x * 0.5f, cameraCenter.y - cameraSize.y * 0.5f};

        const sf::FloatRect viewport{viewportPosition, cameraSize};

        antRenderer.UpdateGeometry(population, spatialGrid, viewport);

        lastGeometryTimeMs = geometryClock.getElapsedTime().asSeconds() * 1000.0f;

        drawClock.restart();

        window.draw(worldBounds);
        antRenderer.Draw(window);

        lastDrawTimeMs = drawClock.getElapsedTime().asSeconds() * 1000.0f;
    } else {
        lastGeometryTimeMs = 0.0f;
        lastDrawTimeMs = 0.0f;
    }

    context.BeginScreen();

    if (titleUpdateRequested) {
        UpdateWindowTitle(window, camera);
        titleUpdateRequested = false;
    }
}

void AntBenchmarkScene::UpdateAutomaticLod(float cameraZoom) {
    if (!automaticLodEnabled) {
        return;
    }

    const AntRenderMode currentMode = antRenderer.GetMode();

    if (currentMode == AntRenderMode::Points && cameraZoom <= EnterQuadZoom) {
        antRenderer.SetMode(AntRenderMode::Quads, population.GetCount());

        return;
    }

    if (currentMode == AntRenderMode::Quads && cameraZoom >= EnterPointZoom) {
        antRenderer.SetMode(AntRenderMode::Points, population.GetCount());
    }
}

void AntBenchmarkScene::UpdateWindowTitle(sf::RenderWindow &window, const Camera2D &camera) {
    std::ostringstream title;

    title << std::fixed << std::setprecision(3);

    title << "PipeFrame Ant Benchmark"
          << " | " << (simulationController.IsPlaying() ? "PLAYING" : "PAUSED") << " | Ants: " << population.GetCount()
          << " | FPS: " << framesPerSecond << " | Tick update: " << lastUpdateTimeMs << " ms"
          << " | Grid build: " << lastGridBuildTimeMs << " ms"
          << " | Render: " << (renderingEnabled ? "ON" : "OFF")
          << " | LOD: " << (automaticLodEnabled ? "AUTO/" : "MANUAL/") << GetRenderModeName(antRenderer.GetMode())
          << " | Zoom: " << camera.GetZoom() << " | Candidates: " << antRenderer.GetCandidateAntCount()
          << " | Visible: " << antRenderer.GetVisibleAntCount() << " | Vertices: " << antRenderer.GetVertexCount()
          << " | Geometry: " << lastGeometryTimeMs << " ms"
          << " | Draw: " << lastDrawTimeMs << " ms"
          << " | Tick: " << simulationController.GetTickCount();

    window.setTitle(title.str());
}
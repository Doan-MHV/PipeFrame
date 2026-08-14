#ifndef PIPEFRAME_ANT_BENCHMARK_SCENE_H
#define PIPEFRAME_ANT_BENCHMARK_SCENE_H

#include <cstddef>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Render/CameraController2D.h>
#include <PipeFrame/Simulation/SimulationController.h>

#include "AntBatchRenderer.h"
#include "AntPopulation.h"
#include "AntSpatialGrid.h"

class AntBenchmarkScene final : public Scene {
  public:
    void Load() override;

    void FixedUpdate(float fixedDeltaTime) override;
    void Update(float deltaTime) override;
    void Render(RenderContext &context) override;
    void HandleEvent(const sf::Event &event, RenderContext &context) override;

  private:
    void UpdateAutomaticLod(float cameraZoom);

    void UpdateWindowTitle(sf::RenderWindow &window, const Camera2D &camera);

    static constexpr std::size_t InitialAntCount = 1'000'000;
    static constexpr std::uint32_t InitialSeed = 42;
    static constexpr float SpatialCellSize = 32.0f;
    static constexpr float EnterQuadZoom = 0.30f;
    static constexpr float EnterPointZoom = 0.45f;

    AntBatchRenderer antRenderer;
    AntPopulation population;
    SimulationController simulationController;
    CameraController2D cameraController;
    AntSpatialGrid spatialGrid;

    sf::RectangleShape worldBounds;

    sf::Clock updateClock;
    sf::Clock geometryClock;
    sf::Clock drawClock;

    bool automaticLodEnabled = true;

    float lastGeometryTimeMs = 0.0f;
    float lastDrawTimeMs = 0.0f;

    bool renderingEnabled = true;

    float lastUpdateTimeMs = 0.0f;

    float metricsElapsedTime = 0.0f;
    unsigned int metricsFrameCount = 0;
    float framesPerSecond = 0.0f;

    bool titleUpdateRequested = true;

    sf::Clock gridBuildClock;
    float lastGridBuildTimeMs = 0.0f;
};

#endif
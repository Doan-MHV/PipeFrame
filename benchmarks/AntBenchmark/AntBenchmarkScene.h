#ifndef PIPEFRAME_ANT_BENCHMARK_SCENE_H
#define PIPEFRAME_ANT_BENCHMARK_SCENE_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Render/CameraController2D.h>
#include <PipeFrame/Simulation/SimulationController.h>

#include "AntBatchRenderer.h"
#include "AntBenchmarkOverlay.h"
#include "AntPopulation.h"
#include "AntSeparationSystem.h"
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

    void UpdateOverlay(const Camera2D &camera);

    void SelectNearestAnt(sf::Vector2f worldPosition, float cameraZoom);

    void UpdateSelectedNeighborhood();

    void CycleBehaviorSliceCount();

    std::optional<std::uint32_t> selectedAntIndex;

    sf::CircleShape selectionMarker;

    std::size_t lastPickCandidateCount = 0;

    static constexpr float SelectionRadiusPixels = 10.0f;
    static constexpr std::size_t InitialAntCount = 1'000'000;
    static constexpr std::uint32_t InitialSeed = 42;
    static constexpr float RenderCellSize = 32.0f;
    static constexpr float InteractionCellSize = 2.0f;
    static constexpr float NeighborRadius = 2.0f;
    static constexpr float EnterQuadZoom = 0.30f;
    static constexpr float EnterPointZoom = 0.45f;

    AntBatchRenderer antRenderer;
    AntPopulation population;
    SimulationController simulationController;
    CameraController2D cameraController;
    AntSpatialGrid renderGrid;
    AntSpatialGrid interactionGrid;
    AntBenchmarkOverlay benchmarkOverlay;

    sf::RectangleShape worldBounds;

    sf::Clock updateClock;
    sf::Clock geometryClock;
    sf::Clock drawClock;

    sf::Clock renderGridBuildClock;
    sf::Clock interactionGridBuildClock;

    float lastRenderGridBuildTimeMs = 0.0f;
    float lastInteractionGridBuildTimeMs = 0.0f;

    bool automaticLodEnabled = true;

    float lastGeometryTimeMs = 0.0f;
    float lastDrawTimeMs = 0.0f;

    bool renderingEnabled = true;

    float lastUpdateTimeMs = 0.0f;

    float metricsElapsedTime = 0.0f;
    unsigned int metricsFrameCount = 0;
    float framesPerSecond = 0.0f;

    bool overlayUpdateRequested = true;

    std::size_t selectedNeighborCandidateCount = 0;
    std::size_t selectedNeighborCount = 0;

    sf::CircleShape neighborhoodMarker;

    AntSeparationSystem separationSystem;
    AntSeparationStats separationStats;

    sf::Clock behaviorClock;

    float lastBehaviorTimeMs = 0.0f;
    bool separationEnabled = false;

    std::size_t behaviorSliceCount = 64;

    float ticksPerSecond = 0.0f;
    std::uint64_t previousSampleTickCount = 0;
};

#endif
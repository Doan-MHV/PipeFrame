#ifndef PIPEFRAME_ANT_BENCHMARK_SCENE_H
#define PIPEFRAME_ANT_BENCHMARK_SCENE_H

#include <cstddef>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Simulation/SimulationController.h>

#include "AntBatchRenderer.h"
#include "AntPopulation.h"

class AntBenchmarkScene final : public Scene {
  public:
    void Load() override;

    void FixedUpdate(float fixedDeltaTime) override;
    void Update(float deltaTime) override;
    void Render(RenderContext &context) override;

  private:
    void UpdateWindowTitle(sf::RenderWindow &window);

    static constexpr std::size_t InitialAntCount = 1'000;
    static constexpr std::uint32_t InitialSeed = 42;

    AntBatchRenderer antRenderer;
    AntPopulation population;
    SimulationController simulationController;

    sf::RectangleShape worldBounds;

    sf::Clock updateClock;
    sf::Clock renderClock;

    float lastUpdateTimeMs = 0.0f;
    float lastRenderTimeMs = 0.0f;

    float metricsElapsedTime = 0.0f;
    unsigned int metricsFrameCount = 0;
    float framesPerSecond = 0.0f;
};

#endif
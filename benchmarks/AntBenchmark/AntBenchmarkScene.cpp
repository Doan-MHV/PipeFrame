#include "AntBenchmarkScene.h"

#include <iomanip>
#include <sstream>

#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Input/Key.h>
#include <PipeFrame/Render/RenderContext.h>

void AntBenchmarkScene::Load() {
    population.Initialize(InitialAntCount, InitialSeed);

    antRenderer.Initialize(population.GetCount());

    worldBounds.setSize({AntPopulation::WorldHalfWidth * 2.0f, AntPopulation::WorldHalfHeight * 2.0f});

    worldBounds.setPosition({-AntPopulation::WorldHalfWidth, -AntPopulation::WorldHalfHeight});

    worldBounds.setFillColor(sf::Color::Transparent);
    worldBounds.setOutlineColor(sf::Color(90, 100, 120));
    worldBounds.setOutlineThickness(2.0f);
}

void AntBenchmarkScene::FixedUpdate(float fixedDeltaTime) {
    if (!simulationController.ConsumeTick()) {
        return;
    }

    updateClock.restart();

    population.Update(fixedDeltaTime);

    lastUpdateTimeMs = updateClock.getElapsedTime().asSeconds() * 1000.0f;
}

void AntBenchmarkScene::Update(float deltaTime) {
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
    }
}

void AntBenchmarkScene::Render(RenderContext &context) {
    context.BeginWorld();

    sf::RenderWindow &window = context.GetWindow();

    renderClock.restart();

    window.draw(worldBounds);

    antRenderer.UpdateGeometry(population);
    antRenderer.Draw(window);

    lastRenderTimeMs = renderClock.getElapsedTime().asSeconds() * 1000.0f;

    context.BeginScreen();

    UpdateWindowTitle(window);
}

void AntBenchmarkScene::UpdateWindowTitle(sf::RenderWindow &window) {
    std::ostringstream title;

    title << std::fixed << std::setprecision(3);

    title << "PipeFrame Ant Benchmark"
          << " | " << (simulationController.IsPlaying() ? "PLAYING" : "PAUSED") << " | Ants: " << population.GetCount()
          << " | FPS: " << framesPerSecond << " | Update: " << lastUpdateTimeMs << " ms"
          << " | Batch build+draw: " << lastRenderTimeMs << " ms"
          << " | Tick: " << simulationController.GetTickCount();

    window.setTitle(title.str());
}
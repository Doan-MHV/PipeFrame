#include "AntBenchmarkScene.h"

#include <iostream>

#include <PipeFrame/Core/Time.h>
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

    const sf::Vector2f worldMinimum{-AntPopulation::WorldHalfWidth, -AntPopulation::WorldHalfHeight};

    const sf::Vector2f worldSize{AntPopulation::WorldHalfWidth * 2.0f, AntPopulation::WorldHalfHeight * 2.0f};

    renderGrid.Initialize(worldMinimum, worldSize, RenderCellSize, population.GetCount());

    interactionGrid.Initialize(worldMinimum, worldSize, InteractionCellSize, population.GetCount());

    renderGrid.Rebuild(population);
    interactionGrid.Rebuild(population);

    antRenderer.Initialize(population.GetCount(), AntRenderMode::Points);

    worldBounds.setSize({AntPopulation::WorldHalfWidth * 2.0f, AntPopulation::WorldHalfHeight * 2.0f});

    worldBounds.setPosition({-AntPopulation::WorldHalfWidth, -AntPopulation::WorldHalfHeight});

    worldBounds.setFillColor(sf::Color::Transparent);
    worldBounds.setOutlineColor(sf::Color(90, 100, 120));
    worldBounds.setOutlineThickness(2.0f);

    selectionMarker.setFillColor(sf::Color::Transparent);
    selectionMarker.setOutlineColor(sf::Color::Yellow);
    selectionMarker.setPointCount(32);

    neighborhoodMarker.setFillColor(sf::Color::Transparent);
    neighborhoodMarker.setOutlineColor(sf::Color::Cyan);
    neighborhoodMarker.setPointCount(48);

    const std::filesystem::path fontPath = std::filesystem::path(PIPEFRAME_ASSET_DIR) / "fonts/roboto_mono_semi.ttf";

    if (!benchmarkOverlay.Load(fontPath)) {
        std::cerr << "Unable to load benchmark font: " << fontPath << '\n';
    }
}

void AntBenchmarkScene::HandleEvent(const sf::Event &event, RenderContext &context) {
    cameraController.HandleEvent(event, context);

    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        const bool selectionClick = pressed->button == sf::Mouse::Button::Left && !Input::IsKeyDown(Key::Space);

        if (selectionClick) {
            const sf::Vector2f worldPosition = context.ScreenToWorld(pressed->position);

            SelectNearestAnt(worldPosition, context.GetCamera().GetZoom());
        }
    }
}

void AntBenchmarkScene::UpdateSelectedNeighborhood() {
    selectedNeighborCandidateCount = 0;
    selectedNeighborCount = 0;

    if (!selectedAntIndex) {
        return;
    }

    const std::vector<Ant> &ants = population.GetAnts();

    if (*selectedAntIndex >= ants.size()) {
        selectedAntIndex.reset();
        return;
    }

    const sf::Vector2f selectedPosition = ants[*selectedAntIndex].position;

    const sf::FloatRect queryArea{{selectedPosition.x - NeighborRadius, selectedPosition.y - NeighborRadius},
                                  {NeighborRadius * 2.0f, NeighborRadius * 2.0f}};

    const AntSpatialGrid::CellRange cellRange = interactionGrid.GetCellsOverlapping(queryArea);

    if (cellRange.IsEmpty()) {
        return;
    }

    const float neighborRadiusSquared = NeighborRadius * NeighborRadius;

    for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {
        for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {
            const std::span<const std::uint32_t> antIndices = interactionGrid.GetAgentIndices(column, row);

            selectedNeighborCandidateCount += antIndices.size();

            for (const std::uint32_t antIndex : antIndices) {
                if (antIndex == *selectedAntIndex) {
                    continue;
                }

                const sf::Vector2f difference = ants[antIndex].position - selectedPosition;

                const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

                if (distanceSquared <= neighborRadiusSquared) {
                    ++selectedNeighborCount;
                }
            }
        }
    }
}

void AntBenchmarkScene::FixedUpdate(float fixedDeltaTime) {
    if (!simulationController.ConsumeTick()) {
        return;
    }

    if (separationEnabled) {
        behaviorClock.restart();

        const std::uint64_t tickCount = simulationController.GetTickCount();

        const std::size_t sliceIndex = static_cast<std::size_t>((tickCount - 1) % behaviorSliceCount);

        const float behaviorDeltaTime = fixedDeltaTime * static_cast<float>(behaviorSliceCount);

        separationStats =
            separationSystem.Update(population, interactionGrid, behaviorDeltaTime, sliceIndex, behaviorSliceCount);

        lastBehaviorTimeMs = behaviorClock.getElapsedTime().asSeconds() * 1000.0f;
    } else {
        separationStats = {};
        lastBehaviorTimeMs = 0.0f;
    }

    updateClock.restart();

    population.Update(fixedDeltaTime);

    lastUpdateTimeMs = updateClock.getElapsedTime().asSeconds() * 1000.0f;

    renderGridBuildClock.restart();

    renderGrid.Rebuild(population);

    lastRenderGridBuildTimeMs = renderGridBuildClock.getElapsedTime().asSeconds() * 1000.0f;

    interactionGridBuildClock.restart();

    interactionGrid.Rebuild(population);

    lastInteractionGridBuildTimeMs = interactionGridBuildClock.getElapsedTime().asSeconds() * 1000.0f;

    UpdateSelectedNeighborhood();
}

void AntBenchmarkScene::Update(float deltaTime) {
    if (Input::WasKeyPressed(Key::A)) {
        automaticLodEnabled = !automaticLodEnabled;
    }

    if (Input::WasKeyPressed(Key::H)) {
        benchmarkOverlay.ToggleVisible();
    }

    if (Input::WasKeyPressed(Key::B)) {
        separationEnabled = !separationEnabled;

        if (!separationEnabled) {
            separationStats = {};
            lastBehaviorTimeMs = 0.0f;
        }

        overlayUpdateRequested = true;
    }

    if (Input::WasKeyPressed(Key::N)) {
        CycleBehaviorSliceCount();
        overlayUpdateRequested = true;
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

        const std::uint64_t currentTickCount = simulationController.GetTickCount();

        const std::uint64_t completedTicks = currentTickCount - previousSampleTickCount;

        ticksPerSecond = static_cast<float>(completedTicks) / metricsElapsedTime;

        previousSampleTickCount = currentTickCount;

        metricsElapsedTime = 0.0f;
        metricsFrameCount = 0;
        overlayUpdateRequested = true;
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

        antRenderer.UpdateGeometry(population, renderGrid, viewport);

        lastGeometryTimeMs = geometryClock.getElapsedTime().asSeconds() * 1000.0f;

        drawClock.restart();

        window.draw(worldBounds);
        antRenderer.Draw(window);

        if (selectedAntIndex) {
            const std::vector<Ant> &ants = population.GetAnts();

            if (*selectedAntIndex < ants.size()) {
                const float markerRadius = SelectionRadiusPixels * camera.GetZoom();

                selectionMarker.setRadius(markerRadius);

                selectionMarker.setOrigin({markerRadius, markerRadius});

                selectionMarker.setOutlineThickness(2.0f * camera.GetZoom());

                selectionMarker.setPosition(ants[*selectedAntIndex].position);

                window.draw(selectionMarker);

                neighborhoodMarker.setRadius(NeighborRadius);

                neighborhoodMarker.setOrigin({NeighborRadius, NeighborRadius});

                neighborhoodMarker.setOutlineThickness(camera.GetZoom());

                neighborhoodMarker.setPosition(ants[*selectedAntIndex].position);

                window.draw(neighborhoodMarker);
            }
        }

        lastDrawTimeMs = drawClock.getElapsedTime().asSeconds() * 1000.0f;
    } else {
        lastGeometryTimeMs = 0.0f;
        lastDrawTimeMs = 0.0f;
    }

    context.BeginScreen();

    if (overlayUpdateRequested) {
        UpdateOverlay(camera);
        overlayUpdateRequested = false;
    }

    benchmarkOverlay.Render(window);
}

void AntBenchmarkScene::SelectNearestAnt(sf::Vector2f worldPosition, float cameraZoom) {
    selectedAntIndex.reset();
    lastPickCandidateCount = 0;
    selectedNeighborCandidateCount = 0;
    selectedNeighborCount = 0;

    // Convert a 10-pixel selection radius into world units.
    const float selectionRadius = SelectionRadiusPixels * cameraZoom;

    const sf::FloatRect queryArea{{worldPosition.x - selectionRadius, worldPosition.y - selectionRadius},
                                  {selectionRadius * 2.0f, selectionRadius * 2.0f}};

    const AntSpatialGrid::CellRange cellRange = interactionGrid.GetCellsOverlapping(queryArea);

    if (cellRange.IsEmpty()) {
        return;
    }

    const std::vector<Ant> &ants = population.GetAnts();

    float nearestDistanceSquared = selectionRadius * selectionRadius;

    for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {
        for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {
            const std::span<const std::uint32_t> antIndices = interactionGrid.GetAgentIndices(column, row);

            lastPickCandidateCount += antIndices.size();

            for (const std::uint32_t antIndex : antIndices) {
                const sf::Vector2f difference = ants[antIndex].position - worldPosition;

                const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

                if (distanceSquared <= nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    selectedAntIndex = antIndex;
                }
            }
        }
    }

    UpdateSelectedNeighborhood();
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

void AntBenchmarkScene::UpdateOverlay(const Camera2D &camera) {
    AntBenchmarkMetrics metrics;

    metrics.playing = simulationController.IsPlaying();

    metrics.renderingEnabled = renderingEnabled;
    metrics.antCount = population.GetCount();
    metrics.tickCount = simulationController.GetTickCount();

    metrics.framesPerSecond = framesPerSecond;
    metrics.tickUpdateTimeMs = lastUpdateTimeMs;
    metrics.renderGridTimeMs = lastRenderGridBuildTimeMs;

    metrics.interactionGridTimeMs = lastInteractionGridBuildTimeMs;

    metrics.geometryTimeMs = lastGeometryTimeMs;
    metrics.drawTimeMs = lastDrawTimeMs;
    metrics.zoom = camera.GetZoom();

    metrics.lodMode = automaticLodEnabled ? "AUTO/" : "MANUAL/";

    metrics.lodMode += GetRenderModeName(antRenderer.GetMode());

    metrics.renderCandidateCount = antRenderer.GetCandidateAntCount();

    metrics.visibleAntCount = antRenderer.GetVisibleAntCount();

    metrics.vertexCount = antRenderer.GetVertexCount();

    metrics.selectedAntIndex = selectedAntIndex;
    metrics.pickCandidateCount = lastPickCandidateCount;

    metrics.neighborCount = selectedNeighborCount;

    metrics.neighborCandidateCount = selectedNeighborCandidateCount;

    metrics.separationEnabled = separationEnabled;

    metrics.behaviorTimeMs = lastBehaviorTimeMs;

    metrics.behaviorProcessedAntCount = separationStats.processedAntCount;

    metrics.behaviorCandidateCheckCount = separationStats.candidateCheckCount;

    metrics.behaviorNeighborInteractionCount = separationStats.neighborInteractionCount;

    metrics.behaviorSliceCount = behaviorSliceCount;

    metrics.ticksPerSecond = ticksPerSecond;

    metrics.behaviorAgentUpdateRateHz = ticksPerSecond / static_cast<float>(behaviorSliceCount);

    benchmarkOverlay.Update(metrics);
}

void AntBenchmarkScene::CycleBehaviorSliceCount() {
    switch (behaviorSliceCount) {
    case 64:
        behaviorSliceCount = 32;
        break;
    case 32:
        behaviorSliceCount = 16;
        break;

    case 16:
        behaviorSliceCount = 8;
        break;

    case 8:
        behaviorSliceCount = 1;
        break;

    default:
        behaviorSliceCount = 64;
        break;
    }
}
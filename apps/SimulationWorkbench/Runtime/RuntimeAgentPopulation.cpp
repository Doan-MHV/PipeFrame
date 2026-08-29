#include "RuntimeAgentPopulation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <new>
#include <random>

namespace pipeframe::runtime {

namespace {

constexpr float PopulationGridCellSize = 64.0f;
constexpr std::uint32_t MaximumRuntimeAgentCount = 1'000'000;

bool IsValidSpawnArea(const sf::Vector2f size) {
    return std::isfinite(size.x) && std::isfinite(size.y) && size.x > 0.0f && size.y > 0.0f;
}

} // namespace

bool RuntimeAgentPopulation::Initialize(const editor::SceneObjectData &sourceObject) {

    Clear();

    if (sourceObject.type != editor::SceneObjectType::AgentPopulation || !sourceObject.population.has_value()) {
        return false;
    }

    const editor::AgentPopulationSettings &settings = *sourceObject.population;

    if (settings.agentCount == 0 || settings.agentCount > MaximumRuntimeAgentCount ||
        !IsValidSpawnArea(settings.spawnAreaSize)) {
        return false;
    }

    sourceObjectId = sourceObject.id;
    transform = sourceObject.transform;

    halfWidth = settings.spawnAreaSize.x * 0.5f;
    halfHeight = settings.spawnAreaSize.y * 0.5f;

    const std::size_t count = static_cast<std::size_t>(settings.agentCount);

    try {
        positionX.resize(count);
        positionY.resize(count);
        velocityX.resize(count);
        velocityY.resize(count);

        std::mt19937 randomGenerator{settings.randomSeed};

        std::uniform_real_distribution<float> spawnX{
            -halfWidth,
            halfWidth,
        };

        std::uniform_real_distribution<float> spawnY{
            -halfHeight,
            halfHeight,
        };

        std::uniform_real_distribution<float> velocity{
            -120.0f,
            120.0f,
        };

        for (std::size_t index = 0; index < count; ++index) {
            positionX[index] = spawnX(randomGenerator);
            positionY[index] = spawnY(randomGenerator);

            velocityX[index] = velocity(randomGenerator);
            velocityY[index] = velocity(randomGenerator);
        }

        spatialGrid.Initialize({-halfWidth, -halfHeight}, settings.spawnAreaSize, PopulationGridCellSize, count);

        spatialGrid.Rebuild(positionX, positionY);
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }

    spatialGridDirty = false;
    return true;
}

void RuntimeAgentPopulation::Clear() {
    movementSliceIndex = 0;

    sourceObjectId = 0;
    transform = {};

    halfWidth = 0.0f;
    halfHeight = 0.0f;

    positionX.clear();
    positionY.clear();
    velocityX.clear();
    velocityY.clear();

    spatialGrid.Clear();

    lastUpdateStats = {};
    spatialGridDirty = false;
}

void RuntimeAgentPopulation::Update(const float fixedDeltaTime) {
    using Clock = std::chrono::steady_clock;

    lastUpdateStats.movementTimeMs = 0.0f;
    lastUpdateStats.spatialGridRebuildTimeMs = 0.0f;

    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f) {
        return;
    }

    const std::size_t count = std::min({
        positionX.size(),
        positionY.size(),
        velocityX.size(),
        velocityY.size(),
    });

    if (count == 0) {
        return;
    }

    const auto movementStart = Clock::now();

    const std::size_t sliceCount = std::min(MovementSliceCount, count);

    const std::size_t beginIndex = count * movementSliceIndex / sliceCount;

    const std::size_t endIndex = count * (movementSliceIndex + 1) / sliceCount;

    // Every agent runs once per slice cycle. Compensate for skipped
    // ticks so the average movement speed remains unchanged.
    const float slicedDeltaTime = fixedDeltaTime * static_cast<float>(sliceCount);

    for (std::size_t index = beginIndex; index < endIndex; ++index) {

        positionX[index] += velocityX[index] * slicedDeltaTime;
        positionY[index] += velocityY[index] * slicedDeltaTime;

        if (positionX[index] < -halfWidth) {
            positionX[index] = -halfWidth;
            velocityX[index] = -velocityX[index];
        } else if (positionX[index] > halfWidth) {
            positionX[index] = halfWidth;
            velocityX[index] = -velocityX[index];
        }

        if (positionY[index] < -halfHeight) {
            positionY[index] = -halfHeight;
            velocityY[index] = -velocityY[index];
        } else if (positionY[index] > halfHeight) {
            positionY[index] = halfHeight;
            velocityY[index] = -velocityY[index];
        }
    }

    movementSliceIndex = (movementSliceIndex + 1) % sliceCount;

    // Rebuild once all movement slices have completed.
    if (movementSliceIndex == 0) {
        spatialGridDirty = true;
    }

    const auto movementEnd = Clock::now();

    lastUpdateStats.movementTimeMs = std::chrono::duration<float, std::milli>(movementEnd - movementStart).count();
}

void RuntimeAgentPopulation::RebuildSpatialGrid() {
    using Clock = std::chrono::steady_clock;

    lastUpdateStats.spatialGridRebuildTimeMs = 0.0f;

    if (!spatialGridDirty) {
        return;
    }

    const auto rebuildStart = Clock::now();

    spatialGrid.Rebuild(positionX, positionY);

    const auto rebuildEnd = Clock::now();

    lastUpdateStats.spatialGridRebuildTimeMs =
        std::chrono::duration<float, std::milli>(rebuildEnd - rebuildStart).count();

    spatialGridDirty = false;
}

std::size_t RuntimeAgentPopulation::GetCount() const { return std::min(positionX.size(), positionY.size()); }

editor::SceneObjectId RuntimeAgentPopulation::GetSourceObjectId() const { return sourceObjectId; }

const editor::SceneTransform &RuntimeAgentPopulation::GetTransform() const { return transform; }

sf::Vector2f RuntimeAgentPopulation::GetSpawnAreaSize() const {
    return {
        halfWidth * 2.0f,
        halfHeight * 2.0f,
    };
}

const RuntimePopulationSpatialGrid &RuntimeAgentPopulation::GetSpatialGrid() const { return spatialGrid; }

const RuntimeAgentPopulationUpdateStats &RuntimeAgentPopulation::GetLastUpdateStats() const { return lastUpdateStats; }

const std::vector<float> &RuntimeAgentPopulation::GetPositionX() const { return positionX; }

const std::vector<float> &RuntimeAgentPopulation::GetPositionY() const { return positionY; }

} // namespace pipeframe::runtime
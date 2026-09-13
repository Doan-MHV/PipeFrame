#include "Population.h"
#include <PipeFrame/Backend/SFML/Conversions.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <random>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Transformable.hpp>

#include "BasicSimulationTypes.h"

namespace basic_simulation {

namespace {

constexpr float GridCellSize = 64.0f;

constexpr std::uint32_t
    MaximumAgentCount = 1'000'000;

bool IsFinite(
    const sf::Vector2f value) {

    return std::isfinite(value.x) &&
           std::isfinite(value.y);
}

std::optional<std::int64_t> ReadInteger(
    const pipeframe::SceneObjectData &object,
    const std::string &key) {

    const auto iterator =
        object.properties.find(key);

    if (iterator ==
        object.properties.end()) {

        return std::nullopt;
    }

    if (const auto *integer =
            std::get_if<std::int64_t>(
                &iterator->second)) {

        return *integer;
    }

    if (const auto *number =
            std::get_if<double>(
                &iterator->second)) {

        return static_cast<std::int64_t>(
            *number);
    }

    return std::nullopt;
}

std::optional<sf::Vector2f> ReadVector(
    const pipeframe::SceneObjectData &object,
    const std::string &key) {

    const auto iterator =
        object.properties.find(key);

    if (iterator ==
        object.properties.end()) {

        return std::nullopt;
    }

    if (const auto *vector =
            std::get_if<pipeframe::Vector2f>(
                &iterator->second)) {

        return pipeframe::backend::sfml::ToBackend(*vector);
    }

    return std::nullopt;
}

} // namespace

bool Population::Initialize(
    const pipeframe::SceneObjectData
        &sourceObject) {

    Clear();

    const std::optional<
        PopulationConfiguration>
        parsedConfiguration =
            ReadConfiguration(sourceObject);

    if (!parsedConfiguration.has_value()) {
        return false;
    }

    sourceObjectId = sourceObject.id;
    transform = sourceObject.transform;
    configuration = *parsedConfiguration;

    halfWidth =
        configuration.spawnArea.x * 0.5f;

    halfHeight =
        configuration.spawnArea.y * 0.5f;

    const std::size_t count =
        static_cast<std::size_t>(
            configuration.agentCount);

    try {
        positionX.resize(count);
        positionY.resize(count);

        velocityX.resize(count);
        velocityY.resize(count);

        std::mt19937 randomGenerator{
            configuration.randomSeed,
        };

        std::uniform_real_distribution<float>
            spawnX{
                -halfWidth,
                halfWidth,
            };

        std::uniform_real_distribution<float>
            spawnY{
                -halfHeight,
                halfHeight,
            };

        std::uniform_real_distribution<float>
            velocity{
                -120.0f,
                120.0f,
            };

        for (std::size_t index = 0;
             index < count;
             ++index) {

            positionX[index] =
                spawnX(randomGenerator);

            positionY[index] =
                spawnY(randomGenerator);

            velocityX[index] =
                velocity(randomGenerator);

            velocityY[index] =
                velocity(randomGenerator);
        }

        spatialGrid.Initialize(
            {
                -halfWidth,
                -halfHeight,
            },
            configuration.spawnArea,
            GridCellSize,
            count);

        spatialGrid.Rebuild(
            positionX,
            positionY);
    } catch (const std::bad_alloc &) {
        Clear();
        return false;
    }

    movementSliceIndex = 0;
    spatialGridDirty = false;

    return true;
}

bool Population::MatchesSource(
    const pipeframe::SceneObjectData
        &sourceObject) const {

    if (sourceObject.id != sourceObjectId ||
        sourceObject.typeId !=
            PopulationTypeId ||
        sourceObject.transform != transform) {

        return false;
    }

    const std::optional<
        PopulationConfiguration>
        parsedConfiguration =
            ReadConfiguration(sourceObject);

    return parsedConfiguration.has_value() &&
           *parsedConfiguration ==
               configuration;
}

void Population::Clear() {
    sourceObjectId = 0;
    transform = {};

    configuration = {};

    halfWidth = 0.0f;
    halfHeight = 0.0f;

    positionX.clear();
    positionY.clear();

    velocityX.clear();
    velocityY.clear();

    spatialGrid.Clear();

    lastUpdateStats = {};

    movementSliceIndex = 0;
    spatialGridDirty = false;
}

void Population::FixedUpdate(
    const float fixedDeltaTime) {

    using Clock = std::chrono::steady_clock;

    lastUpdateStats
        .movementTimeMilliseconds = 0.0f;

    lastUpdateStats
        .gridRebuildTimeMilliseconds = 0.0f;

    if (!std::isfinite(fixedDeltaTime) ||
        fixedDeltaTime <= 0.0f) {

        return;
    }

    const std::size_t count =
        std::min({
            positionX.size(),
            positionY.size(),
            velocityX.size(),
            velocityY.size(),
        });

    if (count == 0) {
        return;
    }

    const auto movementStart = Clock::now();

    const std::size_t sliceCount =
        std::min(
            MovementSliceCount,
            count);

    const std::size_t beginIndex =
        count * movementSliceIndex /
        sliceCount;

    const std::size_t endIndex =
        count *
        (movementSliceIndex + 1) /
        sliceCount;

    const float slicedDeltaTime =
        fixedDeltaTime *
        static_cast<float>(sliceCount);

    for (std::size_t index = beginIndex;
         index < endIndex;
         ++index) {

        positionX[index] +=
            velocityX[index] *
            slicedDeltaTime;

        positionY[index] +=
            velocityY[index] *
            slicedDeltaTime;

        if (positionX[index] < -halfWidth) {
            positionX[index] = -halfWidth;
            velocityX[index] =
                -velocityX[index];
        } else if (
            positionX[index] > halfWidth) {

            positionX[index] = halfWidth;
            velocityX[index] =
                -velocityX[index];
        }

        if (positionY[index] < -halfHeight) {
            positionY[index] = -halfHeight;
            velocityY[index] =
                -velocityY[index];
        } else if (
            positionY[index] > halfHeight) {

            positionY[index] = halfHeight;
            velocityY[index] =
                -velocityY[index];
        }
    }

    movementSliceIndex =
        (movementSliceIndex + 1) %
        sliceCount;

    if (movementSliceIndex == 0) {
        spatialGridDirty = true;
    }

    const auto movementEnd = Clock::now();

    lastUpdateStats
        .movementTimeMilliseconds =
            std::chrono::duration<
                float,
                std::milli>(
                movementEnd -
                movementStart)
                .count();
}

void Population::RebuildSpatialGrid() {
    using Clock = std::chrono::steady_clock;

    lastUpdateStats
        .gridRebuildTimeMilliseconds = 0.0f;

    if (!spatialGridDirty) {
        return;
    }

    const auto rebuildStart = Clock::now();

    spatialGrid.Rebuild(
        positionX,
        positionY);

    const auto rebuildEnd = Clock::now();

    lastUpdateStats
        .gridRebuildTimeMilliseconds =
            std::chrono::duration<
                float,
                std::milli>(
                rebuildEnd -
                rebuildStart)
                .count();

    spatialGridDirty = false;
}

bool Population::Contains(
    const sf::Vector2f worldPoint) const {

    sf::Transformable transformable;

    transformable.setPosition(
        pipeframe::backend::sfml::ToBackend(transform.position));

    transformable.setRotation(
        sf::degrees(
            transform.rotation));

    const sf::Vector2f localPoint =
        transformable
            .getInverseTransform()
            .transformPoint(worldPoint);

    return sf::FloatRect{
        {
            -halfWidth,
            -halfHeight,
        },
        configuration.spawnArea,
    }.contains(localPoint);
}

void Population::RenderBounds(
    sf::RenderTarget &target,
    const bool selected) const {

    sf::RectangleShape bounds(
        configuration.spawnArea);

    bounds.setOrigin({
        halfWidth,
        halfHeight,
    });

    bounds.setPosition(
        pipeframe::backend::sfml::ToBackend(transform.position));

    bounds.setRotation(
        sf::degrees(
            transform.rotation));

    bounds.setFillColor(
        sf::Color::Transparent);

    bounds.setOutlineColor(
        selected
            ? sf::Color(245, 179, 103)
            : sf::Color(95, 100, 110));

    bounds.setOutlineThickness(
        selected ? 4.0f : 1.0f);

    target.draw(bounds);
}

std::size_t Population::GetCount() const {
    return std::min(
        positionX.size(),
        positionY.size());
}

pipeframe::SceneObjectId
Population::GetSourceObjectId() const {
    return sourceObjectId;
}

const pipeframe::SceneTransform &
Population::GetTransform() const {
    return transform;
}

sf::Vector2f
Population::GetSpawnArea() const {
    return configuration.spawnArea;
}

const PopulationSpatialGrid &
Population::GetSpatialGrid() const {
    return spatialGrid;
}

const PopulationUpdateStats &
Population::GetLastUpdateStats() const {
    return lastUpdateStats;
}

const std::vector<float> &
Population::GetPositionX() const {
    return positionX;
}

const std::vector<float> &
Population::GetPositionY() const {
    return positionY;
}

std::optional<PopulationConfiguration>
Population::ReadConfiguration(
    const pipeframe::SceneObjectData
        &sourceObject) {

    if (sourceObject.typeId !=
        PopulationTypeId) {

        return std::nullopt;
    }

    const std::optional<std::int64_t>
        agentCount =
            ReadInteger(
                sourceObject,
                AgentCountKey);

    const std::optional<sf::Vector2f>
        spawnArea =
            ReadVector(
                sourceObject,
                SpawnAreaKey);

    const std::optional<std::int64_t>
        randomSeed =
            ReadInteger(
                sourceObject,
                RandomSeedKey);

    if (!agentCount.has_value() ||
        *agentCount <= 0 ||
        *agentCount >
            static_cast<std::int64_t>(
                MaximumAgentCount) ||
        !spawnArea.has_value() ||
        !IsFinite(*spawnArea) ||
        spawnArea->x <= 0.0f ||
        spawnArea->y <= 0.0f ||
        !randomSeed.has_value() ||
        *randomSeed < 0 ||
        *randomSeed >
            static_cast<std::int64_t>(
                std::numeric_limits<
                    std::uint32_t>::max())) {

        return std::nullopt;
    }

    return PopulationConfiguration{
        .agentCount =
            static_cast<std::uint32_t>(
                *agentCount),

        .spawnArea = *spawnArea,

        .randomSeed =
            static_cast<std::uint32_t>(
                *randomSeed),
    };
}

} // namespace basic_simulation

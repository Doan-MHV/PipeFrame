#ifndef BASIC_SIMULATION_POPULATION_H
#define BASIC_SIMULATION_POPULATION_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <PipeFrame/Project/ProjectTypes.h>

#include "PopulationSpatialGrid.h"

namespace basic_simulation {

struct PopulationConfiguration final {
    std::uint32_t agentCount = 100'000;

    sf::Vector2f spawnArea{
        4000.0f,
        4000.0f,
    };

    std::uint32_t randomSeed = 1;

    bool operator==(
        const PopulationConfiguration &) const =
        default;
};

struct PopulationUpdateStats final {
    float movementTimeMilliseconds = 0.0f;
    float gridRebuildTimeMilliseconds = 0.0f;
};

class Population final {
  public:
    Population() = default;

    Population(const Population &) = delete;
    Population &operator=(const Population &) = delete;

    Population(Population &&) noexcept = default;
    Population &operator=(Population &&) noexcept = default;

    bool Initialize(
        const pipeframe::SceneObjectData &sourceObject);

    bool MatchesSource(
        const pipeframe::SceneObjectData
            &sourceObject) const;

    void Clear();

    void FixedUpdate(float fixedDeltaTime);
    void RebuildSpatialGrid();

    bool Contains(sf::Vector2f worldPoint) const;

    void RenderBounds(
        sf::RenderTarget &target,
        bool selected) const;

    std::size_t GetCount() const;

    pipeframe::SceneObjectId
    GetSourceObjectId() const;

    const pipeframe::SceneTransform &
    GetTransform() const;

    sf::Vector2f GetSpawnArea() const;

    const PopulationSpatialGrid &
    GetSpatialGrid() const;

    const PopulationUpdateStats &
    GetLastUpdateStats() const;

    const std::vector<float> &
    GetPositionX() const;

    const std::vector<float> &
    GetPositionY() const;

  private:
    static std::optional<
        PopulationConfiguration>
    ReadConfiguration(
        const pipeframe::SceneObjectData
            &sourceObject);

    pipeframe::SceneObjectId sourceObjectId = 0;

    pipeframe::SceneTransform transform;

    PopulationConfiguration configuration;

    float halfWidth = 0.0f;
    float halfHeight = 0.0f;

    std::vector<float> positionX;
    std::vector<float> positionY;

    std::vector<float> velocityX;
    std::vector<float> velocityY;

    PopulationSpatialGrid spatialGrid;
    PopulationUpdateStats lastUpdateStats;

    static constexpr std::size_t
        MovementSliceCount = 8;

    std::size_t movementSliceIndex = 0;
    bool spatialGridDirty = false;
};

} // namespace basic_simulation

#endif
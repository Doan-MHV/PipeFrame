#include "RuntimePopulationPointRenderer.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Angle.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

namespace pipeframe::runtime {

namespace {

const sf::Color AgentColor{232, 91, 116};

bool IsPopulationVisible(const RuntimeAgentPopulation &population, const sf::FloatRect &worldViewport) {
    const editor::SceneTransform &transform = population.GetTransform();

    const sf::Vector2f halfSize = population.GetSpawnAreaSize() * 0.5f;

    const float radians = sf::degrees(transform.rotation).asRadians();

    const float absoluteCosine = std::abs(std::cos(radians));

    const float absoluteSine = std::abs(std::sin(radians));

    const sf::Vector2f worldHalfSize{
        absoluteCosine * halfSize.x + absoluteSine * halfSize.y,

        absoluteSine * halfSize.x + absoluteCosine * halfSize.y,
    };

    const sf::FloatRect populationBounds{
        transform.position - worldHalfSize,
        worldHalfSize * 2.0f,
    };

    return populationBounds.findIntersection(worldViewport).has_value();
}

// Convert the world-space camera rectangle into an axis-aligned
// rectangle in the population's local coordinate system.
sf::FloatRect CalculateLocalViewport(const sf::FloatRect &worldViewport,
                                     const sf::Transform &inversePopulationTransform) {
    const sf::Vector2f worldMinimum = worldViewport.position;

    const sf::Vector2f worldMaximum{
        worldViewport.position.x + worldViewport.size.x,
        worldViewport.position.y + worldViewport.size.y,
    };

    const std::array<sf::Vector2f, 4> worldCorners{
        worldMinimum,

        sf::Vector2f{
            worldMaximum.x,
            worldMinimum.y,
        },

        worldMaximum,

        sf::Vector2f{
            worldMinimum.x,
            worldMaximum.y,
        },
    };

    sf::Vector2f localMinimum = inversePopulationTransform.transformPoint(worldCorners[0]);

    sf::Vector2f localMaximum = localMinimum;

    for (std::size_t cornerIndex = 1; cornerIndex < worldCorners.size(); ++cornerIndex) {

        const sf::Vector2f localCorner = inversePopulationTransform.transformPoint(worldCorners[cornerIndex]);

        localMinimum.x = std::min(localMinimum.x, localCorner.x);

        localMinimum.y = std::min(localMinimum.y, localCorner.y);

        localMaximum.x = std::max(localMaximum.x, localCorner.x);

        localMaximum.y = std::max(localMaximum.y, localCorner.y);
    }

    return {
        localMinimum,
        localMaximum - localMinimum,
    };
}

} // namespace

void RuntimePopulationPointRenderer::BeginFrame() { frameStats = {}; }

const RuntimePopulationPointRenderStats &RuntimePopulationPointRenderer::GetFrameStats() const { return frameStats; }

void RuntimePopulationPointRenderer::Render(sf::RenderTarget &target, const RuntimeAgentPopulation &population,
                                            const sf::FloatRect &worldViewport) {
    const std::size_t agentCount = population.GetCount();

    if (agentCount == 0) {
        return;
    }

    // Cheap whole-population rejection.
    if (!IsPopulationVisible(population, worldViewport)) {
        return;
    }

    const editor::SceneTransform &sourceTransform = population.GetTransform();

    sf::Transformable transformable;

    transformable.setPosition(sourceTransform.position);
    transformable.setRotation(sf::degrees(sourceTransform.rotation));

    const sf::Transform &populationTransform = transformable.getTransform();

    const sf::Transform &inversePopulationTransform = transformable.getInverseTransform();

    const sf::FloatRect localViewport = CalculateLocalViewport(worldViewport, inversePopulationTransform);

    const RuntimePopulationSpatialGrid &spatialGrid = population.GetSpatialGrid();

    const RuntimePopulationSpatialGrid::CellRange cellRange = spatialGrid.GetCellsOverlapping(localViewport);

    if (cellRange.IsEmpty()) {
        return;
    }

    const auto geometryStartTime = std::chrono::steady_clock::now();

    // Keep enough reusable storage for the largest possible result.
    if (vertices.size() != agentCount) {
        vertices.resize(agentCount);

        for (sf::Vertex &vertex : vertices) {
            vertex.color = AgentColor;
        }
    }

    const std::vector<float> &positionX = population.GetPositionX();

    const std::vector<float> &positionY = population.GetPositionY();

    std::size_t visibleAgentCount = 0;

    for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {

        for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {

            const std::span<const std::uint32_t> agentIndices = spatialGrid.GetAgentIndices(column, row);
            frameStats.candidateAgentCount += agentIndices.size();

            for (const std::uint32_t agentIndex : agentIndices) {
                const sf::Vector2f localPosition{
                    positionX[agentIndex],
                    positionY[agentIndex],
                };

                // The grid-cell query uses an axis-aligned local
                // rectangle. This final check removes extra agents
                // introduced when the population is rotated.
                const sf::Vector2f worldPosition = populationTransform.transformPoint(localPosition);

                if (!worldViewport.contains(worldPosition)) {
                    continue;
                }

                vertices[visibleAgentCount].position = localPosition;

                ++visibleAgentCount;
            }
        }
    }

    frameStats.visibleAgentCount += visibleAgentCount;

    const auto geometryEndTime = std::chrono::steady_clock::now();

    frameStats.geometryBuildTimeMs +=
        std::chrono::duration<float, std::milli>(geometryEndTime - geometryStartTime).count();

    if (visibleAgentCount == 0) {
        return;
    }

    const sf::RenderStates renderStates{
        populationTransform,
    };

    target.draw(vertices.data(), visibleAgentCount, sf::PrimitiveType::Points, renderStates);
}

} // namespace pipeframe::runtime
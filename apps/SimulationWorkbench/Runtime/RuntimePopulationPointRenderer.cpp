#include "RuntimePopulationPointRenderer.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Angle.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pipeframe::runtime {

namespace {

const sf::Color AgentColor{232, 91, 116};

constexpr std::size_t PointVerticesPerAgent = 1;
constexpr std::size_t QuadVerticesPerAgent = 6;
constexpr float AgentHalfSize = 0.35f;

std::size_t GetVerticesPerAgent(const RuntimePopulationRenderMode mode) {

    return mode == RuntimePopulationRenderMode::Quads ? QuadVerticesPerAgent : PointVerticesPerAgent;
}

sf::PrimitiveType GetPrimitiveType(const RuntimePopulationRenderMode mode) {

    return mode == RuntimePopulationRenderMode::Quads ? sf::PrimitiveType::Triangles : sf::PrimitiveType::Points;
}

bool IsFinite(const sf::Vector2f value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool IsValidViewport(const sf::FloatRect &viewport) {
    return IsFinite(viewport.position) && IsFinite(viewport.size) && viewport.size.x > 0.0f && viewport.size.y > 0.0f;
}

bool IsPopulationVisible(const RuntimeAgentPopulation &population, const sf::FloatRect &worldViewport) {

    const editor::SceneTransform &transform = population.GetTransform();

    const sf::Vector2f spawnSize = population.GetSpawnAreaSize();

    if (!IsFinite(transform.position) || !std::isfinite(transform.rotation) || !IsFinite(spawnSize) ||
        spawnSize.x <= 0.0f || spawnSize.y <= 0.0f) {
        return false;
    }

    const sf::Vector2f halfSize = spawnSize * 0.5f;

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

sf::FloatRect CalculateLocalViewport(const sf::FloatRect &worldViewport,
                                     const sf::Transform &inversePopulationTransform) {

    const sf::Vector2f worldMinimum = worldViewport.position;

    const sf::Vector2f worldMaximum = worldViewport.position + worldViewport.size;

    const std::array<sf::Vector2f, 4> worldCorners{
        worldMinimum,
        sf::Vector2f{worldMaximum.x, worldMinimum.y},
        worldMaximum,
        sf::Vector2f{worldMinimum.x, worldMaximum.y},
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

void RuntimePopulationPointRenderer::BeginFrame() {
    frameStats = {};
    frameStats.renderMode = mode;
}

void RuntimePopulationPointRenderer::SetMode(const RuntimePopulationRenderMode newMode) {

    if (mode == newMode) {
        return;
    }

    mode = newMode;
    vertices.clear();
}

RuntimePopulationRenderMode RuntimePopulationPointRenderer::GetMode() const { return mode; }

const RuntimePopulationPointRenderStats &RuntimePopulationPointRenderer::GetFrameStats() const { return frameStats; }

void RuntimePopulationPointRenderer::Render(sf::RenderTarget &target, const RuntimeAgentPopulation &population,
                                            const sf::FloatRect &worldViewport) {

    if (!IsValidViewport(worldViewport)) {
        return;
    }

    const std::vector<float> &positionX = population.GetPositionX();

    const std::vector<float> &positionY = population.GetPositionY();

    const std::size_t agentCount = std::min({
        population.GetCount(),
        positionX.size(),
        positionY.size(),
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()),
    });

    if (agentCount == 0 || !IsPopulationVisible(population, worldViewport)) {
        return;
    }

    const editor::SceneTransform &sourceTransform = population.GetTransform();

    sf::Transformable transformable;
    transformable.setPosition(sourceTransform.position);
    transformable.setRotation(sf::degrees(sourceTransform.rotation));

    const sf::Transform populationTransform = transformable.getTransform();

    const sf::Transform inversePopulationTransform = transformable.getInverseTransform();

    const sf::FloatRect localViewport = CalculateLocalViewport(worldViewport, inversePopulationTransform);

    const RuntimePopulationSpatialGrid &spatialGrid = population.GetSpatialGrid();

    const float gridCellSize = spatialGrid.GetCellSize();

    const float queryPadding = std::isfinite(gridCellSize) && gridCellSize > 0.0f ? gridCellSize : 0.0f;

    const sf::FloatRect paddedLocalViewport{
        localViewport.position - sf::Vector2f{queryPadding, queryPadding},

        localViewport.size +
            sf::Vector2f{
                queryPadding * 2.0f,
                queryPadding * 2.0f,
            },
    };

    const RuntimePopulationSpatialGrid::CellRange cellRange = spatialGrid.GetCellsOverlapping(paddedLocalViewport);

    if (cellRange.IsEmpty()) {
        return;
    }

    const auto geometryStartTime = std::chrono::steady_clock::now();

    const std::size_t verticesPerAgent = GetVerticesPerAgent(mode);

    if (agentCount > vertices.max_size() / verticesPerAgent) {
        return;
    }

    const std::size_t maximumVertexCount = agentCount * verticesPerAgent;

    if (vertices.size() != maximumVertexCount) {
        vertices.resize(maximumVertexCount);

        for (sf::Vertex &vertex : vertices) {
            vertex.color = AgentColor;
        }
    }

    std::size_t visibleAgentCount = 0;
    std::size_t visibleVertexCount = 0;

    for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {

        for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {

            const std::span<const std::uint32_t> agentIndices = spatialGrid.GetAgentIndices(column, row);

            frameStats.candidateAgentCount += agentIndices.size();

            for (const std::uint32_t agentIndex : agentIndices) {

                if (agentIndex >= agentCount) {
                    continue;
                }

                const sf::Vector2f localPosition{
                    positionX[agentIndex],
                    positionY[agentIndex],
                };

                if (!IsFinite(localPosition)) {
                    continue;
                }

                const sf::Vector2f worldPosition = populationTransform.transformPoint(localPosition);

                if (!worldViewport.contains(worldPosition)) {
                    continue;
                }

                if (mode == RuntimePopulationRenderMode::Points) {

                    vertices[visibleVertexCount].position = localPosition;

                    ++visibleVertexCount;
                } else {
                    const sf::Vector2f topLeft = localPosition + sf::Vector2f{
                                                                     -AgentHalfSize,
                                                                     -AgentHalfSize,
                                                                 };

                    const sf::Vector2f topRight = localPosition + sf::Vector2f{
                                                                      AgentHalfSize,
                                                                      -AgentHalfSize,
                                                                  };

                    const sf::Vector2f bottomRight = localPosition + sf::Vector2f{
                                                                         AgentHalfSize,
                                                                         AgentHalfSize,
                                                                     };

                    const sf::Vector2f bottomLeft = localPosition + sf::Vector2f{
                                                                        -AgentHalfSize,
                                                                        AgentHalfSize,
                                                                    };

                    const std::array<sf::Vector2f, 6> quadPositions{
                        topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft,
                    };

                    for (std::size_t vertexIndex = 0; vertexIndex < QuadVerticesPerAgent; ++vertexIndex) {

                        vertices[visibleVertexCount + vertexIndex].position = quadPositions[vertexIndex];
                    }

                    visibleVertexCount += QuadVerticesPerAgent;
                }

                ++visibleAgentCount;
            }
        }
    }

    frameStats.visibleAgentCount += visibleAgentCount;

    frameStats.vertexCount += visibleVertexCount;

    const auto geometryEndTime = std::chrono::steady_clock::now();

    frameStats.geometryBuildTimeMs +=
        std::chrono::duration<float, std::milli>(geometryEndTime - geometryStartTime).count();

    if (visibleVertexCount == 0) {
        return;
    }

    const sf::RenderStates renderStates{
        populationTransform,
    };

    target.draw(vertices.data(), visibleVertexCount, GetPrimitiveType(mode), renderStates);
}

} // namespace pipeframe::runtime
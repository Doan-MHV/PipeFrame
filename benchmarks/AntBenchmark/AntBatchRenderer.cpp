#include "AntBatchRenderer.h"

namespace {
constexpr std::size_t QuadVerticesPerAnt = 6;
constexpr std::size_t PointVerticesPerAnt = 1;

const sf::Color AntColor{232, 91, 116};

std::size_t GetVerticesPerAnt(AntRenderMode mode) {
    if (mode == AntRenderMode::Quads) {
        return QuadVerticesPerAnt;
    }

    return PointVerticesPerAnt;
}

sf::PrimitiveType GetPrimitiveType(AntRenderMode mode) {
    if (mode == AntRenderMode::Quads) {
        return sf::PrimitiveType::Triangles;
    }

    return sf::PrimitiveType::Points;
}
} // namespace

void AntBatchRenderer::Initialize(std::size_t antCount, AntRenderMode renderMode) {
    mode = renderMode;

    vertices.resize(antCount * GetVerticesPerAnt(mode));

    candidateAntCount = 0;
    visibleAntCount = 0;
    activeVertexCount = 0;

    InitializeVertexColors();
}

void AntBatchRenderer::SetMode(AntRenderMode renderMode, std::size_t antCount) {
    if (mode == renderMode) {
        return;
    }

    Initialize(antCount, renderMode);
}

void AntBatchRenderer::InitializeVertexColors() {
    for (sf::Vertex &vertex : vertices) {
        vertex.color = AntColor;
    }
}

void AntBatchRenderer::UpdateGeometry(const AntPopulation &population, const AntSpatialGrid &spatialGrid,
                                      const sf::FloatRect &viewport) {
    const std::vector<Ant> &ants = population.GetAnts();

    const std::size_t requiredCapacity = ants.size() * GetVerticesPerAnt(mode);

    if (vertices.size() != requiredCapacity) {
        Initialize(ants.size(), mode);
    }

    candidateAntCount = 0;
    visibleAntCount = 0;
    activeVertexCount = 0;

    sf::FloatRect visibleArea = viewport;

    if (mode == AntRenderMode::Quads) {
        visibleArea.position.x -= AntHalfSize;
        visibleArea.position.y -= AntHalfSize;

        visibleArea.size.x += AntHalfSize * 2.0f;
        visibleArea.size.y += AntHalfSize * 2.0f;
    }

    const AntSpatialGrid::CellRange cellRange = spatialGrid.GetCellsOverlapping(visibleArea);

    if (cellRange.IsEmpty()) {
        return;
    }

    if (mode == AntRenderMode::Points) {
        for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {
            for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {
                const std::span<const std::uint32_t> antIndices = spatialGrid.GetAgentIndices(column, row);

                candidateAntCount += antIndices.size();

                for (const std::uint32_t antIndex : antIndices) {
                    const Ant &ant = ants[antIndex];

                    // Grid cells overlap the camera but may extend
                    // beyond its exact boundaries.
                    if (!visibleArea.contains(ant.position)) {
                        continue;
                    }

                    vertices[activeVertexCount].position = ant.position;

                    ++activeVertexCount;
                    ++visibleAntCount;
                }
            }
        }

        return;
    }

    for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {
        for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {
            const std::span<const std::uint32_t> antIndices = spatialGrid.GetAgentIndices(column, row);

            candidateAntCount += antIndices.size();

            for (const std::uint32_t antIndex : antIndices) {
                const Ant &ant = ants[antIndex];

                if (!visibleArea.contains(ant.position)) {
                    continue;
                }

                const sf::Vector2f position = ant.position;

                const float left = position.x - AntHalfSize;

                const float right = position.x + AntHalfSize;

                const float top = position.y - AntHalfSize;

                const float bottom = position.y + AntHalfSize;

                const std::size_t vertexIndex = activeVertexCount;

                vertices[vertexIndex + 0].position = {left, top};

                vertices[vertexIndex + 1].position = {right, top};

                vertices[vertexIndex + 2].position = {right, bottom};

                vertices[vertexIndex + 3].position = {left, top};

                vertices[vertexIndex + 4].position = {right, bottom};

                vertices[vertexIndex + 5].position = {left, bottom};

                activeVertexCount += QuadVerticesPerAnt;
                ++visibleAntCount;
            }
        }
    }
}

void AntBatchRenderer::Draw(sf::RenderTarget &target) const {
    if (activeVertexCount == 0) {
        return;
    }

    target.draw(vertices.data(), activeVertexCount, GetPrimitiveType(mode));
}

AntRenderMode AntBatchRenderer::GetMode() const { return mode; }

std::size_t AntBatchRenderer::GetVisibleAntCount() const { return visibleAntCount; }

std::size_t AntBatchRenderer::GetVertexCount() const { return activeVertexCount; }

std::size_t AntBatchRenderer::GetCandidateAntCount() const { return candidateAntCount; }
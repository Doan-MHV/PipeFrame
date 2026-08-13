#include "AntBatchRenderer.h"

namespace {
constexpr std::size_t VerticesPerAnt = 6;

const sf::Color AntColor{232, 91, 116};
} // namespace

void AntBatchRenderer::Initialize(std::size_t antCount) {
    vertices.resize(antCount * VerticesPerAnt);

    for (std::size_t index = 0; index < vertices.getVertexCount(); ++index) {
        vertices[index].color = AntColor;
    }
}

void AntBatchRenderer::UpdateGeometry(const AntPopulation &population) {
    const std::vector<Ant> &ants = population.GetAnts();

    if (vertices.getVertexCount() != ants.size() * VerticesPerAnt) {
        Initialize(ants.size());
    }

    for (std::size_t antIndex = 0; antIndex < ants.size(); ++antIndex) {
        const sf::Vector2f position = ants[antIndex].position;

        const float left = position.x - AntHalfSize;

        const float right = position.x + AntHalfSize;

        const float top = position.y - AntHalfSize;

        const float bottom = position.y + AntHalfSize;

        const std::size_t vertexIndex = antIndex * VerticesPerAnt;

        // First triangle
        vertices[vertexIndex + 0].position = {left, top};

        vertices[vertexIndex + 1].position = {right, top};

        vertices[vertexIndex + 2].position = {right, bottom};

        // Second triangle
        vertices[vertexIndex + 3].position = {left, top};

        vertices[vertexIndex + 4].position = {right, bottom};

        vertices[vertexIndex + 5].position = {left, bottom};
    }
}

void AntBatchRenderer::Draw(sf::RenderTarget &target) const { target.draw(vertices); }
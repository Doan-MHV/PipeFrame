#include "RuntimePopulationPointRenderer.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Angle.hpp>

namespace pipeframe::runtime {

namespace {

const sf::Color AgentColor{232, 91, 116};

} // namespace

void RuntimePopulationPointRenderer::Render(sf::RenderTarget &target, const RuntimeAgentPopulation &population) {

    const std::size_t agentCount = population.GetCount();

    if (agentCount == 0) {
        return;
    }

    if (vertices.size() != agentCount) {
        vertices.resize(agentCount);

        for (sf::Vertex &vertex : vertices) {
            vertex.color = AgentColor;
        }
    }

    const std::vector<float> &positionX = population.GetPositionX();
    const std::vector<float> &positionY = population.GetPositionY();

    for (std::size_t index = 0; index < agentCount; ++index) {
        vertices[index].position = {
            positionX[index],
            positionY[index],
        };
    }

    const editor::SceneTransform &sourceTransform = population.GetTransform();

    sf::Transformable transformable;

    transformable.setPosition(sourceTransform.position);
    transformable.setRotation(sf::degrees(sourceTransform.rotation));

    const sf::RenderStates renderStates{
        transformable.getTransform(),
    };

    target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Points, renderStates);
}

} // namespace pipeframe::runtime
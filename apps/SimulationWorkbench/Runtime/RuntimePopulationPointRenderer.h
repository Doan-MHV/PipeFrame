#ifndef PIPEFRAME_RUNTIME_POPULATION_POINT_RENDERER_H
#define PIPEFRAME_RUNTIME_POPULATION_POINT_RENDERER_H

#include "RuntimeAgentPopulation.h"

#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>

namespace pipeframe::runtime {

class RuntimePopulationPointRenderer final {
  public:
    void Render(sf::RenderTarget &target, const RuntimeAgentPopulation &population);

  private:
    std::vector<sf::Vertex> vertices;
};

} // namespace pipeframe::runtime

#endif
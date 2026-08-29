#ifndef PIPEFRAME_RUNTIME_POPULATION_POINT_RENDERER_H
#define PIPEFRAME_RUNTIME_POPULATION_POINT_RENDERER_H

#include "RuntimeAgentPopulation.h"

#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>

namespace pipeframe::runtime {

enum class RuntimePopulationRenderMode {
    Points,
    Quads,
};

struct RuntimePopulationPointRenderStats {
    std::size_t candidateAgentCount = 0;
    std::size_t visibleAgentCount = 0;
    std::size_t vertexCount = 0;
    float geometryBuildTimeMs = 0.0f;

    RuntimePopulationRenderMode renderMode = RuntimePopulationRenderMode::Points;
};

class RuntimePopulationPointRenderer final {
  public:
    void BeginFrame();

    void Render(sf::RenderTarget &target, const RuntimeAgentPopulation &population, const sf::FloatRect &worldViewport);

    const RuntimePopulationPointRenderStats &GetFrameStats() const;

    void SetMode(RuntimePopulationRenderMode newMode);

    RuntimePopulationRenderMode GetMode() const;

  private:
    std::vector<sf::Vertex> vertices;
    RuntimePopulationPointRenderStats frameStats;
    RuntimePopulationRenderMode mode = RuntimePopulationRenderMode::Points;
};

} // namespace pipeframe::runtime

#endif
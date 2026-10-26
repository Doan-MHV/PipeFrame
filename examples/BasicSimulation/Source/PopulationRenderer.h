#ifndef BASIC_SIMULATION_POPULATION_RENDERER_H
#define BASIC_SIMULATION_POPULATION_RENDERER_H

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include "Population.h"
#include "PopulationLod.h"

namespace basic_simulation {

struct PopulationRenderStats final {
    std::size_t candidateAgentCount = 0;
    std::size_t visibleAgentCount = 0;
    std::size_t vertexCount = 0;

    float geometryBuildTimeMilliseconds = 0.0f;

    PopulationRenderMode renderMode =
        PopulationRenderMode::Points;
};

class PopulationRenderer final {
public:
    void BeginFrame();

    void Render(
        sf::RenderTarget &target,
        const Population &population,
        const sf::FloatRect &worldViewport);

    void SetMode(PopulationRenderMode mode);

    PopulationRenderMode GetMode() const;

    const PopulationRenderStats &
    GetFrameStats() const;

private:
    std::vector<sf::Vertex> vertices;

    PopulationRenderStats frameStats;

    PopulationRenderMode mode =
        PopulationRenderMode::Points;
};

} // namespace basic_simulation

#endif
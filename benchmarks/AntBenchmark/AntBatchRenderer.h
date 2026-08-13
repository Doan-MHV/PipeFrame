#ifndef PIPEFRAME_ANT_BATCH_RENDERER_H
#define PIPEFRAME_ANT_BATCH_RENDERER_H

#include <cstddef>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "AntPopulation.h"

class AntBatchRenderer {
  public:
    void Initialize(std::size_t antCount);
    void UpdateGeometry(const AntPopulation &population);

    void Draw(sf::RenderTarget &target) const;

  private:
    static constexpr float AntHalfSize = 2.5f;

    sf::VertexArray vertices{sf::PrimitiveType::Triangles};
};

#endif
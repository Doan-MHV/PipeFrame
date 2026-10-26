#ifndef PIPEFRAME_ANT_BATCH_RENDERER_H
#define PIPEFRAME_ANT_BATCH_RENDERER_H

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include "AntPopulation.h"
#include "AntSpatialGrid.h"

enum class AntRenderMode { Quads, Points };

class AntBatchRenderer {
  public:
    void Initialize(std::size_t antCount, AntRenderMode renderMode);

    void SetMode(AntRenderMode renderMode, std::size_t antCount);

    void UpdateGeometry(const AntPopulation &population, const AntSpatialGrid &spatialGrid,
                        const sf::FloatRect &viewport);

    void Draw(sf::RenderTarget &target) const;

    AntRenderMode GetMode() const;

    std::size_t GetCandidateAntCount() const;
    std::size_t GetVisibleAntCount() const;
    std::size_t GetVertexCount() const;

  private:
    void InitializeVertexColors();

    static constexpr float AntHalfSize = 0.35f;

    AntRenderMode mode = AntRenderMode::Points;

    std::vector<sf::Vertex> vertices;

    std::size_t visibleAntCount = 0;
    std::size_t activeVertexCount = 0;
    std::size_t candidateAntCount = 0;
};

#endif
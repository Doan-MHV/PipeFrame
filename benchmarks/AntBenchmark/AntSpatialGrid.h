#ifndef PIPEFRAME_ANT_SPATIAL_GRID_H
#define PIPEFRAME_ANT_SPATIAL_GRID_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "AntPopulation.h"

class AntSpatialGrid {
  public:
    struct CellRange {
        int minimumColumn = 0;
        int maximumColumn = -1;
        int minimumRow = 0;
        int maximumRow = -1;

        bool IsEmpty() const { return maximumColumn < minimumColumn || maximumRow < minimumRow; }
    };

    void Initialize(sf::Vector2f worldMinimum, sf::Vector2f worldSize, float cellSize, std::size_t maximumAntCount);

    void Rebuild(const AntPopulation &population);

    CellRange GetCellsOverlapping(const sf::FloatRect &area) const;

    std::span<const std::uint32_t> GetAgentIndices(int column, int row) const;

    int GetColumnCount() const;
    int GetRowCount() const;
    float GetCellSize() const;

  private:
    std::uint32_t GetCellIndex(sf::Vector2f position) const;

    std::uint32_t GetCellIndex(int column, int row) const;

    sf::Vector2i GetCellCoordinates(sf::Vector2f position) const;

    sf::Vector2f worldMinimum{0.0f, 0.0f};
    sf::Vector2f worldSize{0.0f, 0.0f};

    float cellSize = 1.0f;
    float inverseCellSize = 1.0f;

    int columnCount = 0;
    int rowCount = 0;

    std::vector<std::uint32_t> cellCounts;
    std::vector<std::uint32_t> cellOffsets;
    std::vector<std::uint32_t> writeOffsets;

    std::vector<std::uint32_t> antCellIndices;
    std::vector<std::uint32_t> agentIndices;
};

#endif
#include "RuntimePopulationSpatialGrid.h"

#include <algorithm>
#include <cmath>

namespace pipeframe::runtime {

void RuntimePopulationSpatialGrid::Initialize(sf::Vector2f newLocalMinimum, sf::Vector2f newLocalSize,
                                              float newCellSize, std::size_t maximumAgentCount) {
    localMinimum = newLocalMinimum;
    localSize = newLocalSize;

    cellSize = std::max(newCellSize, 1.0f);
    inverseCellSize = 1.0f / cellSize;

    columnCount = std::max(1, static_cast<int>(std::ceil(localSize.x / cellSize)));

    rowCount = std::max(1, static_cast<int>(std::ceil(localSize.y / cellSize)));

    const std::size_t totalCellCount = static_cast<std::size_t>(columnCount) * static_cast<std::size_t>(rowCount);

    cellCounts.resize(totalCellCount);
    cellOffsets.resize(totalCellCount + 1);
    writeOffsets.resize(totalCellCount);

    agentCellIndices.resize(maximumAgentCount);
    agentIndices.resize(maximumAgentCount);
}

void RuntimePopulationSpatialGrid::Clear() {
    localMinimum = {};
    localSize = {};

    cellSize = 1.0f;
    inverseCellSize = 1.0f;

    columnCount = 0;
    rowCount = 0;

    cellCounts.clear();
    cellOffsets.clear();
    writeOffsets.clear();
    agentCellIndices.clear();
    agentIndices.clear();
}

void RuntimePopulationSpatialGrid::Rebuild(const std::vector<float> &positionX, const std::vector<float> &positionY) {
    if (columnCount == 0 || rowCount == 0) {
        return;
    }

    const std::size_t agentCount = positionX.size();

    if (agentCellIndices.size() != agentCount) {
        agentCellIndices.resize(agentCount);
        agentIndices.resize(agentCount);
    }

    std::fill(cellCounts.begin(), cellCounts.end(), std::uint32_t{0});

    // Pass 1: calculate each agent's cell and count occupancy.
    for (std::size_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {

        const sf::Vector2f position{
            positionX[agentIndex],
            positionY[agentIndex],
        };

        const std::uint32_t cellIndex = GetCellIndex(position);

        agentCellIndices[agentIndex] = cellIndex;
        ++cellCounts[cellIndex];
    }

    // Prefix sum: determine where each cell begins.
    cellOffsets[0] = 0;

    for (std::size_t cellIndex = 0; cellIndex < cellCounts.size(); ++cellIndex) {

        cellOffsets[cellIndex + 1] = cellOffsets[cellIndex] + cellCounts[cellIndex];

        writeOffsets[cellIndex] = cellOffsets[cellIndex];
    }

    // Pass 2: write each agent index into its cell.
    for (std::size_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {

        const std::uint32_t cellIndex = agentCellIndices[agentIndex];

        const std::uint32_t outputIndex = writeOffsets[cellIndex]++;

        agentIndices[outputIndex] = static_cast<std::uint32_t>(agentIndex);
    }
}

RuntimePopulationSpatialGrid::CellRange
RuntimePopulationSpatialGrid::GetCellsOverlapping(const sf::FloatRect &area) const {
    if (columnCount == 0 || rowCount == 0) {
        return {};
    }

    const sf::Vector2f areaEnd{
        area.position.x + area.size.x,
        area.position.y + area.size.y,
    };

    const sf::Vector2f gridEnd{
        localMinimum.x + localSize.x,
        localMinimum.y + localSize.y,
    };

    if (areaEnd.x < localMinimum.x || areaEnd.y < localMinimum.y || area.position.x > gridEnd.x ||
        area.position.y > gridEnd.y) {
        return {};
    }

    const sf::Vector2i minimumCell = GetCellCoordinates(area.position);

    const sf::Vector2i maximumCell = GetCellCoordinates(areaEnd);

    return {
        minimumCell.x,
        maximumCell.x,
        minimumCell.y,
        maximumCell.y,
    };
}

std::span<const std::uint32_t> RuntimePopulationSpatialGrid::GetAgentIndices(int column, int row) const {
    const std::uint32_t cellIndex = GetCellIndex(column, row);

    const std::uint32_t begin = cellOffsets[cellIndex];

    const std::uint32_t end = cellOffsets[cellIndex + 1];

    return {
        agentIndices.data() + begin,
        static_cast<std::size_t>(end - begin),
    };
}

std::uint32_t RuntimePopulationSpatialGrid::GetCellIndex(sf::Vector2f position) const {
    const sf::Vector2i coordinates = GetCellCoordinates(position);

    return GetCellIndex(coordinates.x, coordinates.y);
}

std::uint32_t RuntimePopulationSpatialGrid::GetCellIndex(int column, int row) const {
    return static_cast<std::uint32_t>(row * columnCount + column);
}

sf::Vector2i RuntimePopulationSpatialGrid::GetCellCoordinates(sf::Vector2f position) const {
    int column = static_cast<int>((position.x - localMinimum.x) * inverseCellSize);

    int row = static_cast<int>((position.y - localMinimum.y) * inverseCellSize);

    column = std::clamp(column, 0, columnCount - 1);
    row = std::clamp(row, 0, rowCount - 1);

    return {column, row};
}

int RuntimePopulationSpatialGrid::GetColumnCount() const { return columnCount; }

int RuntimePopulationSpatialGrid::GetRowCount() const { return rowCount; }

float RuntimePopulationSpatialGrid::GetCellSize() const { return cellSize; }

} // namespace pipeframe::runtime
#include "AntSpatialGrid.h"

#include <algorithm>
#include <cmath>

void AntSpatialGrid::Initialize(sf::Vector2f newWorldMinimum, sf::Vector2f newWorldSize, float newCellSize,
                                std::size_t maximumAntCount) {
    worldMinimum = newWorldMinimum;
    worldSize = newWorldSize;
    cellSize = newCellSize;
    inverseCellSize = 1.0f / cellSize;

    columnCount = static_cast<int>(std::ceil(worldSize.x / cellSize));

    rowCount = static_cast<int>(std::ceil(worldSize.y / cellSize));

    const std::size_t totalCellCount = static_cast<std::size_t>(columnCount) * static_cast<std::size_t>(rowCount);

    cellCounts.resize(totalCellCount);
    cellOffsets.resize(totalCellCount + 1);
    writeOffsets.resize(totalCellCount);

    antCellIndices.resize(maximumAntCount);
    agentIndices.resize(maximumAntCount);
}

void AntSpatialGrid::Rebuild(const AntPopulation &population) {
    const std::vector<Ant> &ants = population.GetAnts();

    if (antCellIndices.size() != ants.size()) {
        antCellIndices.resize(ants.size());
        agentIndices.resize(ants.size());
    }

    std::fill(cellCounts.begin(), cellCounts.end(), std::uint32_t{0});

    // Pass one: calculate each ant's cell and count occupancy.
    for (std::size_t antIndex = 0; antIndex < ants.size(); ++antIndex) {
        const std::uint32_t cellIndex = GetCellIndex(ants[antIndex].position);

        antCellIndices[antIndex] = cellIndex;
        ++cellCounts[cellIndex];
    }

    // Prefix sum: determine where each cell begins.
    cellOffsets[0] = 0;

    for (std::size_t cellIndex = 0; cellIndex < cellCounts.size(); ++cellIndex) {
        cellOffsets[cellIndex + 1] = cellOffsets[cellIndex] + cellCounts[cellIndex];

        writeOffsets[cellIndex] = cellOffsets[cellIndex];
    }

    // Pass two: place each ant index into its cell's range.
    for (std::size_t antIndex = 0; antIndex < ants.size(); ++antIndex) {
        const std::uint32_t cellIndex = antCellIndices[antIndex];

        const std::uint32_t outputIndex = writeOffsets[cellIndex]++;

        agentIndices[outputIndex] = static_cast<std::uint32_t>(antIndex);
    }
}

AntSpatialGrid::CellRange AntSpatialGrid::GetCellsOverlapping(const sf::FloatRect &area) const {
    const sf::Vector2f areaEnd{area.position.x + area.size.x, area.position.y + area.size.y};

    const sf::Vector2f worldEnd{worldMinimum.x + worldSize.x, worldMinimum.y + worldSize.y};

    if (areaEnd.x < worldMinimum.x || areaEnd.y < worldMinimum.y || area.position.x > worldEnd.x ||
        area.position.y > worldEnd.y) {
        return {};
    }

    const sf::Vector2i minimumCell = GetCellCoordinates(area.position);

    const sf::Vector2i maximumCell = GetCellCoordinates(areaEnd);

    return {minimumCell.x, maximumCell.x, minimumCell.y, maximumCell.y};
}

std::span<const std::uint32_t> AntSpatialGrid::GetAgentIndices(int column, int row) const {
    const std::uint32_t cellIndex = GetCellIndex(column, row);

    const std::uint32_t begin = cellOffsets[cellIndex];

    const std::uint32_t end = cellOffsets[cellIndex + 1];

    return {agentIndices.data() + begin, static_cast<std::size_t>(end - begin)};
}

std::uint32_t AntSpatialGrid::GetCellIndex(sf::Vector2f position) const {
    const sf::Vector2i coordinates = GetCellCoordinates(position);

    return GetCellIndex(coordinates.x, coordinates.y);
}

std::uint32_t AntSpatialGrid::GetCellIndex(int column, int row) const {
    return static_cast<std::uint32_t>(row * columnCount + column);
}

sf::Vector2i AntSpatialGrid::GetCellCoordinates(sf::Vector2f position) const {
    int column = static_cast<int>((position.x - worldMinimum.x) * inverseCellSize);

    int row = static_cast<int>((position.y - worldMinimum.y) * inverseCellSize);

    column = std::clamp(column, 0, columnCount - 1);

    row = std::clamp(row, 0, rowCount - 1);

    return {column, row};
}

int AntSpatialGrid::GetColumnCount() const { return columnCount; }

int AntSpatialGrid::GetRowCount() const { return rowCount; }

float AntSpatialGrid::GetCellSize() const { return cellSize; }
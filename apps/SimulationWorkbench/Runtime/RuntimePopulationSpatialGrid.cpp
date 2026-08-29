#include "RuntimePopulationSpatialGrid.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pipeframe::runtime {

namespace {

constexpr int MaximumGridDimension = 1024;

bool IsFinite(const sf::Vector2f value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

} // namespace

void RuntimePopulationSpatialGrid::Initialize(
    const sf::Vector2f newLocalMinimum,
    const sf::Vector2f newLocalSize,
    const float newCellSize,
    const std::size_t maximumAgentCount) {

    Clear();

    if (!IsFinite(newLocalMinimum) ||
        !IsFinite(newLocalSize) ||
        newLocalSize.x <= 0.0f ||
        newLocalSize.y <= 0.0f) {
        return;
    }

    localMinimum = newLocalMinimum;
    localSize = newLocalSize;

    float requestedCellSize = newCellSize;

    if (!std::isfinite(requestedCellSize) ||
        requestedCellSize <= 0.0f) {
        requestedCellSize = 1.0f;
    }

    // Prevent malformed scenes from allocating an enormous grid.
    const float minimumSafeCellSize = std::max(
        newLocalSize.x / static_cast<float>(MaximumGridDimension),
        newLocalSize.y / static_cast<float>(MaximumGridDimension));

    cellSize = std::max({
        1.0f,
        requestedCellSize,
        minimumSafeCellSize,
    });

    inverseCellSize = 1.0f / cellSize;

    columnCount = std::clamp(
        static_cast<int>(std::ceil(localSize.x / cellSize)),
        1,
        MaximumGridDimension);

    rowCount = std::clamp(
        static_cast<int>(std::ceil(localSize.y / cellSize)),
        1,
        MaximumGridDimension);

    const std::size_t totalCellCount =
        static_cast<std::size_t>(columnCount) *
        static_cast<std::size_t>(rowCount);

    cellCounts.assign(totalCellCount, 0);
    cellOffsets.assign(totalCellCount + 1, 0);
    writeOffsets.assign(totalCellCount, 0);

    const std::size_t safeMaximumAgentCount = std::min(
        maximumAgentCount,
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max()));

    agentCellIndices.resize(safeMaximumAgentCount);
    agentIndices.resize(safeMaximumAgentCount);
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

void RuntimePopulationSpatialGrid::Rebuild(
    const std::vector<float> &positionX,
    const std::vector<float> &positionY) {

    if (columnCount <= 0 ||
        rowCount <= 0 ||
        cellCounts.empty() ||
        cellOffsets.size() != cellCounts.size() + 1) {
        return;
    }

    const std::size_t agentCount = std::min({
        positionX.size(),
        positionY.size(),
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max()),
    });

    agentCellIndices.resize(agentCount);
    agentIndices.resize(agentCount);

    std::fill(
        cellCounts.begin(),
        cellCounts.end(),
        std::uint32_t{0});

    // Pass 1: calculate each agent's cell and count occupancy.
    for (std::size_t agentIndex = 0;
         agentIndex < agentCount;
         ++agentIndex) {

        const sf::Vector2f position{
            positionX[agentIndex],
            positionY[agentIndex],
        };

        const std::uint32_t cellIndex =
            GetCellIndex(position);

        agentCellIndices[agentIndex] = cellIndex;
        ++cellCounts[cellIndex];
    }

    cellOffsets[0] = 0;

    // Prefix sum: determine where each cell begins.
    for (std::size_t cellIndex = 0;
         cellIndex < cellCounts.size();
         ++cellIndex) {

        cellOffsets[cellIndex + 1] =
            cellOffsets[cellIndex] + cellCounts[cellIndex];

        writeOffsets[cellIndex] =
            cellOffsets[cellIndex];
    }

    // Pass 2: write each agent index into its cell.
    for (std::size_t agentIndex = 0;
         agentIndex < agentCount;
         ++agentIndex) {

        const std::uint32_t cellIndex =
            agentCellIndices[agentIndex];

        const std::uint32_t outputIndex =
            writeOffsets[cellIndex]++;

        if (outputIndex >= agentIndices.size()) {
            continue;
        }

        agentIndices[outputIndex] =
            static_cast<std::uint32_t>(agentIndex);
    }
}

RuntimePopulationSpatialGrid::CellRange
RuntimePopulationSpatialGrid::GetCellsOverlapping(
    const sf::FloatRect &area) const {

    if (columnCount <= 0 ||
        rowCount <= 0 ||
        !IsFinite(area.position) ||
        !IsFinite(area.size) ||
        area.size.x <= 0.0f ||
        area.size.y <= 0.0f) {
        return {};
    }

    const sf::Vector2f areaEnd =
        area.position + area.size;

    const sf::Vector2f gridEnd =
        localMinimum + localSize;

    if (areaEnd.x < localMinimum.x ||
        areaEnd.y < localMinimum.y ||
        area.position.x > gridEnd.x ||
        area.position.y > gridEnd.y) {
        return {};
    }

    const sf::Vector2i minimumCell =
        GetCellCoordinates(area.position);

    const sf::Vector2i maximumCell =
        GetCellCoordinates(areaEnd);

    return {
        minimumCell.x,
        maximumCell.x,
        minimumCell.y,
        maximumCell.y,
    };
}

std::span<const std::uint32_t>
RuntimePopulationSpatialGrid::GetAgentIndices(
    const int column,
    const int row) const {

    if (column < 0 ||
        column >= columnCount ||
        row < 0 ||
        row >= rowCount ||
        cellOffsets.empty()) {
        return {};
    }

    const std::uint32_t cellIndex =
        GetCellIndex(column, row);

    if (static_cast<std::size_t>(cellIndex + 1) >=
        cellOffsets.size()) {
        return {};
    }

    const std::uint32_t begin =
        cellOffsets[cellIndex];

    const std::uint32_t end =
        cellOffsets[cellIndex + 1];

    if (begin > end ||
        end > agentIndices.size()) {
        return {};
    }

    return {
        agentIndices.data() + begin,
        static_cast<std::size_t>(end - begin),
    };
}

std::uint32_t RuntimePopulationSpatialGrid::GetCellIndex(
    const sf::Vector2f position) const {

    const sf::Vector2i coordinates =
        GetCellCoordinates(position);

    return GetCellIndex(coordinates.x, coordinates.y);
}

std::uint32_t RuntimePopulationSpatialGrid::GetCellIndex(
    const int column,
    const int row) const {

    return static_cast<std::uint32_t>(
        row * columnCount + column);
}

sf::Vector2i RuntimePopulationSpatialGrid::GetCellCoordinates(
    const sf::Vector2f position) const {

    if (columnCount <= 0 ||
        rowCount <= 0 ||
        !IsFinite(position)) {
        return {0, 0};
    }

    int column = static_cast<int>(std::floor(
        (position.x - localMinimum.x) * inverseCellSize));

    int row = static_cast<int>(std::floor(
        (position.y - localMinimum.y) * inverseCellSize));

    column = std::clamp(column, 0, columnCount - 1);
    row = std::clamp(row, 0, rowCount - 1);

    return {column, row};
}

int RuntimePopulationSpatialGrid::GetColumnCount() const {
    return columnCount;
}

int RuntimePopulationSpatialGrid::GetRowCount() const {
    return rowCount;
}

float RuntimePopulationSpatialGrid::GetCellSize() const {
    return cellSize;
}

} // namespace pipeframe::runtime
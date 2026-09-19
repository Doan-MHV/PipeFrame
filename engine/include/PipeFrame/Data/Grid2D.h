#ifndef PIPEFRAME_DATA_GRID2D_H
#define PIPEFRAME_DATA_GRID2D_H

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace pipeframe {

struct GridCoordinate {
    int column{};
    int row{};
    constexpr bool operator==(const GridCoordinate&) const = default;
};

struct GridCellRange {
    GridCoordinate minimum{};
    GridCoordinate maximum{-1, -1};
    [[nodiscard]] constexpr bool IsEmpty() const {
        return maximum.column < minimum.column || maximum.row < minimum.row;
    }
};

template <typename T>
class Grid2D {
public:
    Grid2D() = default;
    Grid2D(int columns, int rows, const T& value = T{}) { Resize(columns, rows, value); }

    void Resize(int columns, int rows, const T& value = T{}) {
        if (columns < 0 || rows < 0) throw std::invalid_argument("Grid dimensions cannot be negative");
        columnCount = columns;
        rowCount = rows;
        cells.assign(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows), value);
    }

    [[nodiscard]] int Columns() const { return columnCount; }
    [[nodiscard]] int Rows() const { return rowCount; }
    [[nodiscard]] std::size_t Size() const { return cells.size(); }
    [[nodiscard]] bool InBounds(GridCoordinate coordinate) const {
        return coordinate.column >= 0 && coordinate.column < columnCount && coordinate.row >= 0 &&
               coordinate.row < rowCount;
    }
    [[nodiscard]] GridCoordinate Clamp(GridCoordinate coordinate) const {
        if (cells.empty()) return {};
        return {std::clamp(coordinate.column, 0, columnCount - 1), std::clamp(coordinate.row, 0, rowCount - 1)};
    }
    T& At(GridCoordinate coordinate) { return cells.at(Index(coordinate)); }
    const T& At(GridCoordinate coordinate) const { return cells.at(Index(coordinate)); }
    T* TryGet(GridCoordinate coordinate) { return InBounds(coordinate) ? &cells[Index(coordinate)] : nullptr; }
    const T* TryGet(GridCoordinate coordinate) const {
        return InBounds(coordinate) ? &cells[Index(coordinate)] : nullptr;
    }
    void Fill(const T& value) { std::fill(cells.begin(), cells.end(), value); }

private:
    [[nodiscard]] std::size_t Index(GridCoordinate coordinate) const {
        if (!InBounds(coordinate)) throw std::out_of_range("Grid coordinate is outside the grid");
        return static_cast<std::size_t>(coordinate.row) * static_cast<std::size_t>(columnCount) +
               static_cast<std::size_t>(coordinate.column);
    }
    int columnCount{};
    int rowCount{};
    std::vector<T> cells;
};

}  // namespace pipeframe
#endif

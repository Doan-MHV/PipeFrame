#ifndef PIPEFRAME_SPATIAL_UNIFORM_SPATIAL_INDEX_H
#define PIPEFRAME_SPATIAL_UNIFORM_SPATIAL_INDEX_H

#include <PipeFrame/Data/Grid2D.h>
#include <PipeFrame/Foundation/MathTypes.h>

#include <algorithm>
#include <cmath>
#include <span>
#include <unordered_map>
#include <vector>

namespace pipeframe {

template <typename Id, typename Hash = std::hash<Id>>
class UniformSpatialIndex {
public:
    struct Entry { Id id{}; Vector2f position{}; };

    void Initialize(Rectanglef newBounds, float newCellSize) {
        bounds = newBounds;
        bounds.size.x = std::max(0.001f, bounds.size.x);
        bounds.size.y = std::max(0.001f, bounds.size.y);
        cellSize = std::max(0.001f, newCellSize);
        inverseCellSize = 1.0f / cellSize;
        cells.Resize(std::max(1, static_cast<int>(std::ceil(bounds.size.x / cellSize))),
                     std::max(1, static_cast<int>(std::ceil(bounds.size.y / cellSize))));
        positions.clear();
    }

    void Rebuild(std::span<const Entry> entries) {
        cells.Fill({});
        positions.clear();
        positions.reserve(entries.size());
        for (const auto &entry : entries) Insert(entry.id, entry.position);
    }

    bool Insert(Id id, Vector2f position) {
        if (positions.contains(id) || !ContainsInclusive(position)) return false;
        positions.emplace(id, position);
        cells.At(CoordinateOf(position)).push_back(id);
        return true;
    }

    bool Update(Id id, Vector2f position) {
        const auto found = positions.find(id);
        if (found == positions.end() || !ContainsInclusive(position)) return false;
        const auto oldCoordinate = CoordinateOf(found->second);
        const auto newCoordinate = CoordinateOf(position);
        if (oldCoordinate != newCoordinate) {
            auto &oldCell = cells.At(oldCoordinate);
            std::erase(oldCell, id);
            cells.At(newCoordinate).push_back(id);
        }
        found->second = position;
        return true;
    }

    bool Remove(Id id) {
        const auto found = positions.find(id);
        if (found == positions.end()) return false;
        auto &cell = cells.At(CoordinateOf(found->second));
        std::erase(cell, id);
        positions.erase(found);
        return true;
    }

    [[nodiscard]] GridCellRange CellsOverlapping(Vector2f center, float radius) const {
        radius = std::max(0.0f, radius);
        return {CoordinateOf({center.x - radius, center.y - radius}),
                CoordinateOf({center.x + radius, center.y + radius})};
    }

    [[nodiscard]] std::span<const Id> GetIds(GridCoordinate coordinate) const {
        const auto *cell = cells.TryGet(coordinate);
        return cell ? std::span<const Id>(*cell) : std::span<const Id>{};
    }

    [[nodiscard]] std::vector<Id> QueryRadius(Vector2f center, float radius) const {
        std::vector<Id> result;
        radius = std::max(0.0f, radius);
        const auto range = CellsOverlapping(center, radius);
        const float radiusSquared = radius * radius;
        for (int row = range.minimum.row; row <= range.maximum.row; ++row) {
            for (int column = range.minimum.column; column <= range.maximum.column; ++column) {
                for (const Id id : GetIds({column, row})) {
                    const auto delta = positions.at(id) - center;
                    if (LengthSquared(delta) <= radiusSquared) result.push_back(id);
                }
            }
        }
        return result;
    }

    [[nodiscard]] GridCoordinate CoordinateOf(Vector2f position) const {
        return cells.Clamp({static_cast<int>(std::floor((position.x - bounds.position.x) * inverseCellSize)),
                            static_cast<int>(std::floor((position.y - bounds.position.y) * inverseCellSize))});
    }
    [[nodiscard]] int Columns() const { return cells.Columns(); }
    [[nodiscard]] int Rows() const { return cells.Rows(); }
    [[nodiscard]] std::size_t Size() const { return positions.size(); }

private:
    [[nodiscard]] bool ContainsInclusive(Vector2f point) const {
        return point.x >= bounds.position.x && point.y >= bounds.position.y &&
               point.x <= bounds.position.x + bounds.size.x && point.y <= bounds.position.y + bounds.size.y;
    }
    Rectanglef bounds{{0.0f, 0.0f}, {1.0f, 1.0f}};
    float cellSize{1.0f};
    float inverseCellSize{1.0f};
    Grid2D<std::vector<Id>> cells{1, 1};
    std::unordered_map<Id, Vector2f, Hash> positions;
};

} // namespace pipeframe
#endif

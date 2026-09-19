#include "World/Physics/CollisionGrid.h"
#include "World/Runtime/AntView.h"

#include <vector>

namespace ant_simulation {

bool CollisionGrid::CellRange::IsEmpty() const { return maximumColumn < minimumColumn || maximumRow < minimumRow; }

void CollisionGrid::Initialize(const pipeframe::Vector2f worldSize, const float cellSize) {
    index.Initialize({{0.0f, 0.0f}, worldSize}, cellSize);
}

void CollisionGrid::Rebuild(const std::span<const AntView> ants) {
    std::vector<pipeframe::UniformSpatialIndex<AntId>::Entry> entries;
    entries.reserve(ants.size());
    for (const AntView &ant : ants) {
        const auto position = ant.GetPosition();
        entries.push_back({ant.GetId(), {position.x, position.y}});
    }
    index.Rebuild(entries);
}

CollisionGrid::CellRange CollisionGrid::GetCellsOverlapping(const pipeframe::Vector2f center,
                                                            const float radius) const {
    const auto range = index.CellsOverlapping(center, radius);
    return {range.minimum.column, range.maximum.column, range.minimum.row, range.maximum.row};
}

std::span<const AntId> CollisionGrid::GetAntIds(const int column, const int row) const {
    return index.GetIds({column, row});
}

} // namespace ant_simulation

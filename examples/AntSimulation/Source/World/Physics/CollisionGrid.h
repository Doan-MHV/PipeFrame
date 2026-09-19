#ifndef ANT_COLLISION_GRID_H
#define ANT_COLLISION_GRID_H

#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>
#include <cstdint>
#include <span>

namespace ant_simulation {
class AntView;
using AntId = std::uint64_t;

class CollisionGrid {
  public:
    struct CellRange {
        int minimumColumn{0};
        int maximumColumn{-1};
        int minimumRow{0};
        int maximumRow{-1};

        [[nodiscard]]
        bool IsEmpty() const;
    };

    void Initialize(pipeframe::Vector2f worldSize, float cellSize);

    void Rebuild(std::span<const AntView> ants);

    [[nodiscard]]
    CellRange GetCellsOverlapping(pipeframe::Vector2f center, float radius) const;

    [[nodiscard]]
    std::span<const AntId> GetAntIds(int column, int row) const;

  private:
    pipeframe::UniformSpatialIndex<AntId> index;
};

} // namespace ant_simulation

#endif

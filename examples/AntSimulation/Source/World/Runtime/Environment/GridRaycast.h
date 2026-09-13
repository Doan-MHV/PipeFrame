#ifndef ANT_GRID_RAYCAST_H
#define ANT_GRID_RAYCAST_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace ant_simulation {

class AntEnvironment;
struct AntWorldCell;

struct GridRaycastResult {
    bool hit{false};
    float distance{0.0f};

    pipeframe::Vector2i cellPosition{
        0,
        0,
    };

    pipeframe::Vector2f normal{
        0.0f,
        0.0f,
    };

    const AntWorldCell *cell{nullptr};
};

class GridRaycast {
public:
    [[nodiscard]]
    static GridRaycastResult Cast(
        const AntEnvironment &environment,
        pipeframe::Vector2f start,
        pipeframe::Vector2f direction,
        float maximumDistance
    );
};

} // namespace ant_simulation

#endif

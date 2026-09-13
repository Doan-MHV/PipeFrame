#ifndef ANT_WALL_BUILDER_H
#define ANT_WALL_BUILDER_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace ant_simulation {

class AntEnvironment;

enum class WallType {
    Full,
    NorthWest,
    NorthEast,
    SouthWest,
    SouthEast,
};

class WallBuilder {
public:
    static void CreateBorderWalls(
        AntEnvironment &environment
    );

    static void RebuildSamplingCoefficients(
        AntEnvironment &environment
    );

    [[nodiscard]]
    static bool IsWallBorder(
        const AntEnvironment &environment,
        pipeframe::Vector2i position,
        int maximumDistance = 1
    );

    [[nodiscard]]
    static int GetDistanceToWall(
        const AntEnvironment &environment,
        pipeframe::Vector2i position,
        int maximumDistance
    );

    [[nodiscard]]
    static WallType GetWallType(
        const AntEnvironment &environment,
        pipeframe::Vector2i position
    );
};

} // namespace ant_simulation

#endif
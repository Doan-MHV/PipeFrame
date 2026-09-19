#include "World/Runtime/Environment/WallBuilder.h"

#include <algorithm>
#include <cmath>

#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

void WallBuilder::CreateBorderWalls(AntEnvironment &environment) {
    const int margin = AntEnvironment::BorderMargin;

    for (int y = 0; y < environment.GetHeight(); ++y) {
        for (int x = 0; x < environment.GetWidth(); ++x) {
            const bool borderCell = x < margin || y < margin || x >= environment.GetWidth() - margin ||
                                    y >= environment.GetHeight() - margin;

            if (!borderCell) {
                continue;
            }

            AntWorldCell *cell = environment.TryGetCell(x, y);

            if (cell != nullptr) {
                cell->wall = true;
            }
        }
    }
}

void WallBuilder::RebuildSamplingCoefficients(AntEnvironment &environment) {
    constexpr int MaximumDistance{2};

    const int maximumManhattanDistance = MaximumDistance * 2;

    const int margin = AntEnvironment::BorderMargin;

    for (int y = margin; y < environment.GetHeight() - margin; ++y) {
        for (int x = margin; x < environment.GetWidth() - margin; ++x) {
            AntWorldCell *cell = environment.TryGetCell(x, y);

            if (cell == nullptr) {
                continue;
            }

            if (cell->wall) {
                cell->markerSamplingCoefficient = 0.0f;

                continue;
            }

            const int distanceToWall = GetDistanceToWall(environment, {x, y}, MaximumDistance);

            cell->markerSamplingCoefficient =
                static_cast<float>(distanceToWall - 1) / static_cast<float>(maximumManhattanDistance - 1);

            cell->markerSamplingCoefficient = std::clamp(cell->markerSamplingCoefficient, 0.0f, 1.0f);
        }
    }
}

bool WallBuilder::IsWallBorder(const AntEnvironment &environment, const pipeframe::Vector2i position,
                               const int maximumDistance) {
    if (maximumDistance < 0) {
        return false;
    }

    const AntWorldCell *centerCell = environment.TryGetCell(position);

    if (centerCell == nullptr || !centerCell->wall) {
        return false;
    }

    for (int deltaX = -maximumDistance; deltaX <= maximumDistance; ++deltaX) {
        for (int deltaY = -maximumDistance; deltaY <= maximumDistance; ++deltaY) {
            const pipeframe::Vector2i neighborPosition{
                position.x + deltaX,
                position.y + deltaY,
            };

            const AntWorldCell *neighbor = environment.TryGetCell(neighborPosition);

            if (neighbor == nullptr) {
                continue;
            }

            if (!neighbor->wall) {
                return true;
            }
        }
    }

    return false;
}

int WallBuilder::GetDistanceToWall(const AntEnvironment &environment, const pipeframe::Vector2i position,
                                   const int maximumDistance) {
    if (maximumDistance <= 0) {
        return 0;
    }

    int distance = maximumDistance * 2;

    for (int deltaX = -maximumDistance; deltaX <= maximumDistance; ++deltaX) {
        for (int deltaY = -maximumDistance; deltaY <= maximumDistance; ++deltaY) {
            const pipeframe::Vector2i neighborPosition{
                position.x + deltaX,
                position.y + deltaY,
            };

            const AntWorldCell *neighbor = environment.TryGetCell(neighborPosition);

            if (neighbor == nullptr || !neighbor->wall) {
                continue;
            }

            const int manhattanDistance = std::abs(deltaX) + std::abs(deltaY);

            distance = std::min(distance, manhattanDistance);
        }
    }

    return distance;
}

WallType WallBuilder::GetWallType(const AntEnvironment &environment, const pipeframe::Vector2i position) {
    if (!environment.ContainsCell(position.x, position.y)) {
        return WallType::Full;
    }

    if (position.x == 0 || position.y == 0 || position.x == environment.GetWidth() - 1 ||
        position.y == environment.GetHeight() - 1) {
        return WallType::Full;
    }

    const auto IsWall = [&environment](const int x, const int y) {
        const AntWorldCell *cell = environment.TryGetCell(x, y);

        return cell != nullptr && cell->wall;
    };

    const bool left = IsWall(position.x - 1, position.y);

    const bool right = IsWall(position.x + 1, position.y);

    const bool top = IsWall(position.x, position.y - 1);

    const bool bottom = IsWall(position.x, position.y + 1);

    if (left && top && !right && !bottom) {
        return WallType::SouthEast;
    }

    if (right && top && !left && !bottom) {
        return WallType::SouthWest;
    }

    if (left && bottom && !right && !top) {
        return WallType::NorthEast;
    }

    if (right && bottom && !left && !top) {
        return WallType::NorthWest;
    }

    return WallType::Full;
}

} // namespace ant_simulation
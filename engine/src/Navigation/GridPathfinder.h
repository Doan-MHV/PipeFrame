#ifndef PIPEFRAME_GRIDPATHFINDER_H
#define PIPEFRAME_GRIDPATHFINDER_H

#include <vector>

#include <glm/glm.hpp>

#include "Components/MovementTypeComponent.h"

class TileMap;

struct GridPathOptions
{
    int maxNodes = 500;
    bool allowDiagonal = false;
    float clearanceRadius = 0.0f;
    const std::vector<bool>* blockedCells = nullptr;
};

struct GridPathResult
{
    bool found = false;
    std::vector<glm::vec2> points;
};

class GridPathfinder
{
public:
    static GridPathResult FindPath(
        const TileMap& tileMap,
        const glm::vec2& from,
        const glm::vec2& to,
        MovementType movementType,
        const GridPathOptions& options = {}
    );
};

#endif // PIPEFRAME_GRIDPATHFINDER_H

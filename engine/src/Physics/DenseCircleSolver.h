#ifndef PIPEFRAME_DENSECIRCLESOLVER_H
#define PIPEFRAME_DENSECIRCLESOLVER_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

struct DenseCircleSolverConfig
{
    float radius = 8.0f;
    float response = 0.15f;
    float cellSize = 16.0f;
    int iterations = 1;
};

class DenseCircleSolver
{
public:
    template <typename TAgent, typename TCanMove>
    static void Resolve(
        std::vector<TAgent>& agents,
        const DenseCircleSolverConfig& config,
        TCanMove&& canMove
    )
    {
        if (agents.size() < 2 || config.radius <= 0.0f || config.response <= 0.0f)
        {
            return;
        }

        const float cellSize = std::max(config.cellSize, config.radius * 2.0f);
        const int iterations = std::max(config.iterations, 1);

        for (int iteration = 0; iteration < iterations; iteration++)
        {
            std::unordered_map<long long, std::vector<std::size_t>> grid;
            grid.reserve(agents.size());

            for (std::size_t index = 0; index < agents.size(); index++)
            {
                const glm::ivec2 cell = GetCell(agents[index].position, cellSize);
                grid[HashCell(cell.x, cell.y)].push_back(index);
            }

            for (std::size_t index = 0; index < agents.size(); index++)
            {
                const glm::ivec2 cell = GetCell(agents[index].position, cellSize);
                for (int y = -1; y <= 1; y++)
                {
                    for (int x = -1; x <= 1; x++)
                    {
                        const auto found = grid.find(HashCell(cell.x + x, cell.y + y));
                        if (found == grid.end())
                        {
                            continue;
                        }

                        for (std::size_t otherIndex : found->second)
                        {
                            if (otherIndex <= index)
                            {
                                continue;
                            }

                            ResolvePair(agents[index], agents[otherIndex], config, canMove);
                        }
                    }
                }
            }
        }
    }

private:
    static glm::ivec2 GetCell(glm::vec2 position, float cellSize)
    {
        return {
            static_cast<int>(std::floor(position.x / cellSize)),
            static_cast<int>(std::floor(position.y / cellSize))
        };
    }

    static long long HashCell(int x, int y)
    {
        return (static_cast<long long>(x) << 32) ^ static_cast<unsigned int>(y);
    }

    template <typename TAgent, typename TCanMove>
    static void ResolvePair(
        TAgent& first,
        TAgent& second,
        const DenseCircleSolverConfig& config,
        TCanMove&& canMove
    )
    {
        glm::vec2 delta = first.position - second.position;
        float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        const float minDistance = config.radius * 2.0f;

        if (distance >= minDistance)
        {
            return;
        }

        if (distance <= 0.0001f)
        {
            delta = first.direction;
            distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (distance <= 0.0001f)
            {
                delta = glm::vec2(1.0f, 0.0f);
                distance = 1.0f;
            }
        }

        const glm::vec2 normal = delta / distance;
        const glm::vec2 push = normal * ((minDistance - distance) * 0.5f * config.response);
        TryMove(first, push, canMove);
        TryMove(second, -push, canMove);
    }

    template <typename TAgent, typename TCanMove>
    static void TryMove(TAgent& agent, glm::vec2 delta, TCanMove&& canMove)
    {
        const glm::vec2 target = agent.position + delta;
        if (canMove(agent.position, target))
        {
            agent.position = target;
            return;
        }

        const glm::vec2 xOnly(delta.x, 0.0f);
        if (std::abs(xOnly.x) > 0.0001f && canMove(agent.position, agent.position + xOnly))
        {
            agent.position += xOnly;
        }

        const glm::vec2 yOnly(0.0f, delta.y);
        if (std::abs(yOnly.y) > 0.0001f && canMove(agent.position, agent.position + yOnly))
        {
            agent.position += yOnly;
        }
    }
};

#endif // PIPEFRAME_DENSECIRCLESOLVER_H

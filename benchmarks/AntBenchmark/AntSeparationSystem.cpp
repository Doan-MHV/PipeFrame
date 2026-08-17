#include "AntSeparationSystem.h"

#include <algorithm>
#include <cmath>

AntSeparationStats AntSeparationSystem::Update(AntPopulation &population, const AntSpatialGrid &interactionGrid,
                                               float behaviorDeltaTime, std::size_t sliceIndex,
                                               std::size_t sliceCount) const {
    std::vector<Ant> &ants = population.GetAnts();

    AntSeparationStats stats;

    if (ants.empty() || sliceCount == 0) {
        return stats;
    }

    sliceIndex %= sliceCount;

    const float separationRadiusSquared = SeparationRadius * SeparationRadius;

    const float maximumSpeedSquared = MaximumSpeed * MaximumSpeed;

    for (std::size_t antIndex = sliceIndex; antIndex < ants.size(); antIndex += sliceCount) {
        ++stats.processedAntCount;

        Ant &ant = ants[antIndex];

        const sf::Vector2f position = ant.position;

        const sf::FloatRect queryArea{{position.x - SeparationRadius, position.y - SeparationRadius},
                                      {SeparationRadius * 2.0f, SeparationRadius * 2.0f}};

        const AntSpatialGrid::CellRange cellRange = interactionGrid.GetCellsOverlapping(queryArea);

        if (cellRange.IsEmpty()) {
            continue;
        }

        sf::Vector2f separation{0.0f, 0.0f};
        std::size_t neighborCount = 0;

        for (int row = cellRange.minimumRow; row <= cellRange.maximumRow; ++row) {
            for (int column = cellRange.minimumColumn; column <= cellRange.maximumColumn; ++column) {
                const std::span<const std::uint32_t> indices = interactionGrid.GetAgentIndices(column, row);

                stats.candidateCheckCount += indices.size();

                for (const std::uint32_t neighborIndex : indices) {
                    if (neighborIndex == antIndex) {
                        continue;
                    }

                    const sf::Vector2f difference = position - ants[neighborIndex].position;

                    const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

                    if (distanceSquared <= MinimumDistanceSquared || distanceSquared > separationRadiusSquared) {
                        continue;
                    }

                    const float safeDistanceSquared = std::max(distanceSquared, MinimumDistanceSquared);

                    separation += difference / safeDistanceSquared;

                    ++neighborCount;
                    ++stats.neighborInteractionCount;
                }
            }
        }

        if (neighborCount == 0) {
            continue;
        }

        separation /= static_cast<float>(neighborCount);

        ant.velocity += separation * SeparationStrength * behaviorDeltaTime;

        const float speedSquared = ant.velocity.x * ant.velocity.x + ant.velocity.y * ant.velocity.y;

        if (speedSquared > maximumSpeedSquared) {
            const float speed = std::sqrt(speedSquared);

            ant.velocity *= MaximumSpeed / speed;
        }
    }

    return stats;
}
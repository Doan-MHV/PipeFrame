#ifndef PIPEFRAME_ANT_SEPARATION_SYSTEM_H
#define PIPEFRAME_ANT_SEPARATION_SYSTEM_H

#include <cstddef>
#include <cstdint>

#include "AntPopulation.h"
#include "AntSpatialGrid.h"

struct AntSeparationStats {
    std::size_t processedAntCount = 0;

    std::uint64_t candidateCheckCount = 0;
    std::uint64_t neighborInteractionCount = 0;
};

class AntSeparationSystem {
  public:
    AntSeparationStats Update(AntPopulation &population, const AntSpatialGrid &interactionGrid, float behaviorDeltaTime,
                              std::size_t sliceIndex, std::size_t sliceCount) const;

  private:
    static constexpr float SeparationRadius = 2.0f;
    static constexpr float SeparationStrength = 18.0f;
    static constexpr float MaximumSpeed = 170.0f;
    static constexpr float MinimumDistanceSquared = 0.0001f;
};

#endif
#ifndef SAILBOAT_CONFIGURATION_H
#define SAILBOAT_CONFIGURATION_H

#include <cstddef>
#include <cstdint>
#include <string>

#include <PipeFrame/Foundation/MathTypes.h>

namespace sailboat_simulation {

struct SailBoatConfiguration final {
    std::uint32_t populationSize{1000};
    float maximumIterationTime{900.0f};
    float eliteRatio{0.2f};

    float newNodeProbability{0.1f};
    float newConnectionProbability{0.8f};
    float newValueProbability{0.1f};
    float weightRange{2.0f};
    float smallWeightRange{0.01f};

    std::uint32_t mutationCount{4};
    std::uint32_t maximumHiddenNodes{100};
    std::uint32_t seedOffset{1};
    std::uint32_t bestSavePeriod{1};

    std::size_t maximumWaypointCount{50};
    float waypointRadius{10.0f};
    pipeframe::Vector2f worldSize{1600.0f, 1600.0f};

    float simulationSpeedUp{10.0f};
    float angularSpeedDegrees{10.0f};
    pipeframe::Vector2f wind{1.0f, 0.0f};
    bool asyncTraining{false};
    bool waterAnimation{true};
    bool drawBestOnly{false};
    bool drawGhostBoats{true};
    bool highlightBest{true};
    bool drawBestTrajectory{true};
    bool drawWaypointLabels{true};
    bool showTargetGuide{true};
    bool audioEnabled{true};
    float audioVolume{30.0f};

    [[nodiscard]] bool Validate(std::string &errorMessage) const;
};

} // namespace sailboat_simulation

#endif

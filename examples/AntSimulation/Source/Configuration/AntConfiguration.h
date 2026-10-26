#ifndef ANT_CONFIGURATION_H
#define ANT_CONFIGURATION_H

#include <cstdint>
#include <string>

#include <PipeFrame/Foundation/MathTypes.h>

namespace ant_simulation {

struct AntConfiguration {
    static constexpr float Pi = 3.14159265358979323846f;

    static constexpr float DegreesToRadians(const float degrees) { return degrees * Pi / 180.0f; }

    // World
    pipeframe::Vector2i worldSize{
        384,
        216,
    };

    std::string mapFilename;

    // Markers
    float markerDecayRate{0.035f};
    float markerMaxIntensity{1'000.0f};
    float markerIntensityThreshold{0.0001f};
    float markerSamplingDegradation{0.1f};

    // Ants
    float antMaxEnergy{600.0f};
    float antRefillEnergyRatio{0.5f};
    float antSpeed{2.0f};

    float antCost{8.0f};
    float antSoldierCost{24.0f};

    float antFieldOfView{DegreesToRadians(135.0f)};

    float antExploreFieldOfView{DegreesToRadians(90.0f)};

    float antMarkerDistance{3.0f};

    std::uint32_t antFollowerSampleCount{64};
    std::uint32_t antExplorerSampleCount{8};

    float antSamplingDistanceMinimum{1.5f};
    float antSamplingDistanceMaximum{8.0f};

    // One worker is the deterministic recording mode used by AntPezza.
    // Larger values parallelize only independent per-ant state advancement;
    // shared physics, environment behavior, and RNG remain ordered.
    std::uint32_t antUpdateWorkerCount{1};

    // ColonyView
    pipeframe::Vector2f colonyPosition{
        40.0f,
        40.0f,
    };

    float colonyRadius{4.0f};
    std::uint32_t colonyInitialAntCount{1'000};
    float explorerProbability{0.1f};

    // Colors
    pipeframe::Color toHomeAntColor{
        255,
        209,
        102,
    };

    pipeframe::Color toFoodAntColor{
        239,
        71,
        111,
    };

    pipeframe::Color defaultAntColor{
        231,
        111,
        81,
    };

    pipeframe::Color toHomeMarkerColor{
        239,
        71,
        111,
    };

    pipeframe::Color toFoodMarkerColor{
        255,
        209,
        102,
    };

    pipeframe::Color foodColor{
        255,
        188,
        66,
    };

    // Rendering
    float detailZoomThreshold{15.0f};
    float farAntScaleBoost{0.5f};
    bool dynamicAntColor{false};

    [[nodiscard]]
    pipeframe::Vector2f GetWorldSizeFloat() const;

    [[nodiscard]]
    float GetMarkerMaximumIntensityInverse() const;

    [[nodiscard]]
    float GetAntMarkerInterval() const;

    [[nodiscard]]
    bool Validate(std::string &errorMessage) const;
};

} // namespace ant_simulation

#endif

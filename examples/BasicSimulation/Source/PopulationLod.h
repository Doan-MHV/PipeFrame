#ifndef BASIC_SIMULATION_POPULATION_LOD_H
#define BASIC_SIMULATION_POPULATION_LOD_H

#include <cmath>

namespace basic_simulation {

enum class PopulationRenderMode {
    Points,
    Quads,
};

struct PopulationLodThresholds final {
    float enterQuadsZoom = 0.30f;
    float enterPointsZoom = 0.45f;
};

inline PopulationRenderMode SelectPopulationRenderMode(
    const PopulationRenderMode currentMode,
    const float cameraZoom,
    const PopulationLodThresholds thresholds = {}) {

    if (!std::isfinite(cameraZoom) ||
        !std::isfinite(thresholds.enterQuadsZoom) ||
        !std::isfinite(thresholds.enterPointsZoom) ||
        thresholds.enterQuadsZoom < 0.0f ||
        thresholds.enterPointsZoom <=
            thresholds.enterQuadsZoom) {

        return currentMode;
            }

    if (currentMode ==
            PopulationRenderMode::Points &&
        cameraZoom <=
            thresholds.enterQuadsZoom) {

        return PopulationRenderMode::Quads;
            }

    if (currentMode ==
            PopulationRenderMode::Quads &&
        cameraZoom >=
            thresholds.enterPointsZoom) {

        return PopulationRenderMode::Points;
            }

    return currentMode;
}

} // namespace basic_simulation

#endif
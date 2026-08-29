#ifndef PIPEFRAME_RUNTIME_POPULATION_LOD_H
#define PIPEFRAME_RUNTIME_POPULATION_LOD_H

#include "RuntimePopulationPointRenderer.h"

#include <cmath>

namespace pipeframe::runtime {

struct RuntimePopulationLodThresholds final {
    float enterQuadsZoom = 0.30f;
    float enterPointsZoom = 0.45f;
};

inline RuntimePopulationRenderMode
SelectRuntimePopulationRenderMode(const RuntimePopulationRenderMode currentMode, const float cameraZoom,
                                  const RuntimePopulationLodThresholds thresholds = {}) {

    if (!std::isfinite(cameraZoom) || !std::isfinite(thresholds.enterQuadsZoom) ||
        !std::isfinite(thresholds.enterPointsZoom) || thresholds.enterQuadsZoom < 0.0f ||
        thresholds.enterPointsZoom <= thresholds.enterQuadsZoom) {
        return currentMode;
    }

    if (currentMode == RuntimePopulationRenderMode::Points && cameraZoom <= thresholds.enterQuadsZoom) {
        return RuntimePopulationRenderMode::Quads;
    }

    if (currentMode == RuntimePopulationRenderMode::Quads && cameraZoom >= thresholds.enterPointsZoom) {
        return RuntimePopulationRenderMode::Points;
    }

    return currentMode;
}

} // namespace pipeframe::runtime

#endif
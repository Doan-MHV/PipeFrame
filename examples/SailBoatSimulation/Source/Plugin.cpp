#include "Runtime/SailBoatSimulationRuntime.h"

extern "C" {

PIPEFRAME_PROJECT_RUNTIME_EXPORT
pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime() {
    return new sailboat_simulation::SailBoatSimulationRuntime();
}

PIPEFRAME_PROJECT_RUNTIME_EXPORT
void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime) {
    delete runtime;
}

} // extern "C"

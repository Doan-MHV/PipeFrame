#include "BasicSimulationRuntime.h"

extern "C" {

PIPEFRAME_PROJECT_RUNTIME_EXPORT
pipeframe::ProjectRuntime *
PipeFrameCreateProjectRuntime() {

    return new basic_simulation::
        BasicSimulationRuntime();
}

PIPEFRAME_PROJECT_RUNTIME_EXPORT
void PipeFrameDestroyProjectRuntime(
    pipeframe::ProjectRuntime *runtime) {

    delete runtime;
}

} // extern "C"
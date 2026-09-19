#include "Runtime/AntSimulationRuntime.h"

extern "C" {

PIPEFRAME_PROJECT_RUNTIME_EXPORT
pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime() { return new ant_simulation::AntSimulationRuntime(); }

PIPEFRAME_PROJECT_RUNTIME_EXPORT
void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime) { delete runtime; }

} // extern "C"

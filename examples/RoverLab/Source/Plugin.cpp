#include "Integration/RoverLabAdapter.h"
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime(){return new RoverLab::RoverLabAdapter;}
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime){delete runtime;}

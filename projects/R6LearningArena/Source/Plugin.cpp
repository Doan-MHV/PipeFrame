#include "Runtime/R6LearningArenaRuntime.h"
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime(){return new R6LearningArena::Runtime;}
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime){delete runtime;}

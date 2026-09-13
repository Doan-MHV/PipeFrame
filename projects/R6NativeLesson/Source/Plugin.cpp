#include "Runtime/R6NativeLessonRuntime.h"
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime(){return new R6NativeLesson::Runtime;}
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime){delete runtime;}

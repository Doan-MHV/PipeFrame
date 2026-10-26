#pragma once
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include "GeneratedRegistration.h"
namespace R6NativeLesson { class Runtime final : public pipeframe::SceneProjectRuntime { public: Runtime():SceneProjectRuntime("R6NativeLesson"){pipeframe_generated::Register(*this);} }; }

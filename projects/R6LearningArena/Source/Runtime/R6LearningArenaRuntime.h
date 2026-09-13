#pragma once
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include "GeneratedRegistration.h"
namespace R6LearningArena { class Runtime final : public pipeframe::SceneProjectRuntime { public: Runtime():SceneProjectRuntime("R6LearningArena"){pipeframe_generated::Register(*this);} }; }

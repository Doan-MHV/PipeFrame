#pragma once

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/Event.h>

#include <string>

namespace RoverLab {

// The detector reports contact only. Hidden marker coordinates never leave it.
struct RoverGoalEvent final : pipeframe::Event {
  pipeframe::SceneObject rover;
  pipeframe::SceneObject marker;
  std::string tag;
};

}  // namespace RoverLab

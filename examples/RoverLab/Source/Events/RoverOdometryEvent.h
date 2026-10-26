#pragma once

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/Event.h>

namespace RoverLab {

struct RoverOdometryEvent final : pipeframe::Event {
    pipeframe::SceneObject rover;

    // Relative to the starting pose; metres and radians.
    double x{};
    double y{};
    double heading{};

    // Accumulated centre-path distance, in metres.
    double distance{};
};

} // namespace RoverLab
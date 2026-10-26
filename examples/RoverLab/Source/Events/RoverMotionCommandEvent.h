#pragma once

#include <PipeFrame/Events/Event.h>
#include <PipeFrame/Project/ProjectTypes.h>

namespace RoverLab {

struct RoverMotionCommandEvent final : pipeframe::Event {
    pipeframe::SceneObjectId roverId{};

    // Metres per second: positive forward, negative reverse.
    double forwardSpeed{};

    // Radians per second.
    // Positive means clockwise in our screen-coordinate convention.
    double turnRate{};
};

}  // namespace RoverLab

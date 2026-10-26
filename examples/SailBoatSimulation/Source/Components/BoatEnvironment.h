#ifndef SAILBOAT_BOAT_ENVIRONMENT_H
#define SAILBOAT_BOAT_ENVIRONMENT_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace sailboat_simulation {

struct BoatEnvironment final {
    pipeframe::Vector2f size{1600.0f, 1600.0f};
    pipeframe::Vector2f wind{1.0f, 0.0f};
};

} // namespace sailboat_simulation

#endif

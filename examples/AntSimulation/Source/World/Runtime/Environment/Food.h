#ifndef ANT_FOOD_H
#define ANT_FOOD_H

#include <PipeFrame/Foundation/MathTypes.h>

#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

struct Food {
    WorldEntityId id{InvalidWorldEntityId};

    pipeframe::Vector2f position{
        0.0f,
        0.0f,
    };
};

} // namespace ant_simulation

#endif
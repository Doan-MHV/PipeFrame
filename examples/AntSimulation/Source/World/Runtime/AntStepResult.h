#pragma once
#include <cstddef>
namespace ant_simulation {
struct AntSimulationStepResult {
    std::size_t livingAnts{0};
    std::size_t removedAnts{0};

    std::size_t antContacts{0};
    std::size_t wallConstraints{0};
    std::size_t enemyAlerts{0};

    std::size_t futureCollisions{0};

    float preparationTimeMs{0.0f};
    float avoidanceTimeMs{0.0f};
    float physicsTimeMs{0.0f};
    float behaviorTimeMs{0.0f};
    float cleanupTimeMs{0.0f};
};

}

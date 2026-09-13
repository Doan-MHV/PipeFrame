#ifndef SAILBOAT_BOAT_UPDATER_H
#define SAILBOAT_BOAT_UPDATER_H

#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include <PipeFrame/Simulation/System.h>

namespace sailboat_simulation {

struct BoatUpdateCommand final {
    float rudder{0.0f};
    float angularSpeedDegrees{0.0f};
    bool recordTrajectory{true};
};

class BoatMovementSystem final
    : public pipeframe::EntityUpdateSystem<Boat, BoatEnvironment, BoatUpdateCommand> {
public:
    [[nodiscard]] std::string_view GetSystemId() const override { return "sailboat.movement"; }
    [[nodiscard]] static float KnotsToMetersPerSecond(float knots);
    [[nodiscard]] static float GetWindRelativeAngleDegrees(const Boat &boat,
                                                           const BoatEnvironment &environment);
    [[nodiscard]] static float GetSpeed(const Boat &boat, const BoatEnvironment &environment);

    void Update(Boat &boat, const BoatEnvironment &environment,
                const BoatUpdateCommand &command, float deltaTime) const override;
};

} // namespace sailboat_simulation

#endif

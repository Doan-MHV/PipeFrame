#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include "Systems/BoatMovementSystem.h"

#include <type_traits>

static_assert(std::is_base_of_v<
              pipeframe::EntityUpdateSystem<sailboat_simulation::Boat,
                                             sailboat_simulation::BoatEnvironment,
                                             sailboat_simulation::BoatUpdateCommand>,
              sailboat_simulation::BoatMovementSystem>);
#include "Components/PolarTable.h"

namespace {

bool NearlyEqual(const float left, const float right, const float tolerance = 0.001f) {
    return std::abs(left - right) <= tolerance;
}

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool TestPezzaPolarTable() {
    using namespace sailboat_simulation;
    bool passed = true;
    passed &= Check(PolarTable::GetSamples().size() == 181,
                    "The Pezza polar must contain one sample for every angle from 0 to 180 degrees.");
    passed &= Check(NearlyEqual(PolarTable::GetSpeedKnots(30.0f), 0.0f),
                    "The Pezza no-go zone should stop the boat through 30 degrees.");
    passed &= Check(NearlyEqual(PolarTable::GetSpeedKnots(31.0f), 0.8f),
                    "The first moving polar sample should match Pezza.");
    passed &= Check(NearlyEqual(PolarTable::GetSpeedKnots(120.0f), 6.3f),
                    "The broad-reach polar sample should match Pezza.");
    passed &= Check(NearlyEqual(PolarTable::GetSpeedKnots(30.5f), 0.4f),
                    "Fractional wind angles should interpolate adjacent samples.");
    passed &= Check(NearlyEqual(PolarTable::GetSpeedKnots(999.0f), 4.8f),
                    "Polar lookup should safely clamp angles beyond 180 degrees.");
    return passed;
}

bool TestWindRelativeMovement() {
    using namespace sailboat_simulation;
    const BoatEnvironment environment{{100.0f, 100.0f}, {1.0f, 0.0f}};

    Boat headToWind{{50.0f, 50.0f}, std::numbers::pi_v<float>};
    BoatMovementSystem updater;
    updater.Update(headToWind, environment, {0.0f, 10.0f, true}, 1.0f);

    Boat reaching{{50.0f, 50.0f}, std::numbers::pi_v<float> * 0.5f};
    updater.Update(reaching, environment, {0.0f, 10.0f, true}, 1.0f);

    bool passed = true;
    passed &= Check(NearlyEqual(headToWind.GetSpeed(), 0.0f),
                    "A boat pointed into the incoming wind should remain in the no-go zone.");
    passed &= Check(reaching.GetSpeed() > 3.0f,
                    "A boat sailing across the wind should use Pezza's polar speed.");
    passed &= Check(reaching.GetPosition().y > 53.0f,
                    "Boat movement should advance along its heading.");
    passed &= Check(NearlyEqual(reaching.GetDistanceTravelled(), reaching.GetSpeed()),
                    "Distance travelled should accumulate physical movement.");
    return passed;
}

bool TestRudderAndTrajectory() {
    using namespace sailboat_simulation;
    const BoatEnvironment environment{{100.0f, 100.0f}, {1.0f, 0.0f}};
    Boat boat{{50.0f, 50.0f}, 0.0f};

    BoatMovementSystem updater;
    updater.Update(boat, environment, {2.0f, 10.0f, true}, 0.5f);
    updater.Update(boat, environment, {1.0f, 10.0f, true}, 0.5f);

    bool passed = true;
    passed &= Check(NearlyEqual(boat.GetRudderCommand(), 1.0f),
                    "Rudder commands should be clamped to the neural output range.");
    passed &= Check(NearlyEqual(boat.GetAngleRadians(), 10.0f * std::numbers::pi_v<float> / 180.0f),
                    "Rudder integration should use the configured angular speed.");
    passed &= Check(boat.GetTrajectory().size() == 3,
                    "Reset plus two updates should produce three trajectory samples.");
    passed &= Check(NearlyEqual(boat.GetTrajectory().back().time, 1.0f),
                    "Trajectory samples should preserve simulation time.");
    return passed;
}

bool TestBoundaryAndDeterministicReset() {
    using namespace sailboat_simulation;
    const BoatEnvironment environment{{10.0f, 10.0f}, {1.0f, 0.0f}};
    Boat boat{{9.5f, 5.0f}, 0.0f};

    BoatMovementSystem updater;
    updater.Update(boat, environment, {0.0f, 10.0f, true}, 10.0f);
    bool passed = true;
    passed &= Check(boat.HasCrashed(), "A boat leaving the authored world should crash.");
    passed &= Check(NearlyEqual(boat.GetPosition().x, 10.0f),
                    "A crashed boat should remain clamped to the visible world boundary.");

    boat.Reset({5.0f, 5.0f}, std::numbers::pi_v<float> * 0.5f);
    updater.Update(boat, environment, {-0.25f, 20.0f, true}, 0.5f);
    const pipeframe::Vector2f firstPosition = boat.GetPosition();
    const float firstAngle = boat.GetAngleRadians();

    boat.Reset({5.0f, 5.0f}, std::numbers::pi_v<float> * 0.5f);
    updater.Update(boat, environment, {-0.25f, 20.0f, true}, 0.5f);
    passed &= Check(NearlyEqual(boat.GetPosition().x, firstPosition.x) &&
                        NearlyEqual(boat.GetPosition().y, firstPosition.y) &&
                        NearlyEqual(boat.GetAngleRadians(), firstAngle),
                    "Resetting and replaying identical inputs must be deterministic.");
    passed &= Check(!boat.HasCrashed() && boat.GetTrajectory().size() == 2,
                    "Reset should clear crash and trajectory state before replay.");
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestPezzaPolarTable();
    passed &= TestWindRelativeMovement();
    passed &= TestRudderAndTrajectory();
    passed &= TestBoundaryAndDeterministicReset();

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat movement tests passed.\n";
    return 0;
}

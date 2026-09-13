#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include "Training/SailBoatRaceTask.h"
#include "World/RaceCourse.h"

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

sailboat_simulation::RaceCourse CreateCourse() {
    using namespace sailboat_simulation;
    RaceCourse course;
    course.SetWorldSize({100.0f, 100.0f});
    course.SetStart({{5.0f, 50.0f}, {15.0f, 50.0f}});
    course.AddWaypoint({{{20.0f, 45.0f}, {20.0f, 55.0f}}, 2.0f, 1});
    course.SetFinish({{30.0f, 45.0f}, {30.0f, 55.0f}});
    return course;
}

bool TestFourPezzaInputs() {
    using namespace sailboat_simulation;
    const RaceCourse course = CreateCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    const Boat boat{{10.0f, 50.0f}, 0.0f};
    const SailBoatRaceTask task;
    const auto inputs = task.BuildNeuralInputs(boat, environment, course);

    bool passed = true;
    passed &= Check(inputs.size() == 4, "The SailBoat network contract must expose four inputs.");
    passed &= Check(NearlyEqual(inputs[0], 0.894427f) &&
                        NearlyEqual(inputs[1], -0.447214f),
                    "The first two inputs should point to Pezza's first mark endpoint.");
    passed &= Check(NearlyEqual(inputs[2], 0.0f) && NearlyEqual(inputs[3], 1.0f),
                    "The final two inputs should describe wind direction in boat space.");
    return passed;
}

bool TestOrderedProgressAndFinish() {
    using namespace sailboat_simulation;
    const RaceCourse course = CreateCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    Boat boat{{19.5f, 45.0f}, 0.0f};
    SailBoatRaceTask task;

    Boat onSegment{{20.0f, 50.0f}, 0.0f};
    SailBoatRaceTask endpointTask;
    endpointTask.Update(onSegment, environment, course, 0.0f, 10.0f, 0.01f);
    bool passed = true;
    passed &= Check(endpointTask.GetTargetIndex() == 0,
                    "Pezza targets the mark endpoint, not the closest point on its line.");

    task.Update(boat, environment, course, 0.0f, 10.0f, 0.01f);
    passed &= Check(task.GetTargetIndex() == 1,
                    "Reaching the first ordered waypoint should advance to the finish.");
    passed &= Check(!task.HasFinished(), "A waypoint should not finish the race.");

    boat.Reset({29.0f, 45.0f}, 0.0f);
    task.Update(boat, environment, course, 0.0f, 10.0f, 0.01f);
    passed &= Check(task.HasFinished() && boat.HasFinished(),
                    "Reaching the finish gate should complete both task and boat.");
    passed &= Check(task.GetRaceTime() > 0.0f && task.GetScore() > 1'000'000.0f,
                    "A completed race should retain time and receive Pezza's finish bonus.");
    return passed;
}

bool TestDistanceTimeAndReset() {
    using namespace sailboat_simulation;
    const RaceCourse course = CreateCourse();
    const BoatEnvironment environment{{100.0f, 100.0f}, {0.0f, 1.0f}};
    Boat boat{{5.0f, 40.0f}, 0.0f};
    SailBoatRaceTask task;

    const float distanceBefore = std::sqrt(250.0f);
    task.Update(boat, environment, course, 0.25f, 20.0f, 0.5f);
    bool passed = true;
    passed &= Check(NearlyEqual(task.GetWorldTime(), 0.5f),
                    "Task time should advance by the fixed simulation delta.");
    passed &= Check(task.GetRaceDistance() > 0.0f && task.GetScore() > 0.0f,
                    "Active sailing should accumulate distance and score.");
    passed &= Check(NearlyEqual(task.GetScore(), 0.5f / (1.0f + distanceBefore)),
                    "Progress score should match Pezza's target-distance/time formula exactly.");

    task.Reset();
    passed &= Check(task.GetTargetIndex() == 0 && NearlyEqual(task.GetWorldTime(), 0.0f) &&
                        NearlyEqual(task.GetRaceDistance(), 0.0f) &&
                        NearlyEqual(task.GetRaceTime(), -1.0f) && !task.HasFinished(),
                    "Task reset should restore every race-progress field deterministically.");
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestFourPezzaInputs();
    passed &= TestOrderedProgressAndFinish();
    passed &= TestDistanceTimeAndReset();

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat race task tests passed.\n";
    return 0;
}

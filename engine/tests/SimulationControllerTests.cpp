#include <PipeFrame/Simulation/SimulationController.h>

#include <iostream>
#include <string>

namespace {

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

bool TestSpeedModes() {
    SimulationController controller;
    bool passed = true;

    passed &= Check(controller.GetSpeed() == SimulationSpeed::Realtime, "Simulation should start at 1X.");
    passed &= Check(controller.GetTimeScale() == 1.0f, "1X should use realtime accumulation.");

    controller.SetSpeed(SimulationSpeed::Double);
    passed &= Check(controller.GetTimeScale() == 2.0f, "2X should accumulate time twice as quickly.");

    controller.SetSpeed(SimulationSpeed::Quadruple);
    passed &= Check(controller.GetTimeScale() == 4.0f, "4X should accumulate time four times as quickly.");

    controller.SetSpeed(SimulationSpeed::Maximum);
    passed &= Check(controller.GetTimeScale() == 1.0f, "MAX should use the application's capacity schedule.");
    passed &= Check(controller.IsFullSpeed(), "MAX should preserve the full-speed compatibility state.");

    return passed;
}

bool TestSpeedCycle() {
    SimulationController controller;
    bool passed = true;

    controller.CycleSpeed();
    passed &= Check(controller.GetSpeed() == SimulationSpeed::Double, "1X should cycle to 2X.");
    controller.CycleSpeed();
    passed &= Check(controller.GetSpeed() == SimulationSpeed::Quadruple, "2X should cycle to 4X.");
    controller.CycleSpeed();
    passed &= Check(controller.GetSpeed() == SimulationSpeed::Maximum, "4X should cycle to MAX.");
    controller.CycleSpeed();
    passed &= Check(controller.GetSpeed() == SimulationSpeed::Realtime, "MAX should cycle to 1X.");

    return passed;
}

bool TestPauseAndSingleStep() {
    SimulationController controller;
    controller.Pause();

    bool passed = true;
    passed &= Check(!controller.ConsumeTick(), "Paused simulation must not consume a tick.");

    controller.RequestSingleStep();
    passed &= Check(controller.ConsumeTick(), "Single step should consume exactly one tick.");
    passed &= Check(!controller.ConsumeTick(), "Single step must not continue ticking.");
    passed &= Check(controller.GetTickCount() == 1, "Single step should advance the tick count once.");

    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestSpeedModes();
    passed &= TestSpeedCycle();
    passed &= TestPauseAndSingleStep();

    if (!passed) {
        return 1;
    }

    std::cout << "All simulation controller tests passed.\n";
    return 0;
}

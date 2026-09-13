#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "Configuration/SailBoatConfiguration.h"
#include "Components/BoatEnvironment.h"
#include "Training/SailBoatPopulationTrainer.h"
#include "World/RaceCourse.h"

namespace {

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

sailboat_simulation::RaceCourse CreateLongCourse() {
    sailboat_simulation::RaceCourse course;
    course.SetWorldSize({10000.0f, 10000.0f});
    course.SetStart({{500.0f, 500.0f}, {550.0f, 500.0f}});
    course.SetFinish({{9500.0f, 9400.0f}, {9500.0f, 9600.0f}});
    return course;
}

} // namespace

int main() {
    using namespace sailboat_simulation;

    constexpr std::uint32_t Population = 10000;
    constexpr std::size_t UpdateCount = 60;

    SailBoatConfiguration configuration;
    configuration.populationSize = Population;
    configuration.maximumIterationTime = 3600.0f;
    configuration.simulationSpeedUp = 1.0f;
    configuration.asyncTraining = false;
    configuration.worldSize = {10000.0f, 10000.0f};
    configuration.wind = {1.0f, 0.0f};

    bool passed = true;
    std::string error;
    SailBoatPopulationTrainer trainer;
    passed &= Check(trainer.Initialize(configuration, CreateLongCourse(),
                                       {{10000.0f, 10000.0f}, {1.0f, 0.0f}}, error),
                    "A 10,000-boat stress population should initialize. " + error);
    if (!passed) {
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t update = 0; update < UpdateCount; ++update) {
        trainer.Update(1.0f / 60.0f);
    }
    const float elapsedSeconds = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - start).count();
    const double agentUpdates = static_cast<double>(Population) * UpdateCount;
    const double updatesPerSecond = agentUpdates / std::max(0.000001f, elapsedSeconds);

    std::size_t trajectoryPoints = 0;
    for (const SailBoatAgent &agent : trainer.GetAgents()) {
        trajectoryPoints += agent.GetBoat().GetTrajectory().size();
    }

    passed &= Check(trainer.GetAgents().size() == Population,
                    "The stress run must retain the full population.");
    passed &= Check(std::isfinite(elapsedSeconds) && elapsedSeconds < 30.0f,
                    "600,000 boat updates must remain inside the broad regression budget.");
    passed &= Check(trajectoryPoints <= Population + UpdateCount,
                    "Trajectory storage must remain bounded to the leading boat.");

    std::cout << "SailBoat stress: " << Population << " boats x " << UpdateCount
              << " updates in " << elapsedSeconds << "s ("
              << static_cast<std::uint64_t>(updatesPerSecond) << " agent-updates/s)\n";
    return passed ? 0 : 1;
}

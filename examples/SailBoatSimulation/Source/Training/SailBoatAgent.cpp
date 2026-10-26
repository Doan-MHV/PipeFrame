#include "SailBoatAgent.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <PipeFrame/Learning/NetworkGenerator.h>

namespace sailboat_simulation {

SailBoatAgent::SailBoatAgent(const std::size_t newId) : id(newId) {}

bool SailBoatAgent::Compile(std::string *errorMessage) {
    auto compiled = pipeframe::learning::NetworkGenerator::Generate(genome, errorMessage);
    if (!compiled.has_value()) {
        return false;
    }
    network = std::move(*compiled);
    return true;
}

void SailBoatAgent::Reset(const RaceCourse &course) {
    const RaceSegment &start = course.GetStart();
    const float radians = start.GetAngleDegrees() * std::numbers::pi_v<float> / 180.0f;
    boat.Reset(start.GetFirstPoint(), radians);
    task.Reset();
}

void SailBoatAgent::Update(const BoatEnvironment &environment, const RaceCourse &course,
                           const float angularSpeedDegrees, const float deltaTime,
                           const bool recordTrajectory) {
    if (!IsActive()) {
        return;
    }

    const SailBoatRaceTask::NeuralInputs inputs = task.BuildNeuralInputs(boat, environment, course);
    float rudderCommand = 0.0f;
    if (network.Execute(inputs) && !network.GetOutputs().empty()) {
        rudderCommand = std::clamp(network.GetOutputs().front(), -1.0f, 1.0f);
    }
    task.Update(boat, environment, course, rudderCommand, angularSpeedDegrees, deltaTime,
                recordTrajectory);
}

std::size_t SailBoatAgent::GetId() const { return id; }
float SailBoatAgent::GetScore() const { return task.GetScore(); }
bool SailBoatAgent::IsActive() const { return !boat.HasCrashed() && !boat.HasFinished(); }
const Boat &SailBoatAgent::GetBoat() const { return boat; }
Boat &SailBoatAgent::GetBoat() { return boat; }
const SailBoatRaceTask &SailBoatAgent::GetTask() const { return task; }
SailBoatRaceTask &SailBoatAgent::GetTask() { return task; }
const pipeframe::learning::Genome &SailBoatAgent::GetGenome() const { return genome; }
pipeframe::learning::Genome &SailBoatAgent::GetGenome() { return genome; }
const pipeframe::learning::Network &SailBoatAgent::GetNetwork() const { return network; }

} // namespace sailboat_simulation

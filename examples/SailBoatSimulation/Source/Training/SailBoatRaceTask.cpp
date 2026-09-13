#include "SailBoatRaceTask.h"

#include <algorithm>
#include <cmath>

#include "Systems/BoatMovementSystem.h"

namespace sailboat_simulation {
namespace {

float Dot(const pipeframe::Vector2f left, const pipeframe::Vector2f right) {
    return left.x * right.x + left.y * right.y;
}

float Magnitude(const pipeframe::Vector2f value) {
    return std::sqrt(Dot(value, value));
}

pipeframe::Vector2f NormalizeOr(const pipeframe::Vector2f value, const pipeframe::Vector2f fallback) {
    const float length = Magnitude(value);
    return length > 0.0001f ? value / length : fallback;
}

} // namespace

void SailBoatRaceTask::Reset() {
    targetIndex = 0;
    score = 0.0f;
    worldTime = 0.0f;
    raceTime = -1.0f;
    raceDistance = 0.0f;
    timeOnCurrentTarget = 0.0f;
    finished = false;
}

SailBoatRaceTask::NeuralInputs
SailBoatRaceTask::BuildNeuralInputs(const Boat &boat, const BoatEnvironment &environment,
                                    const RaceCourse &course) const {
    const pipeframe::Vector2f direction = boat.GetDirection();
    const pipeframe::Vector2f normal{-direction.y, direction.x};
    const pipeframe::Vector2f toTarget = GetTargetPoint(boat, course) - boat.GetPosition();
    const pipeframe::Vector2f targetDirection = NormalizeOr(toTarget, direction);
    const pipeframe::Vector2f windDirection = NormalizeOr(environment.wind, {0.0f, 0.0f});

    return {
        Dot(targetDirection, direction),
        Dot(targetDirection, normal),
        Dot(windDirection, direction),
        Dot(windDirection, normal),
    };
}

void SailBoatRaceTask::Update(Boat &boat, const BoatEnvironment &environment,
                              const RaceCourse &course, const float rudderCommand,
                              const float angularSpeedDegrees, const float deltaTime,
                              const bool recordTrajectory) {
    if (finished || boat.HasCrashed() || boat.HasFinished() ||
        !std::isfinite(deltaTime) || deltaTime <= 0.0f || course.GetTargetCount() == 0) {
        return;
    }

    const float distanceBeforeUpdate =
        Magnitude(GetTargetPoint(boat, course) - boat.GetPosition());
    worldTime += deltaTime;

    BoatMovementSystem updater;
    updater.Update(boat, environment,
                   BoatUpdateCommand{rudderCommand, angularSpeedDegrees, recordTrajectory},
                   deltaTime);
    raceDistance = boat.GetDistanceTravelled();

    score += static_cast<float>(targetIndex + 1) * deltaTime /
             (1.0f + distanceBeforeUpdate + timeOnCurrentTarget);

    if (IsCurrentTargetReached(boat, course)) {
        ++targetIndex;
        timeOnCurrentTarget = 0.0f;
        score += 10.0f;

        if (targetIndex >= course.GetTargetCount()) {
            finished = true;
            raceTime = worldTime;
            score += 1'000'000.0f / std::max(raceTime, 0.0001f);
            boat.MarkFinished();
        }
    } else {
        timeOnCurrentTarget += deltaTime;
    }
}

std::size_t SailBoatRaceTask::GetTargetIndex() const { return targetIndex; }

pipeframe::Vector2f SailBoatRaceTask::GetTargetPoint(const Boat &boat, const RaceCourse &course) const {
    if (course.GetTargetCount() == 0) {
        return boat.GetPosition();
    }

    const std::size_t safeIndex = std::min(targetIndex, course.GetTargetCount() - 1);
    return course.GetTarget(safeIndex).GetFirstPoint();
}

float SailBoatRaceTask::GetScore() const { return score; }
float SailBoatRaceTask::GetWorldTime() const { return worldTime; }
float SailBoatRaceTask::GetRaceTime() const { return raceTime; }
float SailBoatRaceTask::GetRaceDistance() const { return raceDistance; }
float SailBoatRaceTask::GetTimeOnCurrentTarget() const { return timeOnCurrentTarget; }
bool SailBoatRaceTask::HasFinished() const { return finished; }

float SailBoatRaceTask::GetTargetRadius(const RaceCourse &course) const {
    if (targetIndex < course.GetWaypoints().size()) {
        return course.GetWaypoints()[targetIndex].radius;
    }
    return 10.0f;
}

bool SailBoatRaceTask::IsCurrentTargetReached(const Boat &boat, const RaceCourse &course) const {
    if (targetIndex >= course.GetTargetCount()) {
        return true;
    }

    const pipeframe::Vector2f difference =
        course.GetTarget(targetIndex).GetFirstPoint() - boat.GetPosition();
    return Magnitude(difference) <= GetTargetRadius(course);
}

} // namespace sailboat_simulation

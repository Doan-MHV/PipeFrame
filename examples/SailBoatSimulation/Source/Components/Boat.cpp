#include "Components/Boat.h"

#include <cmath>

namespace sailboat_simulation {

Boat::Boat(const pipeframe::Vector2f position, const float angleRadians) {
    Reset(position, angleRadians);
}

void Boat::Reset(const pipeframe::Vector2f newPosition, const float newAngleRadians) {
    position = newPosition;
    angleRadians = newAngleRadians;
    speed = 0.0f;
    elapsedTime = 0.0f;
    distanceTravelled = 0.0f;
    rudderCommand = 0.0f;
    crashed = false;
    finished = false;
    trajectory.clear();
    trajectory.push_back({position, GetDirection(), 0.0f});
}

pipeframe::Vector2f Boat::GetPosition() const { return position; }

pipeframe::Vector2f Boat::GetDirection() const {
    return {std::cos(angleRadians), std::sin(angleRadians)};
}

float Boat::GetAngleRadians() const { return angleRadians; }
float Boat::GetSpeed() const { return speed; }
float Boat::GetElapsedTime() const { return elapsedTime; }
float Boat::GetDistanceTravelled() const { return distanceTravelled; }
float Boat::GetRudderCommand() const { return rudderCommand; }
bool Boat::HasCrashed() const { return crashed; }
bool Boat::HasFinished() const { return finished; }
const std::vector<TrajectoryPoint> &Boat::GetTrajectory() const { return trajectory; }

void Boat::MarkFinished() {
    finished = true;
    speed = 0.0f;
    rudderCommand = 0.0f;
}

} // namespace sailboat_simulation

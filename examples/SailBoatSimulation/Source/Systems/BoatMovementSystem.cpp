#include "Systems/BoatMovementSystem.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Components/PolarTable.h"

namespace sailboat_simulation {
namespace {

float Dot(const pipeframe::Vector2f left, const pipeframe::Vector2f right) {
    return left.x * right.x + left.y * right.y;
}

float Cross(const pipeframe::Vector2f left, const pipeframe::Vector2f right) {
    return left.x * right.y - left.y * right.x;
}

float Magnitude(const pipeframe::Vector2f value) {
    return std::sqrt(Dot(value, value));
}

} // namespace

float BoatMovementSystem::KnotsToMetersPerSecond(const float knots) { return knots * 0.5144f; }

float BoatMovementSystem::GetWindRelativeAngleDegrees(const Boat &boat, const BoatEnvironment &environment) {
    const float windLength = Magnitude(environment.wind);
    if (windLength <= 0.0001f) {
        return 0.0f;
    }

    // Pezza's polar is indexed from the direction the wind comes from.
    const pipeframe::Vector2f incomingWind = -environment.wind / windLength;
    const pipeframe::Vector2f boatDirection = boat.GetDirection();
    const float angle = std::atan2(Cross(incomingWind, boatDirection),
                                   Dot(incomingWind, boatDirection));
    return angle * 180.0f / std::numbers::pi_v<float>;
}

float BoatMovementSystem::GetSpeed(const Boat &boat, const BoatEnvironment &environment) {
    if (Magnitude(environment.wind) <= 0.0001f) {
        return 0.0f;
    }
    return KnotsToMetersPerSecond(
        PolarTable::GetSpeedKnots(GetWindRelativeAngleDegrees(boat, environment)));
}

void BoatMovementSystem::Update(Boat &boat, const BoatEnvironment &environment,
                         const BoatUpdateCommand &command, const float deltaTime) const {
    if (boat.crashed || boat.finished || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
        return;
    }

    boat.rudderCommand = std::clamp(command.rudder, -1.0f, 1.0f);
    const float angularSpeedRadians =
        std::max(0.0f, command.angularSpeedDegrees) * std::numbers::pi_v<float> / 180.0f;
    boat.angleRadians += boat.rudderCommand * angularSpeedRadians * deltaTime;
    boat.speed = GetSpeed(boat, environment);

    const pipeframe::Vector2f previousPosition = boat.position;
    boat.position += boat.GetDirection() * (boat.speed * deltaTime);
    boat.elapsedTime += deltaTime;
    boat.distanceTravelled += Magnitude(boat.position - previousPosition);

    const bool outside = boat.position.x < 0.0f || boat.position.y < 0.0f ||
                         boat.position.x > environment.size.x || boat.position.y > environment.size.y;
    if (outside) {
        boat.position.x = std::clamp(boat.position.x, 0.0f, environment.size.x);
        boat.position.y = std::clamp(boat.position.y, 0.0f, environment.size.y);
        boat.speed = 0.0f;
        boat.crashed = true;
    }

    if (command.recordTrajectory) {
        boat.trajectory.push_back({boat.position, boat.GetDirection(), boat.elapsedTime});
    }
}

} // namespace sailboat_simulation

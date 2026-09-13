#include "RaceSegment.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace sailboat_simulation {
namespace {

float Dot(const pipeframe::Vector2f left, const pipeframe::Vector2f right) {
    return left.x * right.x + left.y * right.y;
}

float Magnitude(const pipeframe::Vector2f value) {
    return std::sqrt(Dot(value, value));
}

} // namespace

RaceSegment::RaceSegment(const pipeframe::Vector2f firstPoint, const pipeframe::Vector2f secondPoint) {
    Set(firstPoint, secondPoint);
}

void RaceSegment::Set(const pipeframe::Vector2f firstPoint, const pipeframe::Vector2f secondPoint) {
    first = firstPoint;
    second = secondPoint;

    const pipeframe::Vector2f displacement = second - first;
    length = Magnitude(displacement);
    direction = length > 0.0001f ? displacement / length : pipeframe::Vector2f{1.0f, 0.0f};
}

pipeframe::Vector2f RaceSegment::GetFirstPoint() const { return first; }
pipeframe::Vector2f RaceSegment::GetSecondPoint() const { return second; }
pipeframe::Vector2f RaceSegment::GetDirection() const { return direction; }
float RaceSegment::GetLength() const { return length; }

float RaceSegment::GetAngleDegrees() const {
    return std::atan2(direction.y, direction.x) * 180.0f / std::numbers::pi_v<float>;
}

bool RaceSegment::IsValid() const { return length > 0.0001f; }

pipeframe::Vector2f RaceSegment::Project(const pipeframe::Vector2f point, const bool halfLine) const {
    if (!IsValid()) {
        return first;
    }

    const float projection = Dot(point - first, direction);
    const float distance = halfLine ? std::max(projection, 0.0f) : std::clamp(projection, 0.0f, length);
    return first + direction * distance;
}

float RaceSegment::DistanceTo(const pipeframe::Vector2f point, const bool halfLine) const {
    return Magnitude(point - Project(point, halfLine));
}

} // namespace sailboat_simulation

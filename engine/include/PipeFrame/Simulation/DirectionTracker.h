#ifndef ANT_TRACKING_DIRECTION_H
#define ANT_TRACKING_DIRECTION_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe {

class DirectionTracker {
public:
    DirectionTracker() = default;

    explicit DirectionTracker(float trackingSpeed);

    void Update(float deltaTime);

    void SetDirectionInstant(pipeframe::Vector2f direction);

    void SetAngleInstant(float angle);

    void SetTarget(pipeframe::Vector2f direction);

    void SetSpeed(float speed);

    [[nodiscard]]
    float GetAngle() const;

    [[nodiscard]]
    float GetSpeed() const;

    [[nodiscard]]
    pipeframe::Vector2f GetDirection() const;

    [[nodiscard]]
    pipeframe::Vector2f GetTarget() const;

private:
    float currentAngle{0.0f};

    pipeframe::Vector2f currentDirection{
        1.0f,
        0.0f,
    };

    pipeframe::Vector2f target{
        1.0f,
        0.0f,
    };

    float speed{1.0f};
};

}  // namespace pipeframe

#include <cmath>

namespace pipeframe {

namespace {

pipeframe::Vector2f Normalize(const pipeframe::Vector2f value) {
    const float lengthSquared = value.x * value.x + value.y * value.y;

    if (lengthSquared <= 0.000001f) {
        return {
            1.0f,
            0.0f,
        };
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);

    return value * inverseLength;
}

}  // namespace

inline DirectionTracker::DirectionTracker(const float trackingSpeed) : speed(trackingSpeed) {}

inline void DirectionTracker::Update(const float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }

    const pipeframe::Vector2f temporaryDirection{
        std::cos(currentAngle),
        std::sin(currentAngle),
    };

    const pipeframe::Vector2f normal{
        -temporaryDirection.y,
        temporaryDirection.x,
    };

    const float turnAmount = normal.x * target.x + normal.y * target.y;

    currentAngle += speed * turnAmount * deltaTime;

    currentDirection = {
        std::cos(currentAngle),
        std::sin(currentAngle),
    };
}

inline void DirectionTracker::SetDirectionInstant(const pipeframe::Vector2f direction) {
    currentDirection = Normalize(direction);

    target = currentDirection;

    currentAngle = std::atan2(currentDirection.y, currentDirection.x);
}

inline void DirectionTracker::SetAngleInstant(const float angle) {
    currentAngle = angle;

    currentDirection = {
        std::cos(currentAngle),
        std::sin(currentAngle),
    };

    target = currentDirection;
}

inline void DirectionTracker::SetTarget(const pipeframe::Vector2f direction) {
    target = Normalize(direction);
}

inline void DirectionTracker::SetSpeed(const float newSpeed) {
    speed = newSpeed;
}

inline float DirectionTracker::GetAngle() const {
    return currentAngle;
}

inline float DirectionTracker::GetSpeed() const {
    return speed;
}

pipeframe::Vector2f inline DirectionTracker::GetDirection() const {
    return currentDirection;
}

pipeframe::Vector2f inline DirectionTracker::GetTarget() const {
    return target;
}

}  // namespace pipeframe
#endif
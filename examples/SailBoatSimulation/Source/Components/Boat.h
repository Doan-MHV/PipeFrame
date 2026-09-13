#ifndef SAILBOAT_BOAT_H
#define SAILBOAT_BOAT_H

#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

namespace sailboat_simulation {

struct TrajectoryPoint final {
    pipeframe::Vector2f position{};
    pipeframe::Vector2f direction{1.0f, 0.0f};
    float time{0.0f};
};

class Boat final {
public:
    Boat() = default;
    Boat(pipeframe::Vector2f position, float angleRadians);

    void Reset(pipeframe::Vector2f position, float angleRadians);

    [[nodiscard]] pipeframe::Vector2f GetPosition() const;
    [[nodiscard]] pipeframe::Vector2f GetDirection() const;
    [[nodiscard]] float GetAngleRadians() const;
    [[nodiscard]] float GetSpeed() const;
    [[nodiscard]] float GetElapsedTime() const;
    [[nodiscard]] float GetDistanceTravelled() const;
    [[nodiscard]] float GetRudderCommand() const;
    [[nodiscard]] bool HasCrashed() const;
    [[nodiscard]] bool HasFinished() const;
    [[nodiscard]] const std::vector<TrajectoryPoint> &GetTrajectory() const;

    void MarkFinished();

private:
    friend class BoatMovementSystem;

    pipeframe::Vector2f position{};
    float angleRadians{0.0f};
    float speed{0.0f};
    float elapsedTime{0.0f};
    float distanceTravelled{0.0f};
    float rudderCommand{0.0f};
    bool crashed{false};
    bool finished{false};
    std::vector<TrajectoryPoint> trajectory;
};

} // namespace sailboat_simulation

#endif

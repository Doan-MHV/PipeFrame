#ifndef SAILBOAT_RACE_SEGMENT_H
#define SAILBOAT_RACE_SEGMENT_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace sailboat_simulation {

class RaceSegment final {
public:
    RaceSegment() = default;
    RaceSegment(pipeframe::Vector2f firstPoint, pipeframe::Vector2f secondPoint);

    void Set(pipeframe::Vector2f firstPoint, pipeframe::Vector2f secondPoint);

    [[nodiscard]] pipeframe::Vector2f GetFirstPoint() const;
    [[nodiscard]] pipeframe::Vector2f GetSecondPoint() const;
    [[nodiscard]] pipeframe::Vector2f GetDirection() const;
    [[nodiscard]] float GetLength() const;
    [[nodiscard]] float GetAngleDegrees() const;
    [[nodiscard]] bool IsValid() const;

    [[nodiscard]] pipeframe::Vector2f Project(pipeframe::Vector2f point, bool halfLine) const;
    [[nodiscard]] float DistanceTo(pipeframe::Vector2f point, bool halfLine) const;

private:
    pipeframe::Vector2f first{};
    pipeframe::Vector2f second{};
    pipeframe::Vector2f direction{1.0f, 0.0f};
    float length{0.0f};
};

} // namespace sailboat_simulation

#endif

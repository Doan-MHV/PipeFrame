#ifndef SAILBOAT_RACE_COURSE_H
#define SAILBOAT_RACE_COURSE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

#include "RaceSegment.h"

namespace sailboat_simulation {

struct RaceMark final {
    RaceSegment segment;
    float radius{10.0f};
    std::int64_t order{0};
};

class RaceCourse final {
public:
    void Clear();
    void SetWorldSize(pipeframe::Vector2f size);
    void SetStart(RaceSegment segment);
    void SetFinish(RaceSegment segment);
    void AddWaypoint(RaceMark waypoint);
    void SortWaypoints();

    [[nodiscard]] pipeframe::Vector2f GetWorldSize() const;
    [[nodiscard]] const RaceSegment &GetStart() const;
    [[nodiscard]] const RaceSegment &GetFinish() const;
    [[nodiscard]] const std::vector<RaceMark> &GetWaypoints() const;
    [[nodiscard]] std::size_t GetTargetCount() const;
    [[nodiscard]] const RaceSegment &GetTarget(std::size_t index) const;
    [[nodiscard]] bool Validate(std::string &errorMessage) const;

private:
    pipeframe::Vector2f worldSize{1600.0f, 1600.0f};
    RaceSegment start;
    RaceSegment finish;
    std::vector<RaceMark> waypoints;
};

} // namespace sailboat_simulation

#endif

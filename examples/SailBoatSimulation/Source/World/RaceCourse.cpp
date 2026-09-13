#include "RaceCourse.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sailboat_simulation {

void RaceCourse::Clear() {
    start = {};
    finish = {};
    waypoints.clear();
}

void RaceCourse::SetWorldSize(const pipeframe::Vector2f size) { worldSize = size; }
void RaceCourse::SetStart(RaceSegment segment) { start = std::move(segment); }
void RaceCourse::SetFinish(RaceSegment segment) { finish = std::move(segment); }
void RaceCourse::AddWaypoint(RaceMark waypoint) { waypoints.push_back(std::move(waypoint)); }

void RaceCourse::SortWaypoints() {
    std::stable_sort(waypoints.begin(), waypoints.end(), [](const RaceMark &left, const RaceMark &right) {
        return left.order < right.order;
    });
}

pipeframe::Vector2f RaceCourse::GetWorldSize() const { return worldSize; }
const RaceSegment &RaceCourse::GetStart() const { return start; }
const RaceSegment &RaceCourse::GetFinish() const { return finish; }
const std::vector<RaceMark> &RaceCourse::GetWaypoints() const { return waypoints; }
std::size_t RaceCourse::GetTargetCount() const { return waypoints.size() + 1; }

const RaceSegment &RaceCourse::GetTarget(const std::size_t index) const {
    return index < waypoints.size() ? waypoints[index].segment : finish;
}

bool RaceCourse::Validate(std::string &errorMessage) const {
    errorMessage.clear();

    if (!std::isfinite(worldSize.x) || !std::isfinite(worldSize.y) || worldSize.x <= 0.0f || worldSize.y <= 0.0f) {
        errorMessage = "Race world size must be finite and greater than zero.";
        return false;
    }

    if (!start.IsValid()) {
        errorMessage = "Race start must have a non-zero direction line.";
        return false;
    }

    if (!finish.IsValid()) {
        errorMessage = "Finish line must have non-zero length.";
        return false;
    }

    for (const RaceMark &waypoint : waypoints) {
        if (!waypoint.segment.IsValid() || !std::isfinite(waypoint.radius) || waypoint.radius <= 0.0f) {
            errorMessage = "Every waypoint must have a valid direction line and radius.";
            return false;
        }
    }

    return true;
}

} // namespace sailboat_simulation

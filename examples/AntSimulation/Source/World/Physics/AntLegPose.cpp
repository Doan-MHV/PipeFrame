#include "Components/AntPoseComponent.h"

#include <algorithm>
#include <cmath>

namespace ant_simulation {

void AntLegPose::Initialize(const pipeframe::Vector2f newRelativeStart, const pipeframe::Vector2f newRelativeEnd,
                            const pipeframe::Vector2f initialWorldPosition) {
    relativeStart = newRelativeStart;
    relativeEnd = newRelativeEnd;

    interpolationStart = initialWorldPosition;

    currentWorldEnd = initialWorldPosition;

    targetWorldEnd = initialWorldPosition;

    interpolationProgress = 1.0f;

    referenceDistance = Distance(relativeStart, relativeEnd);
}

void AntLegPose::Advance(const float deltaTime) {
    if (deltaTime <= 0.0f || IsDone()) {
        return;
    }

    interpolationProgress = std::min(1.0f, interpolationProgress + InterpolationSpeed * deltaTime);

    currentWorldEnd = interpolationStart + (targetWorldEnd - interpolationStart) * interpolationProgress;
}

void AntLegPose::UpdateReference(const pipeframe::Vector2f antPosition, const float antAngle) {
    const pipeframe::Vector2f referenceWorldEnd = TransformPoint(relativeEnd, antPosition, antAngle);

    const pipeframe::Vector2f worldStart = TransformPoint(relativeStart, antPosition, antAngle);

    const float distanceToReference = Distance(referenceWorldEnd, currentWorldEnd);

    const float currentLegLength = Distance(worldStart, currentWorldEnd);

    if (distanceToReference > 0.5f || std::abs(currentLegLength - referenceDistance) > 0.75f) {
        SetTargetWorldEnd(referenceWorldEnd);
    }
}

bool AntLegPose::IsDone() const { return interpolationProgress >= 1.0f; }

pipeframe::Vector2f AntLegPose::GetRelativeStart() const { return relativeStart; }

pipeframe::Vector2f AntLegPose::GetRelativeEnd() const { return relativeEnd; }

pipeframe::Vector2f AntLegPose::GetWorldStart(const pipeframe::Vector2f antPosition, const float antAngle) const {
    return TransformPoint(relativeStart, antPosition, antAngle);
}

pipeframe::Vector2f AntLegPose::GetCurrentWorldEnd() const { return currentWorldEnd; }

pipeframe::Vector2f AntLegPose::GetTargetWorldEnd() const { return targetWorldEnd; }

float AntLegPose::GetReferenceDistance() const { return referenceDistance; }

pipeframe::Vector2f AntLegPose::TransformPoint(const pipeframe::Vector2f point, const pipeframe::Vector2f position,
                                               const float angle) {
    const float cosine = std::cos(angle);

    const float sine = std::sin(angle);

    return {
        position.x + point.x * cosine - point.y * sine,
        position.y + point.x * sine + point.y * cosine,
    };
}

float AntLegPose::Distance(const pipeframe::Vector2f first, const pipeframe::Vector2f second) {
    const pipeframe::Vector2f difference = first - second;

    return std::sqrt(difference.x * difference.x + difference.y * difference.y);
}

void AntLegPose::SetTargetWorldEnd(const pipeframe::Vector2f target) {
    if (target == targetWorldEnd) {
        return;
    }

    interpolationStart = currentWorldEnd;

    targetWorldEnd = target;
    interpolationProgress = 0.0f;
}

} // namespace ant_simulation
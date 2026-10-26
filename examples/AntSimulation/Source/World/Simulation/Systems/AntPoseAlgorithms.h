#pragma once
#include "Components/AntPoseComponent.h"
#include <PipeFrame/Components/Transform2DComponent.h>
namespace ant_simulation {
inline void StepAntLegs(AntPoseComponent &pose, const pipeframe::Transform2DComponent &transform, float deltaTime) {
    for (AntLegPose &leg : pose.legs) {
        leg.Advance(deltaTime);
    }

    bool canMove[2]{
        pose.legs[0].IsDone() && pose.legs[2].IsDone() && pose.legs[4].IsDone(),
        pose.legs[1].IsDone() && pose.legs[3].IsDone() && pose.legs[5].IsDone(),
    };

    for (std::size_t index = 0; index < pose.legs.size(); ++index) {
        AntLegPose &leg = pose.legs[index];

        const std::size_t group = index % 2;

        if (!canMove[group]) {
            continue;
        }

        const std::size_t alternativeIndex = group != 0 ? index - 1 : index + 1;

        if (!pose.legs[alternativeIndex].IsDone()) {
            continue;
        }

        leg.UpdateReference(transform.position, pose.direction.GetAngle());

        if (!leg.IsDone()) {
            canMove[group] = false;
        }
    }
}
inline void InitializeAntLegs(AntPoseComponent &pose, const pipeframe::Transform2DComponent &transform) {
    constexpr pipeframe::Vector2f referenceX[]{
        {-0.05f, -0.15f},
        {0.0f, 0.25f},
        {0.1f, 0.65f},
    };

    constexpr float referenceY[]{
        0.4f,
        0.5f,
        0.45f,
    };

    for (std::size_t index = 0; index < 3; ++index) {
        pose.legs[index * 2].Initialize(
            {
                referenceX[index].x,
                0.0f,
            },
            {
                referenceX[index].y,
                referenceY[index],
            },
            transform.position);

        pose.legs[index * 2 + 1].Initialize(
            {
                referenceX[index].x,
                0.0f,
            },
            {
                referenceX[index].y,
                -referenceY[index],
            },
            transform.position);
    }
}
inline void AdvanceAntPose(AntPoseComponent &pose, pipeframe::Transform2DComponent &transform, float deltaTime) {
    pose.direction.Update(deltaTime);
    pose.headDirection.Update(deltaTime);
    pose.tailDirection.Update(deltaTime);
    transform.rotation = pose.direction.GetAngle();
}
} // namespace ant_simulation

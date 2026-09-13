#pragma once
#include <algorithm>
#include <cmath>
#include <PipeFrame/Foundation/MathTypes.h>
namespace pipeframe {
    [[nodiscard]] inline Vector2f SeekVelocity(Vector2f position, Vector2f currentVelocity,
                                               Vector2f target, float speed, float response = 1.0f) {
        const auto delta = target - position;
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        const auto desired = length > 0.0f ? delta / length * std::max(0.0f, speed) : Vector2f{};
        const float blend = std::clamp(response, 0.0f, 1.0f);
        return currentVelocity * (1.0f - blend) + desired * blend;
    }

}

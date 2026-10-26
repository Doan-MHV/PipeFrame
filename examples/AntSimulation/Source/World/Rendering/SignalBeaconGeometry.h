#include <PipeFrame/Render/Canvas.h>
#pragma once
#include "Components/SignalBeaconComponent.h"
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Render/RenderTypes.h>
#include <cmath>
namespace ant_simulation {
// Backend-neutral geometry. The existing render host submits these vertices.
inline std::vector<pipeframe::Vertex2D> BuildSignalBeaconGeometry(const pipeframe::Transform2DComponent &transform,
                                                                  const SignalBeaconComponent &settings) {
    const float radius = static_cast<float>(settings.radius);
    const float c = std::cos(transform.rotation), s = std::sin(transform.rotation);
    const auto point = [&](float x, float y) {
        x *= transform.scale.x;
        y *= transform.scale.y;
        return transform.position + pipeframe::Vector2f{x * c - y * s, x * s + y * c};
    };
    return {{point(radius, 0), settings.color, {}},
            {point(-radius, -radius * .65f), settings.color, {}},
            {point(-radius, radius * .65f), settings.color, {}}};
}
} // namespace ant_simulation

#pragma once

#include <PipeFrame/Foundation/MathTypes.h>

#include <algorithm>
#include <cmath>

namespace pipeframe::ui {
// Backend-neutral camera for maps, diagrams and other interactive mesh views.
// span is the world distance across the shorter widget dimension. +Y points down.
struct Viewport2D {
  Vector2f center{};
  float span{10};
  float minimumSpan{0.5f};
  float maximumSpan{4096};

  bool Valid() const {
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(span) &&
           std::isfinite(minimumSpan) && std::isfinite(maximumSpan) && minimumSpan > 0 &&
           maximumSpan >= minimumSpan && span >= minimumSpan && span <= maximumSpan;
  }

  Vector2f WorldAt(Vector2f normalizedOffset) const { return center + normalizedOffset * span; }
  void Pan(Vector2f normalizedDelta) { center -= normalizedDelta * span; }
  void Zoom(float wheelSteps, Vector2f anchor = {}) {
    if (!std::isfinite(wheelSteps)) return;
    const auto worldAnchor = WorldAt(anchor);
    span = std::clamp(
      span * std::exp(-std::clamp(wheelSteps, -20.0f, 20.0f) * 0.15f), minimumSpan, maximumSpan);
    center = worldAnchor - anchor * span;
  }
};
}  // namespace pipeframe::ui

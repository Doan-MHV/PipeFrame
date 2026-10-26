#pragma once

#include <PipeFrame/UI/Viewport2D.h>

#include "World/Simulation/RoverMappingSystem.h"

namespace RoverLab {
// Per-rover presentation state, independent of the mapping/simulation state.
struct RoverMapCamera {
  pipeframe::ui::Viewport2D view{{}, 8};
  bool following{true};
  pipeframe::Vector2f viewportSize{300, 300};
  std::uint64_t generation{};

  void Track(const RoverMappingSystem::Map& map) {
    if (generation != map.generation) {
      const auto size = viewportSize;
      *this = RoverMapCamera{};
      viewportSize = size;
      generation = map.generation;
    }
    if (following) Center(map);
  }

  void Center(const RoverMappingSystem::Map& map) {
    view.center = {float(map.pose.x), float(map.pose.y)};
  }

  void Change(const pipeframe::ui::Viewport2D& camera, bool panned) {
    view = camera;
    if (panned) following = false;
  }

  pipeframe::Vector2f HalfView() const {
    const pipeframe::Vector2f size{std::max(1.0f, viewportSize.x), std::max(1.0f, viewportSize.y)};
    const float extent = std::min(size.x, size.y);
    return size * (view.span / (2 * extent));
  }

  void Fit(const RoverMappingSystem::Map& map) {
    double left = map.pose.x, right = left, top = map.pose.y, bottom = top;
    for (int row = 0; row < map.height; ++row) {
      for (int col = 0; col < map.width; ++col) {
        if (!map.cells[std::size_t(row * map.width + col)]) continue;
        const double x = map.originX + col * map.settings.cellSize;
        const double y = map.originY + row * map.settings.cellSize;
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x + map.settings.cellSize);
        bottom = std::max(bottom, y + map.settings.cellSize);
      }
    }
    following = false;
    view.center = {float((left + right) / 2), float((top + bottom) / 2)};
    const auto half = HalfView();
    view.span = std::clamp(float(std::max((right - left) / (2 * half.x / view.span),
                                   (bottom - top) / (2 * half.y / view.span)) *
                                 1.15),
      view.minimumSpan,
      view.maximumSpan);
  }
};
}  // namespace RoverLab

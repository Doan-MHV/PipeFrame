#pragma once

#include <numbers>
#include <vector>

#include "MazeSolver.h"

namespace RoverLab {

struct MeasuredPose {
  MazePoint position;
  double heading{};
};

// A position-only polyline cannot describe a small vehicle's corner manoeuvre.
// Keep the measured headings, including turns made without translation.
class MeasuredRoute {
public:
  std::vector<MeasuredPose> poses{{{0, 0}, 0}};

  static double Angle(double value) { return std::remainder(value, 2 * std::numbers::pi); }

  void Record(MazePoint position, double heading, bool final = false) {
    const MeasuredPose next{position, heading};
    const auto& last = poses.back();
    if (!final && Distance(last.position, position) < .002 &&
        std::abs(Angle(heading - last.heading)) < .02)
      return;

    bool merge = false;
    if (poses.size() >= 2) {
      const auto& a = poses[poses.size() - 2];
      const auto& b = poses.back();
      const double dx = position.x - a.position.x, dy = position.y - a.position.y;
      const double length = std::hypot(dx, dy);
      // Retain quarter-turn endpoints so a complete survey is not erased by
      // wrapped angles. A reverse replay follows the same swept orientations.
      const double firstTurn = Angle(b.heading - a.heading);
      const double nextTurn = Angle(heading - b.heading);
      if (Distance(a.position, position) < .0001 && Distance(a.position, b.position) < .0001 &&
          firstTurn * nextTurn >= 0 && std::abs(firstTurn + nextTurn) <= std::numbers::pi / 2)
        merge = true;

      if (length > 0 && std::abs(Angle(heading - a.heading)) < .001 &&
          std::abs(Angle(b.heading - a.heading)) < .001 &&
          std::abs((b.position.x - a.position.x) * dy - (b.position.y - a.position.y) * dx) /
              length <
            .0001 &&
          (b.position.x - a.position.x) * (position.x - b.position.x) +
              (b.position.y - a.position.y) * (position.y - b.position.y) >
            0)
        merge = true;
    }
    if (merge)
      poses.back() = next;
    else if (poses.size() < 100000)
      poses.push_back(next);
  }

  // Close only loops that return to the same position AND orientation. Merely
  // crossing a previous position does not prove a turn fits there.
  void CloseLoops() {
    std::vector<MeasuredPose> result;
    for (const auto& pose : poses) {
      bool closed = false;
      for (std::size_t i = 0; i + 1 < result.size(); ++i) {
        if (Distance(result[i].position, pose.position) <= .0005 &&
            std::abs(Angle(result[i].heading - pose.heading)) <= .003) {
          result.resize(i + 1);
          closed = true;
          break;
        }
      }
      if (!closed) {
        for (std::size_t i = 1; i < result.size(); ++i) {
          const auto a = result[i - 1], b = result[i];
          if (std::abs(Angle(a.heading - b.heading)) > .003 ||
              std::abs(Angle(pose.heading - a.heading)) > .003)
            continue;
          const double dx = b.position.x - a.position.x;
          const double dy = b.position.y - a.position.y;
          const double lengthSquared = dx * dx + dy * dy;
          if (lengthSquared < 1e-8) continue;
          const double t =
            ((pose.position.x - a.position.x) * dx + (pose.position.y - a.position.y) * dy) /
            lengthSquared;
          if (t < 0 || t > 1) continue;
          if (Distance(pose.position, {a.position.x + t * dx, a.position.y + t * dy}) <= .0005) {
            // Recovery backed onto the interior of an earlier straight leg.
            // Keep its actual endpoint, but remove the refused wall approach.
            result.resize(i);
            break;
          }
        }
      }
      result.push_back(pose);
    }
    poses = std::move(result);
  }

private:
  static double Distance(MazePoint a, MazePoint b) { return std::hypot(a.x - b.x, a.y - b.y); }
};

}  // namespace RoverLab

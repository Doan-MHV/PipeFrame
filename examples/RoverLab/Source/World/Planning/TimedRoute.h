#pragma once

#include <numbers>

#include "MazeSolver.h"

namespace RoverLab {

// Check a straight manoeuvre at the actual heading, rather than demanding
// that a rover beside a corner can first rotate onto a cardinal grid heading.
// Every intersected cell must be known free (or previously occupied by the
// chassis). A caller may opt into a <=20 cm local exploration probe; known-route
// callers remain strict. Positive obstacle evidence is never bypassed.
inline bool KnownStraightClear(const MazeGrid& g,
  MazePoint from,
  MazePoint to,
  double heading,
  double halfLength,
  double halfWidth,
  const std::set<MazeKey>& covered,
  bool allowLocalUnknown = false) {
  const double c = std::cos(heading), s = std::sin(heading);
  const double travelX = to.x - from.x, travelY = to.y - from.y;
  // The swept rectangle is centred halfway along the translation. Extending
  // the initial footprint in both directions would incorrectly add clearance
  // toward the wall even when the rover is moving away from it.
  halfLength += std::abs(c * travelX + s * travelY) / 2;
  halfWidth += std::abs(-s * travelX + c * travelY) / 2;
  const MazePoint p{(from.x + to.x) / 2, (from.y + to.y) / 2};
  const double extentX = std::abs(c) * halfLength + std::abs(s) * halfWidth;
  const double extentY = std::abs(s) * halfLength + std::abs(c) * halfWidth;
  const double projection = g.cell * .5 * (std::abs(c) + std::abs(s));
  const int left = int(std::floor((p.x - extentX - g.x) / g.cell));
  const int right = int(std::floor((p.x + extentX - g.x) / g.cell));
  const int top = int(std::floor((p.y - extentY - g.y) / g.cell));
  const int bottom = int(std::floor((p.y + extentY - g.y) / g.cell));
  if (left < 0 || top < 0 || right >= g.width || bottom >= g.height) return false;
  for (int y = top; y <= bottom; ++y)
    for (int x = left; x <= right; ++x) {
      const int i = y * g.width + x;
      const auto centre = g.Point(i);
      const double dx = centre.x - p.x, dy = centre.y - p.y;
      if (std::abs(c * dx + s * dy) > halfLength + projection ||
          std::abs(-s * dx + c * dy) > halfWidth + projection)
        continue;
      const bool localProbe =
        allowLocalUnknown && std::hypot(centre.x - from.x, centre.y - from.y) <= .20;
      if (g.evidence[i] > 0 || (g.evidence[i] == 0 && !covered.contains(g.Key(i)) && !localProbe))
        return false;
    }
  return true;
}

// Check the complete intended turn before starting it. Checking only the next
// small motor command can leave the chassis halfway through an impossible turn.
inline bool KnownTurnClear(const MazeGrid& g,
  MazePoint position,
  double heading,
  double angle,
  double halfLength,
  double halfWidth,
  const std::set<MazeKey>& covered,
  bool allowLocalUnknown = false) {
  const int steps = std::max(1, int(std::ceil(std::abs(angle) / .02)));
  // Cover the motion between samples as well as the sampled rectangles.
  const double padding = std::hypot(halfLength, halfWidth) * std::abs(angle) / steps / 2;
  for (int i = 0; i <= steps; ++i)
    if (!KnownStraightClear(g,
          position,
          position,
          heading + angle * i / steps,
          halfLength + padding,
          halfWidth + padding,
          covered,
          allowLocalUnknown && std::abs(angle) <= .10))
      return false;
  return true;
}

using TimedRoute = MazeRoute;

// Match the follower's capped proportional turn controller down to 0.01 rad.
inline double RouteTurnSeconds(double angle, double turnRate) {
  angle = std::abs(std::remainder(angle, 2 * std::numbers::pi));
  if (angle <= .01) return 0.0;

  const double transition = std::max(.01, turnRate / 4);
  return std::max(0.0, angle - transition) / turnRate +
         std::log(std::min(angle, transition) / .01) / 4;
}

inline double RouteTravelSeconds(
  const MazeRoute& route, double heading, double cellSize, double speed, double turnRate) {
  double seconds = 0;
  for (std::size_t i = 0; i < route.cells.size(); ++i) {
    const double nextHeading = route.headings[i] * std::numbers::pi / 2;
    seconds += RouteTurnSeconds(nextHeading - heading, turnRate);
    if (i > 0) seconds += cellSize / speed;
    heading = nextHeading;
  }
  return seconds;
}

// A* over (cell, heading), with forward/reverse translation and in-place turns.
// The admissible heuristic excludes turns. This minimizes estimated travel time
// on the known four-neighbour grid, not real-world time under noisy sensing.
inline TimedRoute FastestKnownRoute(const MazeGrid& g,
  int start,
  int goal,
  double heading,
  double speed,
  double turnRate,
  std::vector<int> destinations = {},
  std::vector<double>* travelTimes = nullptr) {
  if (travelTimes) travelTimes->assign(g.Count(), std::numeric_limits<double>::infinity());
  if (!g.Open(start) || (!travelTimes && !g.Open(goal)) || !std::isfinite(heading) ||
      !std::isfinite(speed) || speed <= 0 || !std::isfinite(turnRate) || turnRate <= 0)
    return {};
  if (destinations.empty()) destinations.push_back(goal);
  std::erase_if(destinations, [&](int cell) { return !g.Open(cell); });
  if (destinations.empty() && !travelTimes) return {};
  const auto atDestination = [&](int cell) {
    return !travelTimes &&
           std::find(destinations.begin(), destinations.end(), cell) != destinations.end();
  };
  if (atDestination(start)) return {{start}, {0}, 0};

  constexpr double pi = std::numbers::pi;
  const int count = g.Count() * 4;
  std::vector<double> cost(count, std::numeric_limits<double>::infinity());
  std::vector<int> parent(count, -1);
  using Entry = std::pair<double, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;

  const auto heuristic = [&](int cell) {
    if (travelTimes) return 0.0;
    double best = std::numeric_limits<double>::infinity();
    for (int target : destinations)
      best = std::min(best,
        (std::abs(cell % g.width - target % g.width) +
          std::abs(cell / g.width - target / g.width)) *
          g.cell / speed);
    return best;
  };
  const MazeMotionGraph motion{g};
  const auto turnSeconds = [&](double angle) {
    return RouteTurnSeconds(angle, turnRate);
  };

  for (int state : motion.Starts(start, heading)) {
    const double angle = std::abs(std::remainder((state % 4) * pi / 2 - heading, 2 * pi));
    cost[state] = turnSeconds(angle);
    parent[state] = state;
    queue.emplace(cost[state] + heuristic(start), state);
  }

  while (!queue.empty()) {
    const auto [priority, state] = queue.top();
    queue.pop();
    const int cell = state / 4, facing = state % 4;
    if (priority > cost[state] + heuristic(cell) + 1e-9) continue;
    if (atDestination(cell)) {
      TimedRoute result;
      result.seconds = cost[state];
      for (int node = state;; node = parent[node]) {
        result.cells.push_back(node / 4);
        result.headings.push_back(node % 4);
        if (parent[node] == node) break;
      }
      std::reverse(result.cells.begin(), result.cells.end());
      std::reverse(result.headings.begin(), result.headings.end());
      return result;
    }

    for (int d = 0; d < 4; ++d) {
      const int next = g.Next(cell, d);
      if (next < 0) continue;
      for (int facingNext : {d, (d + 2) % 4}) {
        const int node = motion.Next(state, d, facingNext);
        if (node < 0) continue;
        const double candidate =
          cost[state] + g.cell / speed + turnSeconds((facingNext - facing) * pi / 2);
        if (candidate + 1e-9 >= cost[node]) continue;
        cost[node] = candidate;
        parent[node] = state;
        queue.emplace(candidate + heuristic(next), node);
      }
    }
  }
  if (travelTimes)
    for (int cell = 0; cell < g.Count(); ++cell)
      (*travelTimes)[cell] =
        *std::min_element(cost.begin() + cell * 4, cost.begin() + cell * 4 + 4);
  return {};
}

// One Dijkstra traversal gives frontier scoring the same feasible moves and
// turning costs as the route that will actually be driven.
inline std::vector<double> KnownTravelTimes(
  const MazeGrid& g, int start, double heading, double speed, double turnRate) {
  std::vector<double> times;
  FastestKnownRoute(g, start, -1, heading, speed, turnRate, {}, &times);
  return times;
}

}  // namespace RoverLab

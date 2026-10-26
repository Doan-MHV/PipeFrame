#pragma once

#include <map>
#include <optional>

#include "MazeSolver.h"
#include "TimedRoute.h"
#include "World/Simulation/RoverMappingSystem.h"

namespace RoverLab {
struct FrontierAttempt {
  std::uint64_t signature{};
  unsigned count{};
  double retryAt{};
};

struct MazeMemory {
  std::map<MazeKey, int> visits;
  std::set<MazeKey> covered, searched, attempted;
  std::optional<MazeKey> last, searchRegion;
  std::map<MazeKey, std::uint64_t> searchRegions;
  std::uint64_t nextSearchRegion{};
  std::map<MazeKey, FrontierAttempt> trials;
  std::map<MazeKey, FrontierAttempt> observationAreas;
  std::map<MazeEdge, double> blockedEdges;
  double time{};
};

inline MazeGrid BuildMazeGrid(const RoverMappingSystem::Map& map,
  MazeMemory& memory,
  double halfLength,
  double halfWidth,
  double clearance,
  double stopDistance,
  double bodyHalfLength,
  double bodyHalfWidth,
  bool allowProbe = true,
  bool strictKnown = false) {
  MazeGrid g;
  g.width = map.width;
  g.height = map.height;
  g.cell = map.settings.cellSize;
  g.x = map.originX;
  g.y = map.originY;

  for (const auto& [edge, expiry] : memory.blockedEdges)
    if (expiry > memory.time) g.denied.insert(edge);

  g.evidence = map.cells;
  g.stoppingCells = int(std::ceil(stopDistance / g.cell));

  const int n = g.Count();
  g.open.resize(n);
  g.clear.assign(n, 3);
  g.turnClear.assign(n, 1);
  g.frontier.resize(n);
  g.visits.resize(n);

  const double c = std::cos(map.pose.heading), s = std::sin(map.pose.heading);

  for (int i = 0; i < n; ++i) {
    const auto p = g.Point(i);
    const double dx = p.x - map.pose.x, dy = p.y - map.pose.y;

    // Arrival search is about the rover centre passing a floor location.
    // Sensor visibility and the wider chassis footprint do not prove arrival.
    if (std::hypot(dx, dy) <= g.cell * 1.5) memory.searched.insert(g.Key(i));

    // Only the actual occupied chassis footprint seeds unknown cells at startup.
    // Positive obstacle evidence is never overwritten, and this does not change the map.
    if (map.cells[i] <= 0 && std::abs(c * dx + s * dy) <= bodyHalfLength &&
        std::abs(-s * dx + c * dy) <= bodyHalfWidth)
      memory.covered.insert(g.Key(i));

    // Unknown is not free. Permit only a short, live-sensor-guarded local probe
    // to bridge raster gaps near narrow sonar cones. It will be driven at crawl speed.
    g.open[i] = map.cells[i] < 0 ||
                (map.cells[i] == 0 &&
                  (memory.covered.contains(g.Key(i)) || (allowProbe && std::hypot(dx, dy) <= .20)));

    if (map.cells[i] < 0) {
      const MazeKey region{int(std::floor(p.x / 2)), int(std::floor(p.y / 2))};
      if (!memory.searchRegions.contains(region))
        memory.searchRegions.emplace(region, memory.nextSearchRegion++);
    }

    const auto v = memory.visits.find(g.Key(i));
    g.visits[i] = v == memory.visits.end() ? 0 : v->second;
  }

  // Axis-aligned route segments use the full rectangle, not a circumscribed
  // circle. Drive checks the actual swept envelope while turning between them.
  const double length = halfLength + clearance + g.cell * .5;
  const double width = halfWidth + clearance + g.cell * .5;
  const double turnRadius = std::hypot(halfLength, halfWidth) + clearance + g.cell;
  const int r = int(std::ceil(turnRadius / g.cell));
  for (int i = 0; i < n; ++i)
    if (map.cells[i] > 0 ||
        (strictKnown && map.cells[i] == 0 && !memory.covered.contains(g.Key(i)))) {
      for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx) {
          const int x = i % g.width + dx, y = i / g.width + dy;
          if (x < 0 || y < 0 || x >= g.width || y >= g.height) continue;
          const double ax = std::abs(dx) * g.cell, ay = std::abs(dy) * g.cell;
          if (std::hypot(ax, ay) <= turnRadius) g.turnClear[y * g.width + x] = 0;
          if (ax <= length && ay <= width) g.clear[y * g.width + x] &= ~1u;
          if (ax <= width && ay <= length) g.clear[y * g.width + x] &= ~2u;
        }
    }

  if (strictKnown) {
    for (int i = 0; i < n; ++i) {
      const double x = (i % g.width + .5) * g.cell, y = (i / g.width + .5) * g.cell;
      const double horizontal = std::min(x, g.width * g.cell - x);
      const double vertical = std::min(y, g.height * g.cell - y);
      if (horizontal < length || vertical < width) g.clear[i] &= ~1u;
      if (horizontal < width || vertical < length) g.clear[i] &= ~2u;
      if (std::min(horizontal, vertical) < turnRadius) g.turnClear[i] = 0;
    }
  }

  g.poseCell = g.At({map.pose.x, map.pose.y});
  for (int d = 0; d < 4; ++d) {
    const double angle =
      std::remainder(d * std::numbers::pi / 2 - map.pose.heading, 2 * std::numbers::pi);
    if (KnownTurnClear(g,
          {map.pose.x, map.pose.y},
          map.pose.heading,
          angle,
          halfLength + clearance,
          halfWidth + clearance,
          memory.covered,
          allowProbe && std::abs(angle) <= .10))
      g.poseHeadings |= 1u << d;
  }

  // Select a reachable observation position near an unknown boundary.
  // Requiring the inflated robot centre to touch unknown directly can erase
  // every frontier in a corridor, even though it can be observed from nearby.
  const int look = int(std::ceil(.35 / g.cell));

  for (int i = 0; i < n; ++i) {
    g.open[i] = g.open[i] && g.clear[i];
    if (!g.open[i]) continue;
    const auto from = g.Point(i);
    for (int dy = -look; dy <= look; ++dy)
      for (int dx = -look; dx <= look; ++dx) {
        if (std::hypot(dx, dy) * g.cell > .35) continue;
        const int col = i % g.width + dx, row = i / g.width + dy;
        if (col < 0 || row < 0 || col >= g.width || row >= g.height) continue;
        const int j = row * g.width + col;
        if (map.cells[j] != 0 || memory.covered.contains(g.Key(j))) continue;
        const auto to = g.Point(j);
        const int steps = std::max(1, 2 * std::max(std::abs(dx), std::abs(dy)));
        bool visible = true;
        for (int k = 1; k <= steps && visible; ++k) {
          const double t = double(k) / steps;
          const int index = g.At({std::lerp(from.x, to.x, t), std::lerp(from.y, to.y, t)});
          visible = index >= 0 && map.cells[index] <= 0;
        }
        if (visible && g.frontier[i] < 255) ++g.frontier[i];
      }
  }

  return g;
}

// Ignore evidence strength changes: repeating the same scan must not reset retries.
// Keys and local occupancy classes remain stable as the map expands.
inline std::uint64_t FrontierSignature(const MazeGrid& g, int index) {
  if (g.evidence.empty()) return 0;

  std::uint64_t hash = 1469598103934665603ull;
  const int radius = std::max(1, int(std::ceil(.25 / g.cell)));

  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      const int x = index % g.width + dx, y = index / g.width + dy;
      int value = 0;
      if (x >= 0 && y >= 0 && x < g.width && y < g.height) {
        const int score = g.evidence[y * g.width + x];
        value = (score > 0) - (score < 0);
      }
      hash = (hash ^ std::uint64_t(value + 2)) * 1099511628211ull;
    }
  }

  return hash;
}

inline bool FrontierAvailable(const MazeGrid& g, int index, const MazeMemory& memory) {
  if (memory.attempted.contains(g.Key(index))) return false;

  const auto found = memory.trials.find(g.Key(index));
  if (found == memory.trials.end()) return true;

  const auto& trial = found->second;
  // Always respect a short cooldown, even while noisy evidence changes.
  if (memory.time < trial.retryAt) return false;

  return trial.count < 3 || trial.signature != FrontierSignature(g, index);
}

inline void RememberFrontier(const MazeGrid& g, MazeMemory& memory, MazeKey key) {
  const int index = g.At({key.first * g.cell / 2, key.second * g.cell / 2});
  if (index < 0) return;

  const auto signature = FrontierSignature(g, index);
  auto& trial = memory.trials[key];
  if (trial.signature != signature) trial.count = 0;

  trial.signature = signature;
  trial.count = std::min(3u, trial.count + 1);
  trial.retryAt = memory.time + 20 * trial.count;
}

// Select a reachable grid endpoint close to the learned physical position.
// A tiny odometry offset can put an otherwise identical position in an adjacent
// grid row; forcing that exact row can demand an impossible sideways turn.
inline int ChooseKnownDestination(const MazeGrid& g,
  int start,
  MazePoint position,
  MazePoint destination,
  double tolerance,
  bool preferShortest = false) {
  if (!g.Open(start)) return -1;

  const auto root = g.Point(start);
  const MazePoint offset{position.x - root.x, position.y - root.y};
  std::deque<int> queue{start};
  std::vector<bool> seen(g.Count());
  std::vector<int> steps(g.Count());
  seen[start] = true;
  int best = -1;
  int bestCost = std::numeric_limits<int>::max();
  double distance = tolerance;

  while (!queue.empty()) {
    const int u = queue.front();
    queue.pop_front();
    const auto point = g.Point(u);
    const double error =
      std::hypot(point.x + offset.x - destination.x, point.y + offset.y - destination.y);

    if (preferShortest && error <= tolerance) {
      // A one-cell sideways correction can be harder than continuing straight
      // for one more cell. Prefer an endpoint on the current row/column when
      // both are inside the same acceptance region.
      const bool extraAxis = std::abs(point.x + offset.x - position.x) > g.cell * .5 &&
                             std::abs(point.y + offset.y - position.y) > g.cell * .5;
      const int cost = steps[u] + (extraAxis ? 4 : 0);
      if (cost < bestCost || (cost == bestCost && error < distance)) {
        best = u;
        bestCost = cost;
        distance = error;
      }
    } else if (!preferShortest && error <= distance) {
      best = u;
      distance = error;
    }

    for (int d = 0; d < 4; ++d) {
      const int v = g.Next(u, d);
      if (v >= 0 && !seen[v]) {
        seen[v] = true;
        steps[v] = steps[u] + 1;
        queue.push_back(v);
      }
    }
  }

  return best;
}

// Search a stable lattice of floor positions, in long horizontal passes. The
// lattice is anchored in odometry space, so expanding the map cannot reset it.
// Frontiers are a fallback for revealing more floor, not a reason to starve
// already-observed room interiors of physical arrival checks.
inline int ChooseCoverageTarget(
  const MazeGrid& g, int start, MazeMemory& memory, double spacing = .30) {
  if (!g.Open(start)) return -1;

  const int stride = std::max(1, int(std::floor(spacing / g.cell + 1e-6)));
  const auto origin = g.Key(0);
  const int phaseX = (origin.first % 2 + 2) % 2;
  const int phaseY = (origin.second % 2 + 2) % 2;
  const auto candidate = [&](int index) {
    const auto key = g.Key(index);
    const auto point = g.Point(index);
    const auto sample = [&](double coordinate, int key, int phase) {
      // Include a pass at the end of each patch. A patch boundary must not
      // leave its final floor strip waiting for a much later neighbouring area.
      const double edge = 2 * (std::floor(coordinate / 2) + 1);
      return ((key - phase) / 2) % stride == 0 || edge - coordinate <= g.cell * 1.01;
    };
    return sample(point.x, key.first, phaseX) && sample(point.y, key.second, phaseY) &&
           !memory.searched.contains(key) && !g.evidence.empty() && g.evidence[index] < 0 &&
           FrontierAvailable(g, index, memory);
  };

  const auto region = [&](int index) {
    const auto point = g.Point(index);
    return MazeKey{int(std::floor(point.x / 2)), int(std::floor(point.y / 2))};
  };

  std::deque<int> queue{start};
  std::vector<int> distance(g.Count(), -1);
  distance[start] = 0;
  int goal = -1, nearby = -1;
  std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();

  while (!queue.empty()) {
    const int u = queue.front();
    queue.pop_front();

    if (u != start && candidate(u)) {
      const auto area = region(u);
      if (!memory.searchRegions.contains(area))
        memory.searchRegions.emplace(area, memory.nextSearchRegion++);
      const auto order = memory.searchRegions.at(area);
      if (nearby < 0 || order < oldest) {
        nearby = u;
        oldest = order;
      }
      if (memory.searchRegion && region(u) == *memory.searchRegion) {
        goal = u;
        break;
      }
    }

    for (int d = 0; d < 4; ++d) {
      const int v = g.Next(u, d);
      if (v >= 0 && distance[v] < 0) {
        distance[v] = distance[u] + 1;
        queue.push_back(v);
      }
    }
  }

  if (goal < 0) goal = nearby;
  if (goal < 0) return -1;
  memory.searchRegion = region(goal);

  // Finish a local patch before chasing newly revealed floor across the room.
  // Otherwise nearest-target selection can keep walking around its perimeter.
  // Once on a search row, commit to its unsearched straight run rather than
  // stopping, replanning and turning at every tiny grid cell. Row transitions
  // remain separate waypoints; never shortcut diagonally across an obstacle.
  if (goal / g.width == start / g.width) {
    const int direction = goal > start ? 0 : 2;
    int next = start;
    const int maximum = std::max(1, int(std::ceil(2.0 / g.cell)));
    for (int step = 0; step < maximum; ++step) {
      next = g.Next(next, direction);
      if (next < 0 || g.evidence[next] >= 0 || region(next) != *memory.searchRegion) break;
      const auto key = g.Key(next);
      if (step > stride && memory.searched.contains(key)) break;
      if (candidate(next)) goal = next;
    }
  }

  return goal;
}

// Small maze search uses the same heading/turn-clearance graph as the route
// solvers. A cell-only flood can repeatedly choose floor reachable only through
// an impossible sideways turn. Rank unsearched samples by actual travel + turns,
// then finish a straight pass in either axis before selecting another target.
// No marker position or authored wall data is available to this search.
inline int ChooseMicromouseTarget(const MazeGrid& g,
  int start,
  const MazeMemory& memory,
  double heading,
  double spacing,
  double speed,
  double turnRate) {
  const MazeMotionGraph motion{g};
  const auto roots = motion.Starts(start, heading);
  if (roots.empty()) return -1;
  const int stride = std::max(1, int(std::floor(spacing / g.cell + 1e-6)));
  const auto origin = g.Key(0);
  const auto candidate = [&](int cell) {
    const auto key = g.Key(cell);
    const int px = (origin.first % 2 + 2) % 2, py = (origin.second % 2 + 2) % 2;
    return cell != start && ((key.first - px) / 2) % stride == 0 &&
           ((key.second - py) / 2) % stride == 0 && g.evidence[cell] < 0 &&
           !memory.searched.contains(key) && FrontierAvailable(g, cell, memory);
  };
  using Entry = std::pair<double, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
  std::vector<double> cost(g.Count() * 4, std::numeric_limits<double>::infinity());
  for (int root : roots) {
    cost[root] = RouteTurnSeconds(root % 4 * std::numbers::pi / 2 - heading, turnRate);
    queue.emplace(cost[root], root);
  }
  int target = -1;
  while (!queue.empty()) {
    const auto [seconds, node] = queue.top();
    queue.pop();
    if (seconds != cost[node]) continue;
    if (candidate(node / 4)) {
      target = node / 4;
      break;
    }
    motion.EachNext(node, [&](int next) {
      const double value = seconds + g.cell / speed +
                           RouteTurnSeconds((next % 4 - node % 4) * std::numbers::pi / 2, turnRate);
      if (value < cost[next]) {
        cost[next] = value;
        queue.emplace(value, next);
      }
    });
  }
  if (target < 0) return -1;
  const int dx = target % g.width - start % g.width;
  const int dy = target / g.width - start / g.width;
  if (dx == 0 || dy == 0) {
    const int direction = dx > 0 ? 0 : dy > 0 ? 1 : dx < 0 ? 2 : 3;
    int next = start;
    for (int step = 0; step < int(std::ceil(.6 / g.cell)); ++step) {
      next = g.Next(next, direction);
      if (next < 0 || g.evidence[next] >= 0) break;
      if (step > stride && memory.searched.contains(g.Key(next))) break;
      if (candidate(next)) target = next;
    }
  }
  return target;
}

inline int ChooseFrontier(const MazeGrid& g, int start, const MazeMemory& memory) {
  if (!g.Open(start)) return -1;

  std::deque<int> queue{start};
  std::vector<int> distance(g.Count(), -1);
  distance[start] = 0;

  int goal = -1, best = std::numeric_limits<int>::max();

  while (!queue.empty()) {
    const int u = queue.front();
    queue.pop_front();

    const int score =
      distance[u] + 4 * std::min(100, g.visits[u]) - 2 * std::min(50, int(g.frontier[u]));

    if (g.frontier[u] && !memory.covered.contains(g.Key(u)) && FrontierAvailable(g, u, memory) &&
        score < best) {
      goal = u;
      best = score;
    }

    for (int d = 0; d < 4; ++d) {
      const int v = g.Next(u, d);
      if (v >= 0 && distance[v] < 0) {
        distance[v] = distance[u] + 1;
        queue.push_back(v);
      }
    }
  }

  return goal;
}

}  // namespace RoverLab

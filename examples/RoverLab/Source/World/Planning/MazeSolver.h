#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <numbers>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace RoverLab {
struct MazePoint {
  double x{}, y{};
};

using MazeKey = std::pair<int, int>;
using MazeEdge = std::pair<MazeKey, MazeKey>;

struct MazeGrid {
  int width{}, height{};
  double cell{}, x{}, y{};
  std::vector<std::uint8_t> open, clear, frontier;
  std::vector<std::int8_t> evidence;
  // Optional conservative swept-circle clearance for in-place route turns.
  std::vector<std::uint8_t> turnClear;
  // Exact swept-footprint checks at the current estimated pose can permit a
  // small heading correction where the generic full-turn circle cannot fit.
  int poseCell{-1};
  unsigned poseHeadings{};
  int stoppingCells{};
  std::vector<int> visits;
  std::set<MazeEdge> denied;

  int Count() const { return width * height; }

  bool Open(int i) const { return i >= 0 && i < Count() && open[i]; }

  int At(MazePoint p) const {
    const int c = int(std::floor((p.x - x) / cell)), r = int(std::floor((p.y - y) / cell));
    return c < 0 || r < 0 || c >= width || r >= height ? -1 : r * width + c;
  }

  MazePoint Point(int i) const {
    return {x + (i % width + .5) * cell, y + (i / width + .5) * cell};
  }

  // Half-cell coordinates stay stable when the discovered grid expands left/up.
  MazeKey Key(int i) const {
    const auto p = Point(i);
    return {int(std::llround(2 * p.x / cell)), int(std::llround(2 * p.y / cell))};
  }

  int Adjacent(int i, int d) const {
    constexpr int dx[]{1, 0, -1, 0}, dy[]{0, 1, 0, -1};
    const int c = i % width + dx[d], r = i / width + dy[d];
    if (c < 0 || r < 0 || c >= width || r >= height) return -1;
    return r * width + c;
  }

  int Next(int i, int d) const {
    const int j = Adjacent(i, d);
    if (!Open(j) || !(clear[i] & (1u << (d % 2))) || !(clear[j] & (1u << (d % 2)))) return -1;

    int end = j;
    for (int k = 0; k < stoppingCells; ++k) {
      end = Adjacent(end, d);
      if (end < 0 || !(clear[end] & (1u << (d % 2)))) return -1;
    }

    return Open(j) && !denied.contains({Key(i), Key(j)}) ? j : -1;
  }
};

struct MazeRoute {
  std::vector<int> cells, headings;
  double seconds{};
};

// Every planner uses the same (cell, chassis heading) graph. A translation can
// be forward or reverse; a change of chassis heading needs swept turn clearance.
struct MazeMotionGraph {
  const MazeGrid& grid;

  bool CanTurn(int cell) const {
    return grid.clear[cell] == 3 && (grid.turnClear.empty() || grid.turnClear[cell]);
  }

  std::vector<int> Starts(int cell, double heading) const {
    std::vector<int> states;
    if (!grid.Open(cell) || !std::isfinite(heading)) return states;

    constexpr double pi = std::numbers::pi;
    for (int d = 0; d < 4; ++d) {
      const double angle = std::abs(std::remainder(d * pi / 2 - heading, 2 * pi));
      const bool measured = cell == grid.poseCell && (grid.poseHeadings & (1u << d));
      if ((grid.clear[cell] & (1u << (d % 2))) && (angle <= .01 || CanTurn(cell) || measured))
        states.push_back(cell * 4 + d);
    }

    std::stable_sort(states.begin(), states.end(), [&](int a, int b) {
      return std::abs(std::remainder((a % 4) * pi / 2 - heading, 2 * pi)) <
             std::abs(std::remainder((b % 4) * pi / 2 - heading, 2 * pi));
    });
    return states;
  }

  int Next(int state, int direction, int facing) const {
    if (facing != direction && facing != (direction + 2) % 4) return -1;
    const int cell = state / 4;
    if (facing != state % 4 && !CanTurn(cell)) return -1;
    const int next = grid.Next(cell, direction);
    return next < 0 ? -1 : next * 4 + facing;
  }

  template <class F>
  void EachNext(int state, F&& visit) const {
    // Prefer continuing the current heading when equally good choices exist.
    for (int offset : {0, 1, 3, 2}) {
      const int d = (state % 4 + offset) % 4;
      for (int facing : {d, (d + 2) % 4}) {
        const int next = Next(state, d, facing);
        if (next >= 0) visit(next);
      }
    }
  }
};

class MazeSolver {
public:
  virtual ~MazeSolver() = default;
  virtual MazeRoute Plan(const MazeGrid&, int start, int goal, double heading) const = 0;

  // Preserve the cell-path API for callers that do not drive the rover.
  std::vector<int> Solve(const MazeGrid& grid, int start, int goal, int heading) const {
    return Plan(grid, start, goal, heading * std::numbers::pi / 2).cells;
  }

protected:
  static MazeRoute Path(const std::vector<int>& parent, int goal) {
    if (goal < 0 || parent[goal] < 0) return {};

    MazeRoute route;
    for (int state = goal;; state = parent[state]) {
      route.cells.push_back(state / 4);
      route.headings.push_back(state % 4);
      if (parent[state] == state) break;
    }

    std::reverse(route.cells.begin(), route.cells.end());
    std::reverse(route.headings.begin(), route.headings.end());
    return route;
  }
};

class SearchMazeSolver final : public MazeSolver {
public:
  enum class Kind { BFS, DFS, Dijkstra, AStar, FloodFill };
  explicit SearchMazeSolver(Kind kind) : kind(kind) {}

  MazeRoute Plan(const MazeGrid& g, int start, int goal, double heading) const override {
    if (!g.Open(start) || !g.Open(goal)) return {};
    const MazeMotionGraph motion{g};
    const auto roots = motion.Starts(start, heading);
    if (roots.empty()) return {};

    const int n = g.Count() * 4;
    std::vector<int> parent(n, -1);
    for (int root : roots)
      parent[root] = root;

    if (kind == Kind::FloodFill) {
      std::vector<int> distance(n, -1);
      std::deque<int> queue;
      for (int d = 0; d < 4; ++d)
        if (g.clear[goal] & (1u << (d % 2))) {
          const int state = goal * 4 + d;
          distance[state] = 0;
          queue.push_back(state);
        }

      while (!queue.empty()) {
        const int state = queue.front();
        queue.pop_front();
        const int cell = state / 4, facing = state % 4;
        // Reverse the exact directed motion graph, including reverse travel,
        // stopping clearance and rejected edges. Never assume edge symmetry.
        for (int d : {facing, (facing + 2) % 4}) {
          const int previous = g.Adjacent(cell, (d + 2) % 4);
          if (!g.Open(previous)) continue;
          for (int h = 0; h < 4; ++h) {
            const int node = previous * 4 + h;
            if (distance[node] < 0 && motion.Next(node, d, facing) == state) {
              distance[node] = distance[state] + 1;
              queue.push_back(node);
            }
          }
        }
      }

      int state = -1;
      for (int root : roots)
        if (distance[root] >= 0 && (state < 0 || distance[root] < distance[state])) state = root;
      if (state < 0) return {};

      MazeRoute result;
      for (;;) {
        result.cells.push_back(state / 4);
        result.headings.push_back(state % 4);
        if (state / 4 == goal) return result;
        int next = -1;
        motion.EachNext(state, [&](int node) {
          if (next < 0 && distance[node] == distance[state] - 1) next = node;
        });
        if (next < 0) return {};
        state = next;
      }
    }

    if (kind == Kind::BFS || kind == Kind::DFS) {
      std::deque<int> queue(roots.begin(), roots.end());
      if (kind == Kind::DFS) std::reverse(queue.begin(), queue.end());

      while (!queue.empty()) {
        const int state = kind == Kind::DFS ? queue.back() : queue.front();
        if (kind == Kind::DFS)
          queue.pop_back();
        else
          queue.pop_front();
        if (state / 4 == goal) return Path(parent, state);

        std::vector<int> next;
        motion.EachNext(state, [&](int node) { next.push_back(node); });
        if (kind == Kind::DFS) std::reverse(next.begin(), next.end());
        for (int node : next)
          if (parent[node] < 0) {
            parent[node] = state;
            queue.push_back(node);
          }
      }
      return {};
    }

    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    std::vector<int> cost(n, infinity);

    using Entry = std::pair<int, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;

    const auto heuristic = [&](int state) {
      const int cell = state / 4;
      return kind == Kind::AStar ? std::abs(cell % g.width - goal % g.width) +
                                     std::abs(cell / g.width - goal / g.width)
                                 : 0;
    };

    for (int root : roots) {
      cost[root] = 0;
      queue.emplace(heuristic(root), root);
    }

    while (!queue.empty()) {
      const auto [priority, state] = queue.top();
      queue.pop();
      if (priority != cost[state] + heuristic(state)) continue;
      if (state / 4 == goal) return Path(parent, state);

      motion.EachNext(state, [&](int node) {
        const int candidate = cost[state] + 1 + std::min(20, g.visits[node / 4]);
        if (candidate < cost[node]) {
          cost[node] = candidate;
          parent[node] = state;
          queue.emplace(candidate + heuristic(node), node);
        }
      });
    }
    return {};
  }

private:
  Kind kind;
};

class RightHandMazeSolver final : public MazeSolver {
public:
  MazeRoute Plan(const MazeGrid& g, int start, int goal, double heading) const override {
    if (!g.Open(start) || !g.Open(goal)) return {};
    auto route = Trace(g, start, heading, goal);
    return !route.cells.empty() && route.cells.back() == goal ? route : MazeRoute{};
  }

  // Score only observation targets on this finite wall-follow trace.
  MazeRoute Trace(const MazeGrid& g, int start, double heading, int goal = -1) const {
    const MazeMotionGraph motion{g};
    const auto roots = motion.Starts(start, heading);
    if (roots.empty()) return {};

    int state = roots.front();
    int travelHeading = state % 4;
    bool followingWall = false;
    std::vector<std::uint8_t> seen(g.Count() * 32);
    MazeRoute route;

    for (;;) {
      const int visit = (state * 4 + travelHeading) * 2 + int(followingWall);
      if (seen[visit]) return route;
      seen[visit] = 1;
      route.cells.push_back(state / 4);
      route.headings.push_back(state % 4);
      if (state / 4 == goal) return route;

      int next = -1;
      const auto step = [&](int d) {
        const int forward = motion.Next(state, d, d);
        return forward >= 0 ? forward : motion.Next(state, d, (d + 2) % 4);
      };

      // In an open room, blindly choosing right at every cell creates a tiny
      // four-cell circle. Acquire a boundary first, then keep it on the right.
      // Lack of turning room is not itself a wall on the right. Treating it
      // as one makes the first turnable patch trigger a four-cell circle.
      if (g.Next(state / 4, (travelHeading + 1) % 4) < 0) followingWall = true;
      const std::array<int, 4> order =
        followingWall ? std::array{1, 0, 3, 2} : std::array{0, 3, 1, 2};
      for (int turn : order) {
        const int d = (travelHeading + turn) % 4;
        // Prefer a forward wall-follow step. Back out without rotating if a
        // dead end has no space for a U-turn.
        next = step(d);
        if (next >= 0) {
          // After rounding an opening, acquire its next boundary before
          // taking another right. With a rectangular footprint, clearance
          // changes with heading; four legal right turns need not surround
          // a physical obstacle.
          if (turn != 0) followingWall = false;
          travelHeading = d;
          break;
        }
      }
      if (next < 0) return route;
      state = next;
    }
  }
};

inline std::unique_ptr<MazeSolver> MakeMazeSolver(const std::string& name) {
  using K = SearchMazeSolver::Kind;

  if (name == "RightHand") return std::make_unique<RightHandMazeSolver>();
  if (name == "BFS") return std::make_unique<SearchMazeSolver>(K::BFS);
  if (name == "DFS") return std::make_unique<SearchMazeSolver>(K::DFS);
  if (name == "Dijkstra") return std::make_unique<SearchMazeSolver>(K::Dijkstra);
  if (name == "FloodFill") return std::make_unique<SearchMazeSolver>(K::FloodFill);
  if (name == "AStar") return std::make_unique<SearchMazeSolver>(K::AStar);

  return {};
}

}  // namespace RoverLab

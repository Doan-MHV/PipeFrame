#pragma once
#include "Components/MicromouseMazeComponent.h"
#include "MazeSolver.h"

namespace RoverLab {
// Regular maze topology in starting-pose coordinates: start is cell (0,0),
// +X/+Y follow the initial robot frame. Bounds/pitch are authored; walls and goal
// are not. Unknown edges are never used as shortcuts on a learned solve.
struct CellMaze {
  enum Edge : unsigned char { Unknown, Open, Wall };
  MicromouseMazeComponent settings;
  std::vector<std::array<Edge, 4>> edges;
  std::vector<bool> visited;
  void Reset(const MicromouseMazeComponent& config) {
    settings = config;
    edges.assign(config.columns * config.rows, {});
    visited.assign(edges.size(), false);
  }
  int At(MazePoint p) const {
    const int x = int(std::llround(p.x / settings.cellPitch));
    const int y = int(std::llround(p.y / settings.cellPitch));
    return x < 0 || y < 0 || x >= settings.columns || y >= settings.rows ? -1
                                                                         : y * settings.columns + x;
  }
  MazePoint Centre(int cell) const {
    return {
      cell % settings.columns * settings.cellPitch, cell / settings.columns * settings.cellPitch};
  }
  int Neighbour(int cell, int d) const {
    constexpr int dx[]{1, 0, -1, 0}, dy[]{0, 1, 0, -1};
    const int x = cell % settings.columns + dx[d], y = cell / settings.columns + dy[d];
    return x < 0 || y < 0 || x >= settings.columns || y >= settings.rows ? -1
                                                                         : y * settings.columns + x;
  }
  Edge Classify(double distanceFromCentre, bool hasReturn) const {
    if (!std::isfinite(distanceFromCentre) || distanceFromCentre < 0) return Unknown;
    if (hasReturn && distanceFromCentre <= (settings.cellPitch - settings.wallThickness) / 2 + .02)
      return Wall;
    if (distanceFromCentre >= (settings.cellPitch + settings.wallThickness) / 2 + .015) return Open;
    // No return inside a short sensor range does not establish a boundary wall.
    return Unknown;
  }

  void Observe(int cell, int d, Edge value) {
    if (cell < 0 || cell >= int(edges.size()) || d < 0 || d >= 4) return;
    // A new obstacle can close an edge; clear observations must explicitly reopen it.
    edges[cell][d] = value;
    const int next = Neighbour(cell, d);
    if (next >= 0) edges[next][(d + 2) % 4] = value;
  }
  // Multi-source flood fill from all unvisited cells through measured open edges.
  // No goal coordinate is supplied during discovery; each reached cell is tested
  // by the independent arrival detector. Backtracking is inherent in the flood.
  int NextUnvisited(int start) const {
    std::vector<int> distance(edges.size(), -1);
    std::deque<int> queue;
    for (int i = 0; i < int(edges.size()); ++i)
      if (!visited[i]) {
        distance[i] = 0;
        queue.push_back(i);
      }
    while (!queue.empty()) {
      const int cell = queue.front();
      queue.pop_front();
      for (int d = 0; d < 4; ++d) {
        const int next = Neighbour(cell, d);
        if (next >= 0 && edges[cell][d] == Open && distance[next] < 0) {
          distance[next] = distance[cell] + 1;
          queue.push_back(next);
        }
      }
    }
    if (start < 0 || distance[start] <= 0) return -1;
    for (int d = 0; d < 4; ++d) {
      const int next = Neighbour(start, d);
      if (next >= 0 && edges[start][d] == Open && distance[next] == distance[start] - 1)
        return next;
    }
    return -1;
  }
  MazeGrid Grid() const {
    MazeGrid g;
    g.width = int(settings.columns);
    g.height = int(settings.rows);
    g.cell = settings.cellPitch;
    g.x = g.y = -g.cell / 2;
    g.open.assign(edges.size(), 0);
    g.clear.assign(edges.size(), 3);
    g.turnClear.assign(edges.size(), 1);
    g.visits.assign(edges.size(), 0);
    g.evidence.assign(edges.size(), 0);
    for (int i = 0; i < int(edges.size()); ++i) {
      g.open[i] = visited[i];
      for (int d = 0; d < 4; ++d) {
        const int next = Neighbour(i, d);
        if (next < 0) continue;
        if (edges[i][d] == Open) {
          g.open[i] = 1;
        } else
          g.denied.insert({g.Key(i), g.Key(next)});
      }
      if (g.open[i]) g.evidence[i] = -1;
    }
    return g;
  }
};
}  // namespace RoverLab

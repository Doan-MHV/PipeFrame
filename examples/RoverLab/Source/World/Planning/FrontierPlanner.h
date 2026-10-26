#pragma once

#include <numbers>

#include "MazeExploration.h"
#include "TimedRoute.h"

namespace RoverLab {

// Stable half-metre observation areas prevent adjacent cells from becoming
// hundreds of separate retries of the same unproductive boundary.
inline MazeKey ObservationArea(MazePoint point) {
  return {int(std::floor(point.x / .5)), int(std::floor(point.y / .5))};
}

inline std::uint64_t ObservationSignature(const MazeGrid& g, MazeKey area) {
  std::uint64_t hash = 1469598103934665603ull;
  const int count = std::max(1, int(std::ceil(.5 / g.cell)));
  for (int y = 0; y < count; ++y)
    for (int x = 0; x < count; ++x) {
      const int i =
        g.At({area.first * .5 + (x + .5) * g.cell, area.second * .5 + (y + .5) * g.cell});
      const int value = i >= 0 ? (g.evidence[i] > 0) - (g.evidence[i] < 0) : 0;
      hash = (hash ^ std::uint64_t(value + 2)) * 1099511628211ull;
    }
  return hash;
}

struct ExplorationChoice {
  int goal{-1};
  unsigned boundaryCells{}, reachableAreas{}, eligibleAreas{};
  double expectedArea{}, travelSeconds{};
};

// This is an optimistic visibility estimate, not knowledge of the hidden scene.
// Rays stop at known obstacles. Sonar requires a body survey for this 360-degree
// estimate, which is included in the observation cost by the caller.
inline double ExpectedNewArea(const MazeGrid& g, int index, double range) {
  std::set<int> unknown;
  const auto origin = g.Point(index);
  const int steps = std::max(1, int(std::ceil(std::min(range, 5.0) / g.cell)));
  for (int ray = 0; ray < 72; ++ray) {
    const double angle = ray * 2 * std::numbers::pi / 72;
    for (int step = 1; step <= steps; ++step) {
      const int cell = g.At(
        {origin.x + std::cos(angle) * step * g.cell, origin.y + std::sin(angle) * step * g.cell});
      if (cell < 0 || g.evidence[cell] > 0) break;
      if (g.evidence[cell] == 0) unknown.insert(cell);
    }
  }
  return unknown.size() * g.cell * g.cell;
}

inline ExplorationChoice ChooseObservationArea(const MazeGrid& g,
  int start,
  const MazeMemory& memory,
  double heading,
  double speed,
  double turnRate,
  double sensorRange,
  bool lidar,
  bool planTurns = false,
  const std::vector<double>* policyTravelTimes = nullptr) {
  ExplorationChoice result;
  // Count unresolved free/unknown boundaries before footprint inflation, so a
  // narrow or unreachable opening cannot be reported as a completed map.
  for (int i = 0; i < g.Count(); ++i) {
    if (g.evidence[i] >= 0) continue;
    for (int d = 0; d < 4; ++d) {
      const int adjacent = g.Adjacent(i, d);
      if (adjacent < 0 || g.evidence[adjacent] == 0) {
        ++result.boundaryCells;
        break;
      }
    }
  }
  if (!g.Open(start)) return result;

  const auto travelTimes = policyTravelTimes ? *policyTravelTimes
                           : planTurns       ? KnownTravelTimes(g, start, heading, speed, turnRate)
                                             : std::vector<double>{};

  std::vector<int> distance(g.Count(), -1), firstDirection(g.Count(), -1);
  std::deque<int> queue{start};
  distance[start] = 0;
  std::map<MazeKey, int> representatives;
  std::set<MazeKey> reachable;
  while (!queue.empty()) {
    const int u = queue.front();
    queue.pop_front();
    if (g.frontier[u] && (!planTurns || std::isfinite(travelTimes[u]))) {
      const auto area = ObservationArea(g.Point(u));
      reachable.insert(area);
      const auto it = representatives.find(area);
      if (FrontierAvailable(g, u, memory) &&
          (it == representatives.end() || g.frontier[u] > g.frontier[it->second]))
        representatives[area] = u;
    }
    for (int d = 0; d < 4; ++d) {
      const int next = g.Next(u, d);
      if (next < 0 || distance[next] >= 0) continue;
      distance[next] = distance[u] + 1;
      firstDirection[next] = u == start ? d : firstDirection[u];
      queue.push_back(next);
    }
  }
  result.reachableAreas = reachable.size();
  struct Candidate {
    int index;
    double seconds;
    unsigned attempts;
  };
  std::vector<Candidate> candidates;
  for (const auto& [area, index] : representatives) {
    const auto trial = memory.observationAreas.find(area);
    unsigned attempts = 0;
    if (trial != memory.observationAreas.end()) {
      if (memory.time < trial->second.retryAt) continue;
      const bool unchanged = trial->second.signature == ObservationSignature(g, area);
      if (unchanged && trial->second.count >= 3) continue;
      if (unchanged) attempts = trial->second.count;
    }
    if (!FrontierAvailable(g, index, memory)) continue;
    const double angle =
      firstDirection[index] < 0 ? heading : firstDirection[index] * std::numbers::pi / 2;
    const double turn = std::abs(std::remainder(angle - heading, 2 * std::numbers::pi));
    const double seconds =
      planTurns ? travelTimes[index] : distance[index] * g.cell / speed + turn / turnRate;
    candidates.push_back({index, seconds, attempts});
  }
  result.eligibleAreas = candidates.size();
  // Bound visibility work. Keep nearby candidates and candidates with broad
  // unknown boundaries, rather than raycasting every map cell each frame.
  std::stable_sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
    return (a.seconds + 1) / (1 + g.frontier[a.index]) <
           (b.seconds + 1) / (1 + g.frontier[b.index]);
  });
  if (candidates.size() > 64) candidates.resize(64);
  double best = -1;
  for (const auto& candidate : candidates) {
    const double gain = ExpectedNewArea(g, candidate.index, sensorRange);
    const double survey = lidar ? 1.0 : 2 * std::numbers::pi / turnRate + 1;
    const double utility = gain / ((candidate.seconds + survey) * (1 + candidate.attempts));
    if (gain > 0 && utility > best) {
      best = utility;
      result.goal = candidate.index;
      result.expectedArea = gain;
      result.travelSeconds = candidate.seconds;
    }
  }
  return result;
}

inline void RecordObservationAttempt(const MazeGrid& g, MazeMemory& memory, int goal) {
  const auto area = ObservationArea(g.Point(goal));
  auto& trial = memory.observationAreas[area];
  const auto signature = ObservationSignature(g, area);
  if (signature != trial.signature) trial.count = 0;
  trial.signature = signature;
  trial.count = std::min(3u, trial.count + 1);
  trial.retryAt = memory.time + 20 * trial.count;
}

}  // namespace RoverLab

#include <iostream>
#include <stdexcept>

#include "World/Simulation/RoverSolverSystem.h"

using namespace RoverLab;

void Check(bool value, const char* text) {
  if (!value) throw std::runtime_error(text);
}

void CheckMicromouseSearch() {
  MazeGrid g;
  g.width = g.height = 32;
  g.cell = .025;
  g.open.assign(g.Count(), 1);
  g.clear.assign(g.Count(), 3);
  g.turnClear.assign(g.Count(), 0);
  g.evidence.assign(g.Count(), -1);
  g.visits.assign(g.Count(), 0);
  MazeMemory memory;
  const int horizontal = ChooseMicromouseTarget(g, 0, memory, 0, .1, .12, 1);
  Check(horizontal >= 16 && horizontal < g.width,
    "Search stays on reachable heading and commits to a horizontal pass");
  const int vertical = ChooseMicromouseTarget(g, 0, memory, std::numbers::pi / 2, .1, .12, 1);
  Check(vertical % g.width == 0 && vertical / g.width >= 16,
    "Vertical corridors get the same long-pass treatment as horizontal ones");
  g.open[12 * g.width] = 0;
  const int stopped = ChooseMicromouseTarget(g, 0, memory, std::numbers::pi / 2, .1, .12, 1);
  Check(stopped >= 0 && stopped / g.width < 12, "Search pass cannot cross a blocked cell");
  for (int i = 0; i < g.Count(); ++i)
    memory.searched.insert(g.Key(i));
  Check(ChooseMicromouseTarget(g, 0, memory, 0, .1, .12, 1) < 0,
    "Already physically searched floor is not selected again");
}

void CheckRecoveredTrail() {
  RoverMission approach;
  approach.Advance(.1, {0, 0});
  approach.Advance(.1, {-.01, 0});
  approach.Advance(.1, {-.01, 0});
  Check(approach.lastMotionDelta.x < 0,
    "Stationary arrival retains the measured reverse approach direction");

  MeasuredRoute measured;
  measured.Record({1, 0}, 0, true);
  measured.Record({1, 0}, std::numbers::pi / 2, true);
  measured.Record({1, 1}, std::numbers::pi / 2, true);
  measured.Record({1, 1}, std::numbers::pi, true);
  measured.Record({0, 1}, std::numbers::pi, true);
  measured.Record({0, 1}, -std::numbers::pi / 2, true);
  measured.Record({0, 0}, -std::numbers::pi / 2, true);
  measured.CloseLoops();
  Check(measured.poses.size() >= 8,
    "Crossing the start at a different heading must not erase required turns");
  Check(measured.poses[1].position.x == measured.poses[2].position.x &&
          measured.poses[1].heading != measured.poses[2].heading,
    "Measured route keeps an in-place corner's before and after headings");
  measured.Record({0, 0}, 0, true);
  measured.CloseLoops();
  Check(measured.poses.size() == 2,
    "A measured loop can close when both start position and heading match");

  MeasuredRoute retreat;
  retreat.Record({1, 0}, 0, true);
  retreat.Record({.8, 0}, 0, true);
  retreat.Record({.8, 0}, std::numbers::pi / 2, true);
  retreat.CloseLoops();
  Check(retreat.poses.size() == 3 && retreat.poses[1].position.x == .8,
    "Backing onto an earlier segment removes the refused wall approach but keeps its turn");

  RoverMission small;
  small.trail = {{0, 0}, {.1, .015}, {.2, 0}};
  small.SimplifyTrail(.0035);
  Check(small.trail.size() == 3, "Small chassis trace keeps a 1.5 cm clearance bend");

  RoverMission mission;
  mission.trail = {{0, 0}, {1.85, 0}, {1.838, .4}, {1.83, .35}};
  mission.Advance(.1, {1.828, .29});
  mission.SimplifyTrail();

  for (const auto point : mission.trail)
    Check(point.y < .31,
      "A short reverse recovery removes the failed corner from the learned repeat trail");
  Check(mission.trail.front().x == 0 &&
          std::hypot(mission.trail.back().x - 1.828, mission.trail.back().y - .29) < .001,
    "Recovery trimming preserves the start and current measured pose");
}

void CheckDepartureClearance() {
  MazeGrid g;
  g.width = g.height = 60;
  g.cell = .05;
  g.x = g.y = -1.5;
  g.evidence.assign(g.Count(), -1);
  const std::set<MazeKey> covered;
  const MazePoint from{0, 0}, back{-.3, -.03};
  Check(KnownStraightClear(g, from, back, .1, .19, .13, covered),
    "Angled reverse departure accepts observed free swept footprint");
  const int obstacle = g.At({-.25, 0});
  g.evidence[obstacle] = 1;
  Check(!KnownStraightClear(g, from, back, .1, .19, .13, covered),
    "Departure never crosses positive obstacle evidence");
  g.evidence[obstacle] = 0;
  Check(!KnownStraightClear(g, from, back, .1, .19, .13, covered),
    "Departure cannot back into unobserved space");
  Check(KnownStraightClear(g, from, back, .1, .19, .13, {g.Key(obstacle)}),
    "Previously occupied chassis cells can bridge a known footprint raster gap");
  g.evidence[obstacle] = 1;
  Check(!KnownStraightClear(g, from, back, .1, .19, .13, {g.Key(obstacle)}),
    "Historical chassis coverage cannot override a new obstacle");

  g.evidence.assign(g.Count(), -1);
  Check(KnownTurnClear(g, from, 0, std::numbers::pi / 2, .19, .13, covered),
    "Complete turn fits in observed free space");
  const int corner = g.At({.21, .06});
  g.evidence[corner] = 1;
  Check(KnownStraightClear(g, from, from, 0, .19, .13, covered) &&
          KnownStraightClear(g, from, from, std::numbers::pi / 2, .19, .13, covered),
    "Both turn endpoints fit beside the wall corner");
  Check(!KnownTurnClear(g, from, 0, std::numbers::pi / 2, .19, .13, covered),
    "Preflight rejects a wall corner intersecting the middle of the turn");
  g.evidence[corner] = 0;
  Check(!KnownTurnClear(g, from, 0, std::numbers::pi / 2, .19, .13, covered),
    "An unknown turn sweep is not assumed clear");

  g.evidence.assign(g.Count(), 0);
  Check(!KnownTurnClear(g, from, 0, .07, .06, .045, covered),
    "Known-route alignment still refuses unknown footprint margins");
  Check(KnownTurnClear(g, from, 0, .07, .06, .045, covered, true),
    "Exploration can probe a small local alignment under live sensor safety");
  Check(!KnownTurnClear(g, from, 0, .2, .06, .045, covered, true),
    "Exploration probe does not authorize a large unknown turn");
  g.evidence[g.At({.05, 0})] = 1;
  Check(!KnownTurnClear(g, from, 0, .07, .06, .045, covered, true),
    "Even a small alignment probe refuses positive obstacle evidence");
}

void CheckTimedRoute() {
  MazeGrid corridor;
  corridor.width = 3;
  corridor.height = 1;
  corridor.cell = .05;
  corridor.open.assign(3, 1);
  corridor.clear.assign(3, 1);
  corridor.visits.assign(3, 0);
  corridor.turnClear.assign(3, 0);
  Check(FastestKnownRoute(corridor, 0, 2, .04, .1, 1).cells.empty(),
    "A full-turn circle alone cannot approve small alignment corrections");
  corridor.poseCell = 0;
  corridor.poseHeadings = 1;
  Check(!FastestKnownRoute(corridor, 0, 2, .04, .1, 1).cells.empty(),
    "A verified small alignment permits corridor travel without a full spin");
  Check(FastestKnownRoute(corridor, 1, 2, .04, .1, 1).cells.empty(),
    "Pose-specific alignment permission cannot be reused at another cell");

  MazeGrid grid;
  grid.width = 7;
  grid.height = 5;
  grid.cell = 1;
  grid.open.assign(35, 1);
  grid.clear.assign(35, 3);
  grid.visits.assign(35, 0);
  const std::vector<int> shortRoute{14, 15, 8, 9, 10, 11, 12, 19, 20};
  const std::vector<int> smoothRoute{14, 21, 28, 29, 30, 31, 32, 33, 34, 27, 20};
  for (int cell = 0; cell < grid.Count(); ++cell)
    for (int direction = 0; direction < 4; ++direction) {
      const int next = grid.Adjacent(cell, direction);
      if (next >= 0) grid.denied.insert({grid.Key(cell), grid.Key(next)});
    }
  for (const auto& route : {shortRoute, smoothRoute})
    for (std::size_t i = 1; i < route.size(); ++i) {
      grid.denied.erase({grid.Key(route[i - 1]), grid.Key(route[i])});
      grid.denied.erase({grid.Key(route[i]), grid.Key(route[i - 1])});
    }
  const auto shortest = MakeMazeSolver("BFS")->Solve(grid, 14, 20, 1);
  const auto fastest = FastestKnownRoute(grid, 14, 20, std::numbers::pi / 2, 1, .2);
  Check(shortest == shortRoute, "Distance-only solver selects shorter zigzag");
  Check(fastest.cells == smoothRoute && fastest.seconds > 10,
    "Time planner selects longer route with fewer slow turns");
  Check(
    std::abs(RouteTravelSeconds(fastest, std::numbers::pi / 2, 1, 1, .2) - fastest.seconds) < 1e-9,
    "Any selected algorithm can budget its actual route with the follower's turn costs");
  const auto reachable = KnownTravelTimes(grid, 14, std::numbers::pi / 2, 1, .2);
  Check(std::abs(reachable[20] - fastest.seconds) < 1e-9,
    "Exploration reachability uses the same turns and travel costs as the driven route");
  const auto quickTurns = FastestKnownRoute(grid, 14, 20, std::numbers::pi / 2, .1, 10);
  Check(quickTurns.cells == shortRoute, "Speed and turn rate change optimal route");
  grid.turnClear.assign(35, 1);
  grid.turnClear[28] = 0;
  Check(FastestKnownRoute(grid, 14, 20, std::numbers::pi / 2, 1, .2).cells == shortRoute,
    "A turn without swept clearance is rejected");
  grid.open[20] = 0;
  Check(!std::isfinite(KnownTravelTimes(grid, 14, 0, 1, 1)[20]),
    "Unreachable observation cells have no finite travel time");
  Check(FastestKnownRoute(grid, 14, 20, 0, 1, 1).cells.empty(), "Blocked destination has no route");
}

void CheckInformationPlanner() {
  MazeGrid g;
  g.width = 40;
  g.height = 20;
  g.cell = .1;
  g.evidence.assign(g.Count(), 0);
  g.open.assign(g.Count(), 1);
  g.clear.assign(g.Count(), 3);
  g.frontier.assign(g.Count(), 0);
  g.visits.assign(g.Count(), 0);
  // Equal local boundaries: the right observation overlooks a large unseen
  // room, while the left overlooks only a small pocket behind known walls.
  for (int y = 0; y < g.height; ++y)
    for (int x = 0; x < g.width; ++x)
      if (x < 24) g.evidence[y * g.width + x] = -1;
  const int near = g.At({1.05, 1.05});
  const int far = g.At({2.35, 1.05});
  g.evidence[g.At({.55, 1.05})] = 0;
  g.frontier[near] = g.frontier[far] = 1;
  MazeMemory memory;
  const auto choice = ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true);
  Check(choice.goal == far && choice.expectedArea > 1,
    "Information gain can justify travelling beyond a tiny nearby pocket");
  Check(choice.reachableAreas == 2, "Nearby frontier cells form stable observation areas");
  const int neighbour = far + g.width;
  g.frontier[neighbour] = 1;
  memory.attempted.insert(g.Key(far));
  Check(ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true).goal == neighbour,
    "A failed cell does not hide another available point in its observation area");
  memory.attempted.clear();
  for (int attempt = 0; attempt < 3; ++attempt) {
    RecordObservationAttempt(g, memory, far);
    memory.time += 61;
  }
  Check(ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true).goal == near,
    "Repeated unchanged observation area is suspended as a group");
  const auto area = ObservationArea(g.Point(far));
  const auto signature = ObservationSignature(g, area);
  MazeGrid expanded = g;
  expanded.width += 10;
  expanded.x -= 1;
  expanded.evidence.assign(expanded.Count(), 0);
  for (int y = 0; y < g.height; ++y)
    for (int x = 0; x < g.width; ++x)
      expanded.evidence[y * expanded.width + x + 10] = g.evidence[y * g.width + x];
  Check(ObservationSignature(expanded, area) == signature,
    "Grid expansion preserves observation signatures and retry history");
  g.evidence[far] = 1;
  Check(ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true).goal == far,
    "Changed occupancy reopens a suspended observation area");

  // A closed known wall must hide unknown space from the gain estimate.
  for (int y = 0; y < g.height; ++y)
    g.evidence[y * g.width + 24] = 1;
  Check(ExpectedNewArea(g, far, 1) == 0, "Known walls occlude estimated information gain");
  std::fill(g.evidence.begin(), g.evidence.end(), 1);
  for (int y = 1; y < g.height - 1; ++y)
    for (int x = 1; x < g.width - 1; ++x)
      g.evidence[y * g.width + x] = -1;
  std::fill(g.frontier.begin(), g.frontier.end(), 0);
  Check(ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true).boundaryCells == 0,
    "Fully observed enclosed room has no unresolved boundary");
  g.evidence[g.At({3.5, 1})] = 0;
  const auto blocked = ChooseObservationArea(g, near, memory, 0, .1, 1, 1, true);
  Check(blocked.goal < 0 && blocked.boundaryCells > 0,
    "Unknown space without a safe target is unresolved, not completed");
}

void CheckEveryAlgorithmTurns() {
  MazeGrid g;
  g.width = g.height = 5;
  g.cell = .1;
  g.open.assign(25, 0);
  g.clear.assign(25, 3);
  g.turnClear.assign(25, 1);
  g.visits.assign(25, 0);
  for (int cell : {6, 7, 8, 11, 13, 16, 17, 18})
    g.open[cell] = 1;
  g.turnClear[8] = 0;
  for (const char* name : {"BFS", "DFS", "Dijkstra", "AStar", "FloodFill", "RightHand"}) {
    const auto route = MakeMazeSolver(name)->Plan(g, 6, 13, 0);
    Check(!route.cells.empty() && route.cells.back() == 13,
      "Every algorithm finds the turnable detour");
    Check(
      route.cells.size() == route.headings.size(), "Every driven route preserves chassis heading");
    for (std::size_t i = 1; i < route.cells.size(); ++i) {
      const int from = route.cells[i - 1], to = route.cells[i];
      const int d = to - from == 1 ? 0 : to - from == g.width ? 1 : to - from == -1 ? 2 : 3;
      Check(g.Next(from, d) == to, "Footprint and directed-edge clearance apply to every route");
      Check(route.headings[i] == d || route.headings[i] == (d + 2) % 4,
        "Rover translates along its chassis axis");
      Check(route.headings[i] == route.headings[i - 1] || g.turnClear[from],
        "No algorithm turns in a cell without swept clearance");
    }

    MazeGrid narrow;
    narrow.width = 4;
    narrow.height = 1;
    narrow.cell = .1;
    narrow.open.assign(4, 1);
    narrow.clear.assign(4, 1);
    narrow.turnClear.assign(4, 0);
    narrow.visits.assign(4, 0);
    const auto reverse = MakeMazeSolver(name)->Plan(narrow, 2, 0, 0);
    Check(!reverse.cells.empty() && reverse.cells.back() == 0,
      "Every algorithm can back out without an impossible U-turn");
    for (int heading : reverse.headings)
      Check(heading == 0, "Reverse keeps the chassis aligned");
    Check(MakeMazeSolver(name)->Plan(narrow, 2, 0, .04).cells.empty(),
      "Unverified initial alignment cannot bypass turn clearance");
    narrow.poseCell = 2;
    narrow.poseHeadings = 1;
    Check(!MakeMazeSolver(name)->Plan(narrow, 2, 0, .04).cells.empty(),
      "Every algorithm accepts an explicitly verified small alignment");
    narrow.open[0] = 0;
    Check(MakeMazeSolver(name)->Plan(narrow, 2, 0, 0).cells.empty(),
      "Every algorithm rejects an occupied destination");
  }
}

int main() {
  CheckMicromouseSearch();
  CheckRecoveredTrail();
  CheckEveryAlgorithmTurns();
  CheckTimedRoute();
  CheckDepartureClearance();
  CheckInformationPlanner();
  // Exercise the inherited schema through the same serialization API as the editor.
  const auto schema = RoverSolverComponent::Schema();
  RoverSolverComponent config;
  config.rover.objectId = 2;
  config.algorithm = "FloodFill";

  RoverSolverComponent restored;
  std::string error;
  Check(schema.Apply(restored, schema.Serialize(config).properties, error) &&
          schema.BindOwner(restored, 2, error) && restored == config,
    "New component base preserves solver settings on save/load");

  Check(
    !schema.Apply(restored, {{"algorithm", std::string{"Unknown"}}}, error) && restored == config,
    "Unsupported algorithms are rejected without changing settings");
  Check(!schema.Apply(restored, {{"speed", 0.0}}, error), "Zero cruise speed is rejected");
  Check(!schema.Apply(restored, {{"noNewCellTimeout", 1.0}}, error), "Watchdog has a lower bound");
  Check(
    !schema.Apply(restored, {{"searchSpacing", 0.0}}, error), "Search spacing must be positive");

  MazeGrid g;
  g.width = 8;
  g.height = 6;
  g.cell = .1;
  g.open.assign(48, 1);
  g.clear.assign(48, 3);
  g.frontier.assign(48, 0);
  g.visits.assign(48, 0);
  for (int y = 0; y < 5; ++y)
    g.open[y * 8 + 3] = 0;

  for (const char* name : {"BFS", "DFS", "Dijkstra", "AStar", "FloodFill", "RightHand"}) {
    auto solver = MakeMazeSolver(name);
    auto path = solver->Solve(g, 0, 7, 0);
    Check(!path.empty() && path.front() == 0 && path.back() == 7,
      "Algorithm must find corridor detour");

    if (std::string(name) != "DFS" && std::string(name) != "RightHand")
      Check(path.size() == 18, "Unweighted planners find shortest detour");

    for (std::size_t i = 1; i < path.size(); ++i) {
      bool adjacent = false;
      for (int d = 0; d < 4; ++d)
        adjacent |= g.Next(path[i - 1], d) == path[i];
      Check(adjacent, "Route contains only legal edges");
    }
  }

  g.denied.insert({g.Key(0), g.Key(1)});

  for (const char* name : {"BFS", "Dijkstra", "AStar", "FloodFill"}) {
    auto path = MakeMazeSolver(name)->Solve(g, 0, 7, 0);
    Check(!path.empty() && path[1] != 1, "Respect directed failed edge");
  }

  g.open[7] = 0;

  for (const char* name : {"BFS", "DFS", "Dijkstra", "AStar", "FloodFill", "RightHand"})
    Check(MakeMazeSolver(name)->Solve(g, 0, 7, 0).empty(), "Blocked target fails explicitly");

  g.open[7] = 1;

  MazeGrid loop;
  loop.width = loop.height = 3;
  loop.cell = 1;
  loop.open.assign(9, 1);
  loop.clear.assign(9, 3);
  loop.visits.assign(9, 0);
  Check(MakeMazeSolver("RightHand")->Solve(loop, 0, 4, 0).empty(),
    "Wall-follow detects its perimeter loop");

  Check(
    !MakeMazeSolver("BFS")->Solve(loop, 0, 4, 0).empty(), "Search can leave a wall-follow loop");

  loop.visits[1] = 20;

  for (const char* name : {"Dijkstra", "AStar"}) {
    const auto path = MakeMazeSolver(name)->Solve(loop, 0, 2, 0);
    Check(path.size() == 5 && path[1] == 3,
      "Weighted planners prefer a longer route over repeatedly visited cells");
  }

  // A narrow corridor can fit the rectangle lengthwise while refusing a sideways route.
  RoverMappingSystem::Map map;
  map.width = map.height = 21;
  map.settings.cellSize = .05;
  map.originX = map.originY = -.525;
  map.cells.assign(21 * 21, -1);
  map.pose.x = map.pose.y = map.pose.heading = 0;
  for (int x = 0; x < map.width; ++x) {
    map.cells[7 * map.width + x] = 12;
    map.cells[13 * map.width + x] = 12;
  }

  MazeMemory footprint;
  auto narrow = BuildMazeGrid(map, footprint, .16, .09, .03, 0, .15, .10);
  const int centre = narrow.At({0, 0});
  Check(narrow.Next(centre, 0) >= 0 && narrow.Next(centre, 1) < 0,
    "Rectangular clearance permits a fitting corridor without permitting sideways motion");

  map.cells.assign(21 * 21, 0);
  map.cells[centre] = 12;
  footprint = {};
  auto unknown = BuildMazeGrid(map, footprint, .16, .09, .03, 0, .15, .10);
  Check(!unknown.Open(centre) && !footprint.covered.contains(unknown.Key(centre)),
    "Chassis coverage never overwrites positive obstacle evidence");
  Check(!unknown.Open(unknown.At({.45, .45})), "Distant unknown cells are not traversable");
  Check(map.cells[centre] == 12 && map.cells.back() == 0,
    "Planning does not rewrite the discovered map");

  map.cells.assign(map.cells.size(), -1);
  const auto known = BuildMazeGrid(map, footprint, .16, .09, .03, 0, .15, .10, false, true);
  Check(known.turnClear[centre] && !known.Open(0),
    "Known route keeps body and turns inside grid bounds");
  map.cells.assign(map.cells.size(), 0);
  for (int x = 0; x < map.width; ++x)
    map.cells[10 * map.width + x] = -12;
  footprint = {};
  const auto thin = BuildMazeGrid(map, footprint, .16, .09, .03, 0, .15, .10, false, true);
  Check(!thin.Open(thin.At({.3, 0})),
    "A known centreline is insufficient when the body overlaps unknown space");

  MazeMemory memory;
  g.frontier[0] = g.frontier[7] = 1;
  Check(ChooseFrontier(g, 0, memory) == 0, "Nearest frontier first");
  memory.attempted.insert(g.Key(0));
  Check(ChooseFrontier(g, 0, memory) == 7, "Remember surveyed frontier");
  const auto old = g.Key(7);
  g.x -= 6.4;
  g.width += 64;
  Check(g.Key(71) == old, "Expansion preserves stable keys");

  std::cout << "RL7 planner checks passed\n";
}

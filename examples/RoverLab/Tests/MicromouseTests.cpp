#include <iostream>
#include <stdexcept>

#include "World/Planning/CellMaze.h"
#include "World/Planning/TimedRoute.h"
#include "World/Simulation/InfraredSensorSystem.h"
#include "World/Simulation/RoverDriveSystem.h"
#include "World/Simulation/RoverMappingSystem.h"

using namespace pipeframe;
using namespace RoverLab;

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
struct Wall final : EnvironmentQueries {
  float x{.4f};
  int rays{};
  std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin,
    Vector2f direction,
    float maximum,
    std::uint32_t,
    std::uint64_t excluded) override {
    ++rays;
    Check(excluded == 10, "Infrared excludes its own rover collider");
    if (std::abs(direction.x) < 1e-6) return {};
    const float distance = (x - origin.x) / direction.x;
    if (distance < 0 || distance > maximum) return {};
    TilemapQueryHit result;
    result.distance = distance;
    result.point = origin + direction * distance;
    return result;
  }
  std::vector<TilemapQueryHit> OverlapEnvironment(Circle2D, std::uint32_t) override { return {}; }
  bool HasEnvironmentClearance(Circle2D, std::uint32_t) override { return true; }
  std::optional<TilemapQueryHit> SweepEnvironment(
    Circle2D, Vector2f, std::uint32_t, std::uint64_t, bool) override {
    return {};
  }
};

void CellPlanning() {
  MicromouseMazeComponent config;
  config.columns = config.rows = 3;
  CellMaze maze;
  maze.Reset(config);
  Check(maze.Classify(.1, false) == CellMaze::Unknown && maze.Classify(.1, true) == CellMaze::Wall,
    "A short-range no-return cannot invent a cell wall");
  Check(maze.Classify(.4, false) == CellMaze::Open && maze.Classify(.19, true) == CellMaze::Unknown,
    "Only a range beyond the boundary proves open; ambiguous returns stay unknown");
  maze.visited[0] = true;
  Check(maze.NextUnvisited(0) == -1, "Unknown walls cannot be used for exploration shortcuts");
  maze.Observe(0, 0, CellMaze::Open);
  Check(maze.NextUnvisited(0) == 1 && maze.edges[1][2] == CellMaze::Open,
    "Flood fill chooses an observed open edge and stores reciprocal walls");
  maze.visited[1] = true;
  maze.Observe(1, 1, CellMaze::Open);
  Check(
    maze.NextUnvisited(0) == 1, "Flood fill backtracks through visited cells to unvisited floor");
  maze.Observe(1, 1, CellMaze::Wall);
  Check(maze.NextUnvisited(0) == -1, "New wall removes a previously open exploration edge");
  maze.Observe(1, 1, CellMaze::Open);
  maze.visited[4] = true;
  maze.Observe(4, 0, CellMaze::Open);
  maze.visited[5] = true;
  auto grid = maze.Grid();
  auto winding = FastestKnownRoute(grid, 0, 5, 0, .12, 1);
  Check(winding.cells == std::vector<int>({0, 1, 4, 5}), "Solve uses only learned edges");
  // A new observed shortcut is not part of the earlier winding route.
  maze.Observe(1, 0, CellMaze::Open);
  maze.Observe(2, 1, CellMaze::Open);
  grid = maze.Grid();
  const auto fast = FastestKnownRoute(grid, 0, 5, 0, .12, 1);
  Check(fast.cells == std::vector<int>({0, 1, 2, 5}) && fast.seconds < winding.seconds,
    "Fresh solve chooses fewer turns at equal distance rather than replaying an old route");
  Check(std::abs(fast.seconds - (3 * .35 / .12 + RouteTurnSeconds(std::numbers::pi / 2, 1))) < 1e-8,
    "Planner cost includes translation and physical heading changes");
  maze.Observe(2, 1, CellMaze::Wall);
  Check(FastestKnownRoute(maze.Grid(), 0, 5, 0, .12, 1).cells == winding.cells,
    "A blocked shortcut replans through other observed open cells");
  maze.Reset(config);
  Check(!maze.visited[0] && maze.edges[0][0] == CellMaze::Unknown, "Reset removes learned walls");
}

void OpticalSensing() {
  BehaviourScene scene;
  EventChannels events;
  Wall wall;
  scene.Services().Provide<EnvironmentQueries>(wall);
  const auto rover = scene.CreateObject();
  rover.AddComponent<Transform2DComponent>();
  rover.AddComponent<RoverMappingComponent>();
  rover.AddComponent<RoverDriveComponent>().stopDistance = .025;
  auto& safety = rover.AddComponent<RoverSafetyComponent>();
  safety.halfLength = .05;
  safety.halfWidth = .035;
  safety.clearance = .01;
  rover.AddComponent<KinematicBody2DComponent>();
  rover.AddComponent<EnvironmentCollider2DComponent>().size = {.1f, .07f};
  std::vector<SceneObject> sensors;
  for (int d = 0; d < 4; ++d) {
    auto sensor = scene.CreateObject();
    sensor.AddComponent<Transform2DComponent>();
    auto& config = sensor.AddComponent<InfraredSensorComponent>();
    config.parent.objectId = 10;
    config.localAngleDegrees = d * 90;
    sensors.push_back(sensor);
  }
  const auto resolve = [&](SceneObjectId id) {
    return id == 10 ? rover : SceneObject{};
  };
  InfraredSensorSystem infrared(scene, events);
  infrared.SetObjectResolver(resolve);
  RoverMappingSystem mapping(scene, events);
  mapping.SetObjectResolver(resolve);
  RoverDriveSystem drive(scene, events);
  drive.SetObjectResolver(resolve);
  std::vector<SensorReadingEvent> readings;
  auto subscription =
    events.Listen<SensorReadingEvent>([&](const auto& event) { readings.push_back(event); });
  double time = 0;
  const auto tick = [&](float dt) {
    time += dt;
    RoverOdometryEvent pose;
    pose.rover = rover;
    pose.simulationTime = time;
    events.Publish(pose);
    infrared.Update(dt);
  };
  infrared.AlignMounts();
  Check(wall.rays == 0, "Mount alignment does not sample in edit mode");
  tick(.01f);
  Check(readings.size() == 4, "Each optical sensor delivers immediately without sonar scheduling");
  const auto first = *infrared.Latest(sensors.front().GetEntity());
  Check(
    first.valid && first.status == SensorReadingStatus::Hit && std::abs(first.distance - .4) < 1e-5,
    "Infrared measures the analytic wall");
  Check(first.capturedAt == first.simulationTime,
    "Optical reading has capture timestamp and no acoustic delay");
  const auto accepted = mapping.Find(rover.GetEntity())->accepted;
  Check(accepted == 4, "Mapping accepts infrared-only observations");
  events.Publish(first);
  Check(mapping.Find(rover.GetEntity())->accepted == accepted,
    "Duplicate optical readings do not add map evidence");
  tick(.005f);
  Check(readings.size() == 5,
    "Sampling cadence does not resample early (one explicitly republished reading)");
  const int before = wall.rays;
  infrared.Update(0);
  Check(wall.rays == before, "Pause freezes infrared sampling");
  RoverMotionCommandEvent command;
  command.roverId = 10;
  command.forwardSpeed = .1;
  events.Publish(command);
  drive.Update(.01f);
  Check(
    rover.GetComponent<KinematicBody2DComponent>()->velocity.x > 0, "Infrared-only drive can move");
  wall.x = .06f;
  tick(.03f);
  events.Publish(command);
  drive.Update(.03f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Optical front wall stops travel");
  wall.x = -.06f;
  tick(.03f);
  command.forwardSpeed = -.1;
  events.Publish(command);
  drive.Update(.03f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Rear infrared protects reversing");
  wall.x = 1;
  tick(.03f);
  command.forwardSpeed = .1;
  events.Publish(command);
  drive.Update(.03f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x > 0,
    "Out-of-range optical reading allows clear travel");
  events.Publish(command);
  drive.Update(.4f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Stale optical data cannot authorize travel");
  sensors.front().GetComponent<InfraredSensorComponent>()->enabled = false;
  tick(.03f);
  Check(!infrared.Latest(sensors.front().GetEntity())->valid,
    "Disabled sensor invalidates its last reading");
  auto& config = *sensors.front().GetComponent<InfraredSensorComponent>();
  config.enabled = true;
  config.noiseAmplitude = .02;
  wall.x = .3f;
  tick(.03f);
  Check(std::abs(infrared.Latest(sensors.front().GetEntity())->distance - .3) <= .02001,
    "Optical noise is bounded");
  infrared.Reset();
  readings.clear();
  for (int i = 0; i < 60; ++i)
    infrared.Update(1.0f / 60);
  Check(readings.size() >= 200 && readings.size() <= 204,
    "A 50 Hz optical schedule retains cadence on a 60 Hz simulation clock");
  config.parent.objectId = 99;
  tick(.03f);
  Check(!infrared.Latest(sensors.front().GetEntity())->valid,
    "Missing parent invalidates optical sample");
}
}  // namespace
int main() {
  CellPlanning();
  OpticalSensing();
  std::cout << "Micromouse cell planning and infrared tests passed\n";
}

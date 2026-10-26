#include <cstdlib>
#include <iostream>
#include <limits>

#include "Editor/RoverMapCamera.h"
#include "World/Simulation/RoverMappingSystem.h"

void Require(bool value, const char* message) {
  if (!value) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

int main() {
  using namespace pipeframe;
  using namespace RoverLab;

  BehaviourScene scene;
  EventChannels events;
  RoverMappingSystem mapping(scene, events);

  auto rover = scene.CreateObject();
  rover.AddComponent<RoverMappingComponent>();

  auto sensor = scene.CreateObject();
  auto& mount = sensor.AddComponent<UltrasonicSensorComponent>();

  mount.parent.objectId = 2;
  mount.localPosition = {0.15f, 0.05f};

  mapping.SetObjectResolver([&](SceneObjectId id) { return id == 2 ? rover : SceneObject{}; });

  auto pose = [&](double time, double x, double heading = 0, double y = 0) {
    RoverOdometryEvent event;
    event.rover = rover;
    event.simulationTime = time;
    event.x = x;
    event.y = y;
    event.heading = heading;

    events.Publish(event);
  };

  auto reading = [&](double capture, double delivery, SensorReadingStatus status) {
    SensorReadingEvent event;
    event.sensor = sensor.GetEntity();
    event.roverId = 2;
    event.capturedAt = capture;
    event.simulationTime = delivery;
    event.valid = true;
    event.distance = 1;
    event.status = status;

    // Poison visualization geometry: mapping must ignore it.
    event.origin = {999, 999};
    event.endpoints.fill({999, 999});
    event.hits.fill(true);
    event.nearestRay = 8;

    events.Publish(event);
  };

  auto score = [&](double x, double y) {
    const auto* map = mapping.Find(rover.GetEntity());
    Require(map != nullptr, "Map exists");

    const int index = map->Index(x, y);
    Require(index >= 0, "Test point inside map");

    return int(map->cells[std::size_t(index)]);
  };

  // Exact capture pose remains usable across an odometry gap.
  pose(0, 0);
  pose(1, 1);
  pose(1.1, 1);

  reading(1, 1.1, SensorReadingStatus::Hit);

  Require(score(2.15, 0.05) > 0, "Mount offset + capture pose determine hit arc");

  Require(score(1.55, 0.05) < 0, "Free evidence ahead of mount");

  auto before = mapping.Find(rover.GetEntity())->cells;

  reading(1, 1.1, SensorReadingStatus::Hit);

  Require(mapping.Find(rover.GetEntity())->cells == before, "Duplicate is not reused");

  pose(1.2, 1);
  reading(1.2, 1.2, SensorReadingStatus::Dropped);

  Require(mapping.Find(rover.GetEntity())->cells == before, "Dropout does not clear map");

  pose(1.3, 1);
  reading(1.3, 1.3, SensorReadingStatus::BelowMinimumRange);

  Require(mapping.Find(rover.GetEntity())->cells == before, "Too-close reading is not free space");

  reading(std::numeric_limits<double>::quiet_NaN(), 1.3, SensorReadingStatus::Hit);

  Require(mapping.Find(rover.GetEntity())->cells == before, "Invalid timestamp is ignored");

  // Interpolate capture pose instead of using delivery pose.
  mapping.Reset();

  pose(0, 0);
  pose(0.2, 0.4);

  reading(0.1, 0.2, SensorReadingStatus::Hit);

  Require(score(1.35, 0.05) > 0, "Delayed hit uses interpolated capture position");

  Require(score(1.55, 0.05) == 0, "No hit added at delivery position");

  // Refuse to interpolate across a large missing-data interval.
  mapping.Reset();

  pose(0, 0);
  pose(1, 2);

  reading(0.5, 1, SensorReadingStatus::Hit);

  Require(mapping.Find(rover.GetEntity())->accepted == 0, "Missing odometry gap rejected");

  // Reject excessively old observations.
  mapping.Reset();

  pose(0, 0);
  pose(3, 0);

  reading(0, 3, SensorReadingStatus::Hit);

  Require(mapping.Find(rover.GetEntity())->accepted == 0, "Expired reading rejected");

  // Heading interpolation takes the short path across +/-pi.
  mapping.Reset();

  sensor.GetComponent<UltrasonicSensorComponent>()->localPosition = {0, 0.05f};

  pose(0, 0, 179 * std::numbers::pi / 180);
  pose(0.2, 0, -179 * std::numbers::pi / 180);

  reading(0.1, 0.2, SensorReadingStatus::Hit);

  Require(score(-0.95, -0.05) > 0, "Wrapped heading points left");

  Require(score(1.05, -0.05) == 0, "Wrapped heading never points right");

  // Repeated measurements strengthen evidence.
  mapping.Reset();

  sensor.GetComponent<UltrasonicSensorComponent>()->localPosition = {0.05f, 0.05f};

  for (int i = 0; i < 3; ++i) {
    pose(i * 0.1, 0);

    reading(i * 0.1, i * 0.1, SensorReadingStatus::Hit);
  }

  Require(score(1.05, 0.05) >= 3, "Repeated hits strengthen occupied evidence");

  // OutOfRange can add free evidence but cannot invent a wall.
  mapping.Reset();
  pose(0, 0);

  reading(0, 0, SensorReadingStatus::OutOfRange);

  for (auto value : mapping.Find(rover.GetEntity())->cells) {
    Require(value <= 0, "No-return never invents a wall");
  }

  // Changing grid dimensions discards incompatible cells.
  rover.GetComponent<RoverMappingComponent>()->sideCells = 32;
  pose(0.1, 0);

  Require(mapping.Find(rover.GetEntity())->cells.size() == 32 * 32, "Resize applied");

  Require(mapping.Find(rover.GetEntity())->accepted == 0, "Resize clears old evidence");

  // Cross every original grid edge; preserve evidence at its world coordinates.
  rover.GetComponent<RoverMappingComponent>()->sideCells = 160;
  mapping.Reset();
  pose(0, 0);
  reading(0, 0, SensorReadingStatus::Hit);
  const int originalEvidence = score(1.05, 0.05);
  Require(originalEvidence > 0, "Initial occupied evidence exists");
  const pipeframe::Vector2f positions[] = {{0, 8.31f}, {20, 0}, {-20, 0}, {0, -20}};
  double time = 1;
  for (const auto position : positions) {
    pose(time, position.x, 0, position.y);
    reading(time, time, SensorReadingStatus::Hit);
    const auto* grown = mapping.Find(rover.GetEntity());
    Require(grown->Index(position.x, position.y) >= 0, "Rover stays inside growing map");
    Require(score(position.x + 1.05, position.y + 0.05) > 0,
      "Sensor evidence recorded outside original grid");
    Require(score(1.05, 0.05) == originalEvidence, "Expansion preserves old world-space evidence");
    time += 1;
  }
  const auto* grown = mapping.Find(rover.GetEntity());
  Require(grown->Width() > 16 && grown->Height() > 16, "Both dimensions expand");
  const auto retainedCells = grown->cells;
  pose(time, 1e100);
  reading(time, time, SensorReadingStatus::Hit);
  Require(mapping.Find(rover.GetEntity())->storageLimited, "Extreme jumps respect memory bound");
  Require(mapping.Find(rover.GetEntity())->cells == retainedCells,
    "Allocation refusal preserves existing map");
  pose(time + 1, 0);
  reading(time + 1, time + 1, SensorReadingStatus::Hit);
  Require(
    !mapping.Find(rover.GetEntity())->storageLimited, "Mapping recovers inside retained bounds");

  RoverMapCamera camera;
  grown = mapping.Find(rover.GetEntity());
  camera.Track(*grown);
  Require(
    camera.following && camera.view.center.x == float(grown->pose.x), "Default follows rover");
  auto manual = camera.view;
  manual.Pan({0.25f, -0.25f});
  camera.Change(manual, true);
  pose(time + 2, 2, 0, 3);
  grown = mapping.Find(rover.GetEntity());
  camera.Track(*grown);
  Require(
    !camera.following && camera.view.center == manual.center, "Pan disables follow across updates");
  camera.following = true;
  camera.Track(*grown);
  Require(camera.view.center == pipeframe::Vector2f{2, 3}, "Follow resumes at latest pose");
  camera.view.Zoom(2);
  const auto span = camera.view.span;
  pose(time + 3, 3, 0, 4);
  grown = mapping.Find(rover.GetEntity());
  camera.Track(*grown);
  Require(camera.view.center == pipeframe::Vector2f{3, 4} && camera.view.span == span,
    "Following preserves zoom");
  camera.Fit(*grown);
  Require(!camera.following && camera.view.span > 40,
    "Fit includes explored cells across all expansions");
  const auto oldGeneration = grown->generation;
  mapping.Reset();
  pose(0, 0);
  grown = mapping.Find(rover.GetEntity());
  camera.Track(*grown);
  Require(grown->generation != oldGeneration && camera.following && camera.view.span == 8,
    "Reset restores default follow camera and fresh map generation");
  Require(
    grown->cells.size() == 160 * 160 && grown->accepted == 0, "Reset restores initial storage");

  // Separate rover, separate map.
  auto second = scene.CreateObject();
  second.AddComponent<RoverMappingComponent>();

  RoverOdometryEvent other;
  other.rover = second;
  other.simulationTime = 0.1;

  events.Publish(other);

  Require(mapping.Entities().size() == 2, "Separate maps for separate rovers");

  Require(mapping.Find(second.GetEntity())->accepted == 0, "No cross-rover evidence");

  const auto id = rover.GetEntity();

  rover.Destroy();
  mapping.Update(0.01f);

  Require(mapping.Find(id) == nullptr, "Destroyed rover removed");

  mapping.Reset();

  Require(mapping.Entities().empty(), "Reset clears all maps and history");

  std::cout << "Rover mapping checks passed.\n";
}

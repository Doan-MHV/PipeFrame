#include <iostream>
#include <stdexcept>

#include "World/Simulation/LidarSensorSystem.h"
#include "World/Simulation/RoverDriveSystem.h"
#include "World/Simulation/RoverMappingSystem.h"
using namespace pipeframe;
using namespace RoverLab;
namespace {
void Check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
bool Near(double a, double b) {
  return std::abs(a - b) < 1e-5;
}
struct Wall final : EnvironmentQueries {
  float x{1};
  int rays{};
  std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin,
    Vector2f direction,
    float maximum,
    std::uint32_t,
    std::uint64_t excluded) override {
    ++rays;
    Check(excluded == 10, "LiDAR must exclude its rover collider");
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
}  // namespace
void CheckSafetyBeamFrames() {
  BehaviourScene scene;
  EventChannels events;
  auto rover = scene.CreateObject();
  rover.AddComponent<RoverDriveComponent>();
  auto sensor = scene.CreateObject();
  auto& config = sensor.AddComponent<LidarSensorComponent>();
  config.parent.objectId = 10;
  RoverRangeObservations observations(scene, events);
  observations.SetObjectResolver(
    [&](SceneObjectId id) { return id == 10 ? rover : SceneObject{}; });
  RoverOdometryEvent pose;
  pose.rover = rover;
  pose.simulationTime = 1;
  events.Publish(pose);
  LidarScanEvent scan;
  scan.sensor = sensor;
  scan.roverId = 10;
  scan.settings = config;
  scan.simulationTime = 1;
  scan.valid = true;
  for (int i = 0; i < config.samplesPerScan; ++i)
    scan.beams.push_back({2 * std::numbers::pi * i / config.samplesPerScan,
      1,
      i == 180 ? .5 : config.maximumRange,
      i == 180 ? SensorReadingStatus::Hit : SensorReadingStatus::OutOfRange});
  events.Publish(scan);
  pose.simulationTime = 1.1;
  pose.x = -.1;
  pose.heading = std::numbers::pi / 2;
  events.Publish(pose);
  observations.Update(.1f);
  int count = 0;
  observations.Each(
    [&](const RangeObservation& value) {
      ++count;
      if (value.status != SensorReadingStatus::Hit) return;
      const double angle = value.settings.localAngleDegrees * std::numbers::pi / 180;
      const double x = value.settings.localPosition.x + std::cos(angle) * value.distance;
      const double y = value.settings.localPosition.y + std::sin(angle) * value.distance;
      Check(std::abs(x) < 1e-5 && std::abs(y - .4) < 1e-5,
        "Safety preserves the measured obstacle when rover translates and rotates after capture");
      Check(value.age >= .099 && value.settings.coneAngleDegrees == 1,
        "Measured beam retains its age and angular sampling uncertainty");
    },
    true);
  Check(count == 360, "Safety receives measured bearings instead of twelve broad sectors");
  observations.Reset();
  events.Publish(scan);
  count = 0;
  observations.Each([&](const RangeObservation&) { ++count; }, true);
  Check(count == 12, "Missing motion history falls back to conservative sector safety");
}

int main() {
  CheckSafetyBeamFrames();
  BehaviourScene scene;
  EventChannels events;
  Wall wall;
  scene.Services().Provide<EnvironmentQueries>(wall);
  auto rover = scene.CreateObject();
  rover.AddComponent<Transform2DComponent>();
  rover.AddComponent<RoverMappingComponent>();
  rover.AddComponent<RoverDriveComponent>();
  rover.AddComponent<RoverSafetyComponent>();
  rover.AddComponent<KinematicBody2DComponent>();
  auto& collider = rover.AddComponent<EnvironmentCollider2DComponent>();
  collider.size = {.3f, .2f};
  auto sensor = scene.CreateObject();
  sensor.AddComponent<Transform2DComponent>();
  auto& config = sensor.AddComponent<LidarSensorComponent>();
  config.parent.objectId = 10;
  const auto resolver = [&](SceneObjectId id) {
    return id == 10 ? rover : SceneObject{};
  };
  LidarSensorSystem lidar(scene, events);
  lidar.SetObjectResolver(resolver);
  RoverMappingSystem mapping(scene, events);
  mapping.SetObjectResolver(resolver);
  RoverDriveSystem drive(scene, events);
  drive.SetObjectResolver(resolver);
  std::vector<LidarScanEvent> scans;
  auto subscription =
    events.Listen<LidarScanEvent>([&](const auto& event) { scans.push_back(event); });
  auto tick = [&](float dt) {
    RoverOdometryEvent pose;
    pose.rover = rover;
    pose.simulationTime = lidar.SimulationTime() + dt;
    events.Publish(pose);
    lidar.Update(dt);
  };
  lidar.AlignMounts();
  Check(wall.rays == 0, "Editor mount alignment must not scan");
  for (int i = 0; i < 5; ++i)
    tick(.01f);
  Check(scans.empty(), "A rotating scan takes a complete revolution");
  for (int i = 0; i < 6; ++i)
    tick(.01f);
  Check(scans.size() == 1 && scans.back().beams.size() == 360, "One complete 360 beam scan");
  auto first = scans.back();
  Check(first.beams.front().capturedAt < first.beams.back().capturedAt,
    "Scan spans simulation timestamps");
  Check(first.beams[0].status == SensorReadingStatus::Hit && Near(first.beams[0].distance, 1),
    "Front laser measures analytic wall");
  Check(first.beams[180].status == SensorReadingStatus::OutOfRange, "Rear laser sees open space");
  auto* map = mapping.Find(rover.GetEntity());
  Check(map && map->accepted == 360, "Mapper integrates every beam");
  Check(map->cells[map->Index(.5, 0)] < 0 && map->cells[map->Index(1, 0)] > 0,
    "Laser mapping marks free path and wall endpoint");
  const auto accepted = map->accepted;
  events.Publish(first);
  Check(
    mapping.Find(rover.GetEntity())->accepted == accepted, "Duplicate scan must not add evidence");
  RoverMotionCommandEvent command;
  command.roverId = 10;
  command.forwardSpeed = .1;
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x > 0,
    "LiDAR-only drive moves with clear front");
  wall.x = .2f;
  for (int i = 0; i < 12; ++i)
    tick(.01f);
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "LiDAR-only safety stops for front wall");
  wall.x = -.2f;
  for (int i = 0; i < 12; ++i)
    tick(.01f);
  command.forwardSpeed = -.1;
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Rear scan protects reverse recovery");
  wall.x = 1;
  for (int i = 0; i < 12; ++i)
    tick(.01f);
  command.forwardSpeed = .1;
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x > 0,
    "Fresh clear scan restarts forward travel");
  events.Publish(command);
  drive.Update(.31f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Stale LiDAR cannot authorize travel");
  // A rear-side wall is outside the rectangular reverse stopping corridor.
  // Collapsing its 150-degree hit into a 30-degree cone invents a blockage
  // toward 165 degrees, although LiDAR measured the actual bearing.
  auto gapScan = first;
  gapScan.simulationTime = lidar.SimulationTime();
  for (std::size_t i = 0; i < gapScan.beams.size(); ++i) {
    auto& beam = gapScan.beams[i];
    beam.capturedAt = gapScan.simulationTime;
    beam.distance = config.maximumRange;
    beam.status = SensorReadingStatus::OutOfRange;
  }
  gapScan.beams[150].status = SensorReadingStatus::Hit;
  gapScan.beams[150].distance = .4;
  events.Publish(gapScan);
  command.forwardSpeed = -.1;
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x < 0,
    "Measured rear-side LiDAR wall outside swept corridor must not block reverse through gap");
  gapScan.beams[180].status = SensorReadingStatus::Hit;
  gapScan.beams[180].distance = .4;
  events.Publish(gapScan);
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "A real obstacle inside reverse stopping corridor still blocks motion");
  const auto beforePause = wall.rays;
  lidar.Update(0);
  Check(wall.rays == beforePause, "Pause freezes scan");
  config.enabled = false;
  tick(.01f);
  Check(!scans.back().valid && !lidar.Latest(sensor.GetEntity()),
    "Disabled scanner invalidates delivered data");
  config.enabled = true;
  config.deliveryDelay = .2;
  config.noiseAmplitude = .03;
  lidar.Reset();
  mapping.Reset();
  scans.clear();
  wall.x = 1;
  for (int i = 0; i < 29; ++i)
    tick(.01f);
  Check(scans.empty(), "Configured delivery delay is respected");
  for (int i = 0; i < 3; ++i)
    tick(.01f);
  Check(!scans.empty() && scans.back().simulationTime - scans.back().beams.back().capturedAt >= .19,
    "Delayed scan retains original timestamps");
  const auto noisy = scans.front().beams;
  Check(std::abs(noisy[0].distance - 1) <= .03, "Noise stays within amplitude");
  lidar.Reset();
  mapping.Reset();
  scans.clear();
  for (int i = 0; i < 32; ++i)
    tick(.01f);
  Check(Near(scans.front().beams[0].distance, noisy[0].distance), "Reset reproduces seeded noise");
  config.deliveryDelay = 0;
  config.dropoutProbability = 1;
  lidar.Reset();
  mapping.Reset();
  scans.clear();
  for (int i = 0; i < 12; ++i)
    tick(.01f);
  Check(std::all_of(scans.back().beams.begin(),
          scans.back().beams.end(),
          [](auto& beam) { return beam.status == SensorReadingStatus::Dropped; }),
    "Full dropout marks every beam");
  map = mapping.Find(rover.GetEntity());
  Check(map && map->accepted == 0 &&
          std::all_of(map->cells.begin(), map->cells.end(), [](auto score) { return score == 0; }),
    "Dropped rays must not clear unknown map");
  command.forwardSpeed = .1;
  events.Publish(command);
  drive.Update(.01f);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity.x == 0,
    "Dropped LiDAR data cannot authorize motion");
  // Only two valid beams, captured at different estimated rover positions.
  // At delivery x=1, replaying the whole scan at that pose would invent a wall at x=2.
  config.samplesPerScan = 36;
  config.dropoutProbability = 0;
  config.noiseAmplitude = 0;
  mapping.Reset();
  RoverOdometryEvent oldPose;
  oldPose.rover = rover;
  oldPose.simulationTime = .1;
  events.Publish(oldPose);
  auto newPose = oldPose;
  newPose.x = 1;
  newPose.simulationTime = .2;
  events.Publish(newPose);
  LidarScanEvent moving;
  moving.sensor = sensor;
  moving.roverId = 10;
  moving.settings = config;
  moving.valid = true;
  moving.simulationTime = .2;
  for (int i = 0; i < 36; ++i) {
    LidarBeam beam;
    beam.angle = 2 * std::numbers::pi * i / 36;
    beam.capturedAt = i == 0 ? .1 : .2;
    beam.status = i == 0 || i == 18 ? SensorReadingStatus::Hit : SensorReadingStatus::Dropped;
    beam.distance = 1;
    moving.beams.push_back(beam);
  }
  events.Publish(moving);
  map = mapping.Find(rover.GetEntity());
  Check(map && map->accepted == 2 && map->cells[map->Index(1, 0)] > 0 &&
          map->cells[map->Index(2, 0)] == 0,
    "Each laser beam maps at its own capture pose, not the scan delivery pose");
  sensor.Destroy();
  drive.Update(.01f);
  lidar.Update(.01f);
  std::cout << "LiDAR rotation, timestamps, mapping, front/rear safety, dropout, noise, reset and "
               "deletion passed\n";
}

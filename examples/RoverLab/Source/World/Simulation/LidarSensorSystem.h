#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Environment/EnvironmentQueries.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <deque>
#include <functional>
#include <limits>
#include <numbers>
#include <unordered_map>

#include "Events/LidarScanEvent.h"

namespace RoverLab {
// Geometric 2D rotating LiDAR. A fixed-step batch samples only the current pose;
// beams never claim a historical pose/time that the simulator did not sample.
class LidarSensorSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;
  LidarSensorSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events) {}
  std::string_view GetSystemId() const override { return "roverlab.lidar"; }
  void SetObjectResolver(Resolver value) {
    resolve = std::move(value);
    Reset();
  }
  void Reset() {
    states.clear();
    time = 0;
  }
  double SimulationTime() const { return time; }
  const LidarScanEvent* Latest(pipeframe::ecs::Entity id) const {
    const auto it = states.find(id);
    return it == states.end() || !it->second.latest.valid ? nullptr : &it->second.latest;
  }
  void AlignMounts() {
    scene.Components().Each<LidarSensorComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity id) { Align(scene.GetObject(id)); });
  }
  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) return;
    time += delta;
    auto* environment = scene.Services().Find<pipeframe::EnvironmentQueries>();
    std::vector<pipeframe::ecs::Entity> ids;
    scene.Components().Each<LidarSensorComponent>(
      [&](pipeframe::ecs::Entity id) { ids.push_back(id); });
    std::sort(ids.begin(), ids.end());
    std::erase_if(states, [&](auto& entry) {
      const auto object = entry.second.sensor;
      const auto* config =
        object.IsValid() ? object.template GetComponent<LidarSensorComponent>() : nullptr;
      if (!environment || !object.IsValid() || !scene.IsActive(entry.first) || !config ||
          !config->enabled || !config->Valid() || entry.second.sensor != object ||
          !(entry.second.settings == *config) || !resolve ||
          entry.second.rover != resolve(config->parent.objectId) || !Align(object)) {
        LidarScanEvent invalid;
        invalid.sensor = entry.second.sensor;
        invalid.roverId = entry.second.settings.parent.objectId;
        invalid.simulationTime = time;
        events.Publish(invalid);
        return true;
      }
      return false;
    });
    if (!environment) return;
    for (const auto id : ids) {
      const auto object = scene.GetObject(id);
      const auto& config = *object.GetComponent<LidarSensorComponent>();
      if (!config.enabled || !scene.IsActive(id) || !Align(object)) continue;
      auto [it, created] = states.try_emplace(id);
      auto& state = it->second;
      if (created) {
        state.sensor = object;
        state.rover = resolve(config.parent.objectId);
        state.settings = config;
      }
      state.credit += double(delta) * config.scanRate * config.samplesPerScan;
      const int count = std::min(int(config.samplesPerScan), int(std::min(state.credit, 720.0)));
      state.credit -= count;
      state.credit =
        std::min(state.credit, 1.0);  // Drop missed revolutions after an oversized step.
      for (int i = 0; i < count; ++i) {
        Capture(state, *environment);
        if (state.beams.size() == std::size_t(config.samplesPerScan)) {
          LidarScanEvent scan;
          scan.sensor = object;
          scan.roverId = config.parent.objectId;
          scan.settings = config;
          scan.beams.swap(state.beams);
          scan.valid = true;
          state.pending.push_back({std::move(scan), time + config.deliveryDelay});
          LidarDebugEvent debug;
          debug.sensor = object;
          debug.simulationTime = time;
          debug.rays.swap(state.rays);
          events.Publish(debug);
        }
      }
      while (!state.pending.empty() && state.pending.front().due <= time + 1e-7) {
        state.latest = std::move(state.pending.front().scan);
        state.pending.pop_front();
        state.latest.simulationTime = time;
        events.Publish(state.latest);
      }
    }
  }

private:
  struct Pending {
    LidarScanEvent scan;
    double due{};
  };
  struct State {
    pipeframe::SceneObject sensor, rover;
    LidarSensorComponent settings;
    double credit{};
    std::uint64_t captures{};
    std::vector<LidarBeam> beams;
    std::vector<LidarDebugEvent::Ray> rays;
    std::deque<Pending> pending;
    LidarScanEvent latest;
  };
  static double Random(std::uint64_t seed, std::uint64_t capture, std::uint64_t lane) {
    auto value = seed ^ (capture * 0x9e3779b97f4a7c15ULL) ^ (lane * 0xbf58476d1ce4e5b9ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return double((value ^ (value >> 31)) >> 11) / 9007199254740992.0;
  }
  bool Align(pipeframe::SceneObject object) {
    const auto* config = object.GetComponent<LidarSensorComponent>();
    auto* pose = object.GetComponent<pipeframe::Transform2DComponent>();
    if (!config || !pose || !config->Valid() || !resolve) return false;
    const auto parent = resolve(config->parent.objectId);
    const auto* rover =
      parent.IsValid() ? parent.GetComponent<pipeframe::Transform2DComponent>() : nullptr;
    if (!parent.IsValid() || parent == object || !scene.IsActive(parent.GetEntity()) || !rover ||
        rover->scale != pipeframe::Vector2f{1, 1} || !std::isfinite(rover->position.x) ||
        !std::isfinite(rover->position.y) || !std::isfinite(rover->rotation))
      return false;
    const double c = std::cos(rover->rotation), s = std::sin(rover->rotation);
    pose->position = {
      float(rover->position.x + c * config->localPosition.x - s * config->localPosition.y),
      float(rover->position.y + s * config->localPosition.x + c * config->localPosition.y)};
    pose->rotation = float(rover->rotation + config->localAngleDegrees * std::numbers::pi / 180);
    return true;
  }
  void Capture(State& state, pipeframe::EnvironmentQueries& environment) {
    const auto& config = state.settings;
    const auto& pose = *state.sensor.GetComponent<pipeframe::Transform2DComponent>();
    LidarBeam beam;
    beam.angle = 2 * std::numbers::pi * state.beams.size() / config.samplesPerScan;
    beam.capturedAt = time;
    const double angle = pose.rotation + beam.angle;
    const pipeframe::Vector2f direction{float(std::cos(angle)), float(std::sin(angle))};
    const auto hit = environment.RaycastEnvironment(pose.position,
      direction,
      float(config.maximumRange),
      std::numeric_limits<std::uint32_t>::max(),
      config.parent.objectId);
    const auto capture = state.captures++;
    if (hit && (!std::isfinite(hit->distance) || hit->distance < 0 ||
                 hit->distance > config.maximumRange)) {
      beam.status = SensorReadingStatus::Unavailable;
    } else if (Random(config.randomSeed ^ config.parent.objectId, capture, 1) <
               config.dropoutProbability) {
      beam.status = SensorReadingStatus::Dropped;
    } else if (!hit) {
      beam.status = SensorReadingStatus::OutOfRange;
      beam.distance = config.maximumRange;
    } else if (hit->distance < config.minimumRange) {
      beam.status = SensorReadingStatus::BelowMinimumRange;
    } else {
      beam.distance = std::min(config.maximumRange,
        hit->distance + config.noiseAmplitude *
                          (2 * Random(config.randomSeed ^ config.parent.objectId, capture, 2) - 1));
      beam.status = beam.distance < config.minimumRange ? SensorReadingStatus::BelowMinimumRange
                                                        : SensorReadingStatus::Hit;
      if (beam.status == SensorReadingStatus::BelowMinimumRange) beam.distance = 0;
    }
    state.beams.push_back(beam);
    state.rays.push_back({pose.position,
      pose.position + direction * float(hit && std::isfinite(hit->distance) ? hit->distance
                                                                            : config.maximumRange),
      bool(hit)});
  }
  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  Resolver resolve;
  double time{};
  std::unordered_map<pipeframe::ecs::Entity, State> states;
};
}  // namespace RoverLab

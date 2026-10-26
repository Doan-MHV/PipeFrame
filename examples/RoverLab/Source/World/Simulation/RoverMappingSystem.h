#pragma once

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <numbers>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Components/RoverMappingComponent.h"
#include "Components/UltrasonicScheduleComponent.h"
#include "Events/LidarScanEvent.h"
#include "Events/RoverOdometryEvent.h"
#include "Events/SensorReadingEvent.h"
#include "RangeMount.h"

namespace RoverLab {

class RoverMappingSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  using Entity = pipeframe::ecs::Entity;

  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;

  struct Pose {
    double time{};
    double x{};
    double y{};
    double heading{};
  };

  struct Map {
    pipeframe::SceneObject rover;
    RoverMappingComponent settings;
    Pose pose;

    // Evidence scores, not calibrated probabilities.
    // Zero starts unknown.
    std::vector<std::int8_t> cells;

    std::uint64_t generation{};
    std::uint64_t accepted{};
    std::uint64_t ignored{};

    std::string status{"Waiting for sensor readings"};

    // Runtime bounds grow in blocks; authored sideCells is only the initial size.
    int width{}, height{};
    double originX{}, originY{};
    bool storageLimited{};
    static constexpr std::size_t MaxCells = 4 * 1024 * 1024;

    double Width() const { return width * settings.cellSize; }
    double Height() const { return height * settings.cellSize; }
    double Extent() const { return std::max(Width(), Height()); }

    int Index(double x, double y) const {
      const double col = std::floor((x - originX) / settings.cellSize);
      const double row = std::floor((y - originY) / settings.cellSize);
      if (!std::isfinite(col) || !std::isfinite(row) || col < 0 || row < 0 || col >= width ||
          row >= height)
        return -1;
      return int(row) * width + int(col);
    }

    void Initialize() {
      width = height = int(settings.sideCells);
      originX = originY = -width * settings.cellSize / 2;
      cells.assign(std::size_t(width) * height, 0);
      storageLimited = false;
    }

    // Preserve the cell lattice and evidence, including when growing left/up.
    // A bounded allocation refuses extreme odometry jumps without deleting data.
    bool EnsureBounds(double left, double top, double right, double bottom) {
      if (!width || !height) return false;
      const double cell = settings.cellSize;
      const double addLeft = std::max(0.0, std::ceil((originX - left) / (64 * cell))) * 64;
      const double addTop = std::max(0.0, std::ceil((originY - top) / (64 * cell))) * 64;
      const double addRight =
        std::max(0.0, std::ceil((std::floor((right - originX) / cell) + 1 - width) / 64)) * 64;
      const double addBottom =
        std::max(0.0, std::ceil((std::floor((bottom - originY) / cell) + 1 - height) / 64)) * 64;
      const double newWidth = width + addLeft + addRight;
      const double newHeight = height + addTop + addBottom;
      if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) ||
          !std::isfinite(bottom) || left > right || top > bottom ||
          !std::isfinite(newWidth * newHeight) || newWidth * newHeight > MaxCells) {
        storageLimited = true;
        return false;
      }
      storageLimited = false;
      if (newWidth == width && newHeight == height) return true;
      std::vector<std::int8_t> grown(std::size_t(newWidth) * std::size_t(newHeight), 0);
      for (int row = 0; row < height; ++row) {
        std::copy_n(cells.begin() + row * width,
          width,
          grown.begin() + (row + int(addTop)) * int(newWidth) + int(addLeft));
      }
      cells.swap(grown);
      width = int(newWidth);
      height = int(newHeight);
      originX -= addLeft * cell;
      originY -= addTop * cell;
      return true;
    }
  };

  RoverMappingSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene) {
    odometryConnection = events.Listen<RoverOdometryEvent>(
      [this](const RoverOdometryEvent& event) { OnOdometry(event); });

    lidarConnection = events.Listen<LidarScanEvent>([this](const auto& event) { OnScan(event); });
    sensorConnection = events.Listen<SensorReadingEvent>(
      [this](const SensorReadingEvent& event) { OnReading(event); });
  }

  std::string_view GetSystemId() const override { return "roverlab.mapping"; }

  void SetObjectResolver(Resolver value) { resolveObject = std::move(value); }

  void Reset() { states.clear(); }

  void Update(float) override {
    std::erase_if(states, [this](const auto& item) { return !Current(item.second.map); });

    for (auto& [entity, state] : states) {
      std::erase_if(state.lidarConsumed, [this](const auto& item) {
        return !item.second.sensor.IsValid() || !scene.IsActive(item.first);
      });
      std::erase_if(state.consumed, [this](const auto& item) {
        return !item.second.sensor.IsValid() || !scene.IsActive(item.first);
      });
    }
  }

  const Map* Find(Entity entity) const {
    const auto found = states.find(entity);

    return found != states.end() && Current(found->second.map) ? &found->second.map : nullptr;
  }

  std::vector<Entity> Entities() const {
    std::vector<Entity> result;

    for (const auto& [entity, state] : states) {
      if (Current(state.map)) {
        result.push_back(entity);
      }
    }

    std::sort(result.begin(), result.end());
    return result;
  }

private:
  struct Consumed {
    pipeframe::SceneObject sensor;
    double time{-1};
  };

  struct State {
    Map map;
    std::deque<Pose> history;
    std::unordered_map<Entity, Consumed> consumed;
    std::unordered_map<Entity, Consumed> lidarConsumed;
  };

  static constexpr double Pi = std::numbers::pi;

  // RL5 currently allows delivery delays up to two seconds.
  static constexpr double HistorySeconds = 3.0;
  static constexpr double MaxDeliveryAge = 2.5;

  static constexpr double TimeTolerance = 1e-6;
  static constexpr std::size_t MaxHistorySamples = 4096;

  bool Current(const Map& map) const {
    if (!map.rover.IsValid() || !scene.IsActive(map.rover.GetEntity())) {
      return false;
    }

    const auto* config = map.rover.GetComponent<RoverMappingComponent>();

    return config && config->enabled && config->Valid() && *config == map.settings;
  }

  void OnOdometry(const RoverOdometryEvent& event) {
    if (!event.rover.IsValid() || !scene.IsActive(event.rover.GetEntity()) ||
        !std::isfinite(event.simulationTime) || event.simulationTime < 0 ||
        !std::isfinite(event.x) || !std::isfinite(event.y) || !std::isfinite(event.heading)) {
      return;
    }

    const auto* config = event.rover.GetComponent<RoverMappingComponent>();

    if (!config || !config->enabled || !config->Valid()) {
      return;
    }

    auto& state = states[event.rover.GetEntity()];

    if (!(state.map.rover == event.rover) || !Current(state.map) ||
        (!state.history.empty() && event.simulationTime < state.history.back().time)) {
      state = State{};
      state.map.rover = event.rover;
      state.map.settings = *config;

      state.map.Initialize();
      state.map.generation = ++generation;
    }

    if (!state.history.empty() && event.simulationTime <= state.history.back().time) {
      return;
    }

    state.map.pose = {
      event.simulationTime,
      event.x,
      event.y,
      event.heading,
    };

    if (!state.map.EnsureBounds(event.x, event.y, event.x, event.y)) {
      state.map.status = "Map storage limit reached; existing evidence retained";
    }
    state.history.push_back(state.map.pose);

    // Retain one sample before the time window for interpolation.
    while (
      state.history.size() > 2 && state.history[1].time < event.simulationTime - HistorySeconds) {
      state.history.pop_front();
    }

    while (state.history.size() > MaxHistorySamples) {
      state.history.pop_front();
    }
  }

  static std::optional<Pose> PoseAt(const State& state, double time) {
    const auto& history = state.history;

    if (history.empty() || time < history.front().time - TimeTolerance ||
        time > history.back().time + TimeTolerance) {
      return {};
    }

    if (time <= history.front().time) {
      return history.front();
    }

    for (std::size_t i = 1; i < history.size(); ++i) {
      if (time > history[i].time) {
        continue;
      }

      const auto& a = history[i - 1];
      const auto& b = history[i];

      if (std::abs(time - b.time) <= TimeTolerance) {
        return b;
      }

      // Do not invent motion across an inactive rover
      // or a large gap in odometry.
      if (b.time - a.time > 0.25) {
        return {};
      }

      const double t = (time - a.time) / (b.time - a.time);

      return Pose{
        time,
        std::lerp(a.x, b.x, t),
        std::lerp(a.y, b.y, t),
        a.heading + t * std::remainder(b.heading - a.heading, 2 * Pi),
      };
    }

    return history.back();
  }

  static bool ValidSensor(const RangeMount& sensor) {
    return sensor.enabled && std::isfinite(sensor.localPosition.x) &&
           std::isfinite(sensor.localPosition.y) && std::isfinite(sensor.localAngleDegrees) &&
           std::isfinite(sensor.minimumRange) && std::isfinite(sensor.maximumRange) &&
           sensor.minimumRange > 0 && sensor.maximumRange > sensor.minimumRange &&
           sensor.maximumRange <= 10 && std::isfinite(sensor.coneAngleDegrees) &&
           sensor.coneAngleDegrees >= 1 && sensor.coneAngleDegrees <= 90;
  }

  void OnScan(const LidarScanEvent& event) {
    if (!resolveObject || !event.valid || !event.sensor.IsValid() || !event.settings.Valid())
      return;
    const auto rover = resolveObject(event.roverId);
    if (!rover.IsValid()) return;
    const auto found = states.find(rover.GetEntity());
    if (found == states.end() || found->second.map.rover != rover || !Current(found->second.map))
      return;
    auto& state = found->second;
    auto& map = state.map;
    const auto ignore = [&](const char* reason) {
      ++map.ignored;
      map.status = reason;
    };
    const auto* sensor = event.sensor.GetComponent<LidarSensorComponent>();
    if (!sensor || !sensor->enabled || !scene.IsActive(event.sensor.GetEntity()) ||
        !(*sensor == event.settings) || sensor->parent.objectId != event.roverId ||
        event.beams.size() != std::size_t(sensor->samplesPerScan)) {
      ignore("Invalid or detached LiDAR");
      return;
    }
    if (!std::isfinite(event.simulationTime) ||
        event.simulationTime > map.pose.time + TimeTolerance) {
      ignore("Invalid scan delivery time");
      return;
    }
    // Validate the complete scan before writing any cells. Equal timestamps are
    // expected within a fixed-step beam batch, but angles must cover one revolution.
    double previous = -1;
    for (std::size_t i = 0; i < event.beams.size(); ++i) {
      const auto& beam = event.beams[i];
      if (!std::isfinite(beam.capturedAt) || beam.capturedAt < 0 || beam.capturedAt < previous ||
          beam.capturedAt > event.simulationTime + TimeTolerance ||
          map.pose.time - beam.capturedAt > MaxDeliveryAge || !std::isfinite(beam.angle) ||
          std::abs(beam.angle - 2 * Pi * i / event.beams.size()) > 1e-6) {
        ignore("Invalid or expired scan timestamp/angle");
        return;
      }
      previous = beam.capturedAt;
    }
    auto& consumed = state.lidarConsumed[event.sensor.GetEntity()];
    if (consumed.sensor != event.sensor) consumed = {event.sensor, -1};
    if (previous <= consumed.time) {
      ignore("Duplicate or out-of-order scan");
      return;
    }
    consumed.time = previous;
    for (const auto& beam : event.beams) {
      if ((beam.status != SensorReadingStatus::Hit &&
            beam.status != SensorReadingStatus::OutOfRange) ||
          !std::isfinite(beam.distance) || beam.distance < sensor->minimumRange ||
          beam.distance > sensor->maximumRange) {
        ignore("No usable LiDAR return; beam skipped");
        continue;
      }
      const auto pose = PoseAt(state, beam.capturedAt);
      if (!pose) {
        ignore("Beam capture pose unavailable");
        continue;
      }
      if (!IntegrateBeam(map, *pose, *sensor, beam)) {
        ignore("Map storage limit reached; existing evidence retained");
        continue;
      }
      ++map.accepted;
      map.status = "LiDAR beam integrated";
    }
  }

  static bool IntegrateBeam(
    Map& map, const Pose& pose, const LidarSensorComponent& sensor, const LidarBeam& beam) {
    const double c = std::cos(pose.heading), s = std::sin(pose.heading);
    const double x = pose.x + c * sensor.localPosition.x - s * sensor.localPosition.y;
    const double y = pose.y + s * sensor.localPosition.x + c * sensor.localPosition.y;
    const double angle = pose.heading + sensor.localAngleDegrees * Pi / 180 + beam.angle;
    const double dx = std::cos(angle), dy = std::sin(angle);
    const bool hit = beam.status == SensorReadingStatus::Hit;
    const double range = hit ? beam.distance : sensor.maximumRange;
    const double margin = map.settings.rangeMargin + sensor.noiseAmplitude;
    const double end = hit ? std::min(sensor.maximumRange, range + margin) : range;
    const double ex = x + dx * end, ey = y + dy * end;
    if (!map.EnsureBounds(std::min(x, ex), std::min(y, ey), std::max(x, ex), std::max(y, ey)))
      return false;
    // A narrow laser beam carves a line; it does not paint an ultrasonic cone.
    // Accumulate once per crossed cell, with occupied evidence winning at the end.
    const double step = map.settings.cellSize * .4;
    int last = -1, evidence = 0;
    const auto commit = [&] {
      if (last >= 0 && evidence)
        map.cells[last] = std::int8_t(std::clamp(int(map.cells[last]) + evidence, -12, 12));
    };
    const int steps = std::max(1, int(std::ceil((end - sensor.minimumRange) / step)));
    for (int i = 0; i <= steps; ++i) {
      const double r = std::lerp(sensor.minimumRange, end, double(i) / steps);
      const int index = map.Index(x + dx * r, y + dy * r);
      if (index != last) {
        commit();
        last = index;
        evidence = 0;
      }
      const int delta = hit && r >= range - margin ? 1 : -1;
      evidence = evidence == 1 ? 1 : delta;
    }
    commit();
    return true;
  }

  void OnReading(const SensorReadingEvent& event) {
    if (!resolveObject) {
      return;
    }

    const auto rover = resolveObject(event.roverId);

    if (!rover.IsValid()) {
      return;
    }

    const auto found = states.find(rover.GetEntity());

    if (found == states.end() || !(found->second.map.rover == rover) ||
        !Current(found->second.map)) {
      return;
    }

    auto& state = found->second;
    auto& map = state.map;

    const auto ignore = [&](const char* reason) {
      ++map.ignored;
      map.status = reason;
    };

    if (!scene.Components().IsAlive(event.sensor)) {
      ignore("Removed sensor");
      return;
    }
    const auto sensorObject = scene.GetObject(event.sensor);

    const auto sensor = ReadRangeMount(sensorObject);

    if (!sensorObject.IsValid() || !scene.IsActive(event.sensor) || !sensor ||
        sensor->parent.objectId != event.roverId || !ValidSensor(*sensor)) {
      ignore("Invalid or detached sensor");
      return;
    }

    if (event.AgeAtDelivery() > MaxDeliveryAge ||
        event.simulationTime > map.pose.time + TimeTolerance ||
        map.pose.time - event.capturedAt > MaxDeliveryAge) {
      ignore("Invalid or expired timestamp");
      return;
    }

    auto& consumed = state.consumed[event.sensor];

    if (!(consumed.sensor == sensorObject)) {
      consumed = {sensorObject, -1};
    }

    if (event.capturedAt <= consumed.time) {
      ignore("Duplicate or out-of-order reading");
      return;
    }

    consumed.time = event.capturedAt;

    if (!event.valid || (event.status != SensorReadingStatus::Hit &&
                          event.status != SensorReadingStatus::OutOfRange)) {
      ignore("No usable echo; map unchanged");
      return;
    }

    if (event.status == SensorReadingStatus::Hit &&
        (!std::isfinite(event.distance) || event.distance < sensor->minimumRange ||
          event.distance > sensor->maximumRange)) {
      ignore("Invalid echo distance");
      return;
    }

    const auto pose = PoseAt(state, event.capturedAt);

    if (!pose) {
      ignore("Capture pose unavailable");
      return;
    }

    const auto* optical = sensorObject.GetComponent<InfraredSensorComponent>();
    const auto* schedule = optical ? nullptr : rover.GetComponent<UltrasonicScheduleComponent>();

    if (schedule && !schedule->Valid()) {
      ignore("Invalid sensor schedule");
      return;
    }

    const double noise = optical    ? optical->noiseAmplitude
                         : schedule ? schedule->noiseAmplitude
                                    : 0;

    if (!Integrate(map, *pose, *sensor, event, noise)) {
      ignore("Map storage limit reached; existing evidence retained");
      return;
    }

    ++map.accepted;
    map.status = "Observation integrated";
  }

  static bool Integrate(Map& map,
    const Pose& pose,
    const RangeMount& sensor,
    const SensorReadingEvent& event,
    double noise) {
    // Mounts are authored in metres.
    // This RL6 baseline requires rover scale (1, 1).
    const double c = std::cos(pose.heading);
    const double s = std::sin(pose.heading);

    const double ox = pose.x + c * sensor.localPosition.x - s * sensor.localPosition.y;

    const double oy = pose.y + s * sensor.localPosition.x + c * sensor.localPosition.y;

    const double heading = pose.heading + sensor.localAngleDegrees * Pi / 180;

    const double halfCone = sensor.coneAngleDegrees * Pi / 360;

    const double rayStep = 2 * halfCone / (SensorReadingEvent::RayCount - 1);

    const double cell = map.settings.cellSize;
    const double radius = cell * std::sqrt(0.5);

    const double margin = map.settings.rangeMargin + noise;

    const bool hit = event.status == SensorReadingStatus::Hit;

    const double freeEnd = hit ? event.distance - margin : sensor.maximumRange;

    const double reach = (hit ? event.distance : sensor.maximumRange) + margin + radius;
    if (!map.EnsureBounds(ox - reach, oy - reach, ox + reach, oy + reach)) return false;
    // Work only on the observation bounds, not the entire expanded map.
    const int firstCol = std::max(0, int(std::floor((ox - reach - map.originX) / cell)));
    const int lastCol = std::min(map.width - 1, int(std::floor((ox + reach - map.originX) / cell)));
    const int firstRow = std::max(0, int(std::floor((oy - reach - map.originY) / cell)));
    const int lastRow =
      std::min(map.height - 1, int(std::floor((oy + reach - map.originY) / cell)));
    for (int row = firstRow; row <= lastRow; ++row) {
      for (int col = firstCol; col <= lastCol; ++col) {
        const double dx = map.originX + (col + 0.5) * cell - ox;
        const double dy = map.originY + (row + 0.5) * cell - oy;

        const double range = std::hypot(dx, dy);

        if (range <= radius || range > sensor.maximumRange + margin + radius) {
          continue;
        }

        const double angle = std::remainder(std::atan2(dy, dx) - heading, 2 * Pi);

        const double cellAngle = std::asin(std::min(1.0, radius / range));

        int evidence = 0;

        if (hit && std::abs(range - event.distance) <= margin + radius &&
            std::abs(angle) <= halfCone + cellAngle) {
          // The echo has no bearing.
          // Spread weak occupied evidence across an arc.
          evidence = 1;
        } else if (std::abs(angle) <= halfCone) {
          const double ray = std::round((angle + halfCone) / rayStep) * rayStep - halfCone;

          const double along = range * std::cos(angle - ray);

          const double across = std::abs(range * std::sin(angle - ray));

          // Free evidence near sampled rays, not the entire cone.
          // This is evidence, not proof that the whole cell is clear.
          if (across <= cell * 0.25 && along - radius >= sensor.minimumRange &&
              along + radius < freeEnd) {
            evidence = -1;
          }
        }

        auto& score = map.cells[std::size_t(row * map.width + col)];

        score = std::int8_t(std::clamp(int(score) + evidence, -12, 12));
      }
    }
    return true;
  }

  pipeframe::BehaviourScene& scene;
  Resolver resolveObject;

  std::unordered_map<Entity, State> states;
  std::uint64_t generation{};

  pipeframe::EventSubscription odometryConnection;
  pipeframe::EventSubscription sensorConnection, lidarConnection;
};

}  // namespace RoverLab

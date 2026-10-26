#pragma once
#include <PipeFrame/Events/EventChannels.h>

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <unordered_map>

#include "Components/RoverDriveComponent.h"
#include "Events/LidarScanEvent.h"
#include "Events/RoverOdometryEvent.h"
#include "RangeMount.h"

namespace RoverLab {
struct RangeObservation {
  pipeframe::SceneObject sensor;
  RangeMount settings;
  double age{std::numeric_limits<double>::infinity()};
  double capturedAt{}, distance{};
  bool valid{}, present{};
  SensorReadingStatus status{SensorReadingStatus::Unavailable};
  std::uint64_t sequence{};
  pipeframe::SceneObject rover;
};
// Each consumer owns its cache. LiDAR reduces to twelve measured 30-degree sectors
// for the reactive controller. Safety can use timestamp-corrected measured beams;
// mapping also receives every original beam.
class RoverRangeObservations {
public:
  RoverRangeObservations(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene) {
    odometryConnection = events.Listen<RoverOdometryEvent>([this](const auto& event) {
      if (!event.rover.IsValid() || !std::isfinite(event.simulationTime) ||
          !std::isfinite(event.x) || !std::isfinite(event.y) || !std::isfinite(event.heading))
        return;
      auto& history = poses[event.rover.GetEntity()];
      if (!history.empty() && (history.back().rover != event.rover ||
                                event.simulationTime <= history.back().simulationTime))
        history.clear();
      history.push_back(event);
      while (history.size() > 1 && history.front().simulationTime < event.simulationTime - 5)
        history.pop_front();
    });
    ultrasoundConnection = events.Listen<SensorReadingEvent>([this](const auto& event) {
      if (!this->scene.Components().IsAlive(event.sensor)) return;
      const auto object = this->scene.GetObject(event.sensor);
      if (!object.IsValid()) return;
      const auto config = ReadRangeMount(object);
      if (!object.IsValid() || !config || config->parent.objectId != event.roverId) return;
      ultrasonic[event.sensor] = {object,
        *config,
        event.AgeAtDelivery(),
        event.capturedAt,
        event.distance,
        event.valid && std::isfinite(event.AgeAtDelivery()) && std::isfinite(event.distance) &&
          event.distance >= 0,
        true,
        event.status,
        ++sequence,
        resolve ? resolve(event.roverId) : pipeframe::SceneObject{}};
    });
    lidarConnection = events.Listen<LidarScanEvent>([this](const auto& event) {
      if (!event.sensor.IsValid()) return;
      const auto id = event.sensor.GetEntity();
      if (!event.valid || !event.settings.Valid() ||
          event.roverId != event.settings.parent.objectId ||
          event.beams.size() != std::size_t(event.settings.samplesPerScan)) {
        lidar.erase(id);
        return;
      }
      Scan cached;
      cached.sensor = event.sensor;
      cached.settings = event.settings;
      const auto stamp = ++sequence;
      const int samples = int(event.settings.samplesPerScan);
      cached.beams.reserve(samples);
      cached.preciseReady = true;
      std::array<int, 12> counts{};
      for (int sector = 0; sector < 12; ++sector) {
        auto& value = cached.sectors[sector];
        value.sensor = event.sensor;
        value.rover = resolve ? resolve(event.roverId) : pipeframe::SceneObject{};
        value.settings = Sector(event.settings, sector);
        value.valid = value.present = true;
        value.age = 0;
        value.capturedAt = event.simulationTime;
        value.distance = event.settings.maximumRange;
        value.status = SensorReadingStatus::OutOfRange;
        value.sequence = stamp;
      }
      for (int i = 0; i < samples; ++i) {
        const auto& beam = event.beams[i];
        const double expected = 2 * std::numbers::pi * i / samples;
        // Index-based coverage cannot be forged by duplicate/missing angles.
        const int sector = int(std::floor((double(i) * 12 / samples) + 0.5)) % 12;
        auto& value = cached.sectors[sector];
        ++counts[sector];
        value.age = std::max(value.age, event.simulationTime - beam.capturedAt);
        value.capturedAt = std::min(value.capturedAt, beam.capturedAt);
        const bool usable = std::isfinite(beam.angle) && std::abs(beam.angle - expected) < 1e-6 &&
                            std::isfinite(beam.capturedAt) && std::isfinite(event.simulationTime) &&
                            beam.capturedAt >= 0 && beam.capturedAt <= event.simulationTime &&
                            std::isfinite(beam.distance) && beam.distance >= 0 &&
                            beam.distance <= event.settings.maximumRange &&
                            (beam.status == SensorReadingStatus::Hit ||
                              beam.status == SensorReadingStatus::OutOfRange ||
                              beam.status == SensorReadingStatus::BelowMinimumRange);
        BeamObservation measured;
        measured.value = value;
        measured.value.settings.localAngleDegrees =
          event.settings.localAngleDegrees + expected * 180 / std::numbers::pi;
        measured.value.settings.coneAngleDegrees = 360.0 / samples;
        measured.value.age = event.simulationTime - beam.capturedAt;
        measured.value.capturedAt = beam.capturedAt;
        measured.value.distance =
          beam.status == SensorReadingStatus::BelowMinimumRange ? 0 : beam.distance;
        measured.value.status = beam.status;
        measured.value.valid = usable;
        const auto capturedPose = PoseAt(measured.value.rover, beam.capturedAt);
        if (capturedPose)
          measured.pose = *capturedPose;
        else
          cached.preciseReady = false;
        cached.beams.push_back(std::move(measured));
        value.valid = value.valid && usable;
        if (usable && beam.status != SensorReadingStatus::OutOfRange &&
            (value.status == SensorReadingStatus::OutOfRange || beam.distance < value.distance)) {
          value.status = beam.status;
          value.distance =
            beam.status == SensorReadingStatus::BelowMinimumRange ? 0 : beam.distance;
        }
      }
      for (int i = 0; i < 12; ++i)
        cached.sectors[i].valid &= counts[i] == samples / 12;
      for (int i = 0; i < samples; ++i) {
        const int sector = int(std::floor(double(i) * 12 / samples + .5)) % 12;
        cached.beams[i].value.valid &= cached.sectors[sector].valid;
      }
      lidar.insert_or_assign(id, std::move(cached));
    });
  }
  void SetObjectResolver(std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)> value) {
    resolve = std::move(value);
    Reset();
  }
  void Reset() {
    ultrasonic.clear();
    lidar.clear();
    poses.clear();
    sequence = 0;
  }
  std::uint64_t Sequence() const { return sequence; }
  void Update(float delta) {
    std::erase_if(ultrasonic, [&](const auto& entry) {
      const auto& object = entry.second.sensor;
      if (!object.IsValid()) return true;
      const auto config = ReadRangeMount(object);
      return !object.IsValid() || !config || !config->enabled ||
             !scene.IsActive(object.GetEntity()) || !ParentActive(config->parent.objectId);
    });
    std::erase_if(lidar, [&](const auto& entry) {
      const auto& object = entry.second.sensor;
      if (!object.IsValid()) return true;
      const auto* config = object.template GetComponent<LidarSensorComponent>();
      return !object.IsValid() || !config || !config->enabled ||
             !scene.IsActive(object.GetEntity()) || !ParentActive(config->parent.objectId);
    });
    for (auto& [id, value] : ultrasonic)
      value.age += delta;
    for (auto& [id, scan] : lidar) {
      for (auto& value : scan.sectors)
        value.age += delta;
      for (auto& beam : scan.beams)
        beam.value.age += delta;
    }
    std::erase_if(poses, [&](const auto& entry) {
      return entry.second.empty() || !entry.second.back().rover.IsValid();
    });
  }
  template <class F>
  void Each(F&& callback, bool measuredBeams = false) {
    const auto singleReturn = [&](pipeframe::ecs::Entity id) {
      const auto mount = ReadRangeMount(scene.GetObject(id));
      if (!mount) return;
      const auto& config = *mount;
      if (!config.enabled || !scene.IsActive(id)) return;
      RangeObservation value;
      value.sensor = scene.GetObject(id);
      value.settings = config;
      const auto found = ultrasonic.find(id);
      if (found != ultrasonic.end() && found->second.sensor == value.sensor) {
        value = found->second;
        value.valid &= value.settings == config;
        value.settings = config;
      }
      value.valid &=
        resolve && value.rover.IsValid() && value.rover == resolve(config.parent.objectId);
      callback(value);
    };
    scene.Components().Each<UltrasonicSensorComponent>(singleReturn);
    scene.Components().Each<InfraredSensorComponent>(singleReturn);
    scene.Components().Each<LidarSensorComponent>([&](pipeframe::ecs::Entity id) {
      const auto& config = *scene.Components().Get<LidarSensorComponent>(id);
      if (!config.enabled || !scene.IsActive(id)) return;
      const auto found = lidar.find(id);
      const auto rover = resolve ? resolve(config.parent.objectId) : pipeframe::SceneObject{};
      const auto history = rover.IsValid() ? poses.find(rover.GetEntity()) : poses.end();
      if (measuredBeams && found != lidar.end() && found->second.sensor == scene.GetObject(id) &&
          found->second.settings == config && found->second.preciseReady &&
          history != poses.end() && !history->second.empty() &&
          history->second.back().rover == rover) {
        const auto& current = history->second.back();
        for (const auto& beam : found->second.beams) {
          auto value = beam.value;
          const double c = std::cos(beam.pose.heading), s = std::sin(beam.pose.heading);
          const double x =
            beam.pose.x + c * config.localPosition.x - s * config.localPosition.y - current.x;
          const double y =
            beam.pose.y + s * config.localPosition.x + c * config.localPosition.y - current.y;
          const double nowC = std::cos(current.heading), nowS = std::sin(current.heading);
          // Express the capture ray's origin and bearing in the current body frame.
          // Moving during a rotating scan must not drag old hits with the rover.
          value.settings.localPosition = {float(nowC * x + nowS * y), float(-nowS * x + nowC * y)};
          value.settings.localAngleDegrees +=
            (beam.pose.heading - current.heading) * 180 / std::numbers::pi;
          value.valid &= config.Valid() && value.rover == rover;
          callback(value);
        }
        return;
      }
      // Missing pose history keeps the previous conservative sector behaviour.
      for (int sector = 0; sector < 12; ++sector) {
        RangeObservation value;
        value.sensor = scene.GetObject(id);
        if (found != lidar.end() && found->second.sensor == value.sensor &&
            found->second.settings == config)
          value = found->second.sectors[sector];
        value.settings = Sector(config, sector);
        value.valid &= config.Valid();
        value.valid &=
          resolve && value.rover.IsValid() && value.rover == resolve(config.parent.objectId);
        callback(value);
      }
    });
  }

private:
  std::optional<RoverOdometryEvent> PoseAt(pipeframe::SceneObject rover, double time) const {
    if (!rover.IsValid() || !std::isfinite(time)) return std::nullopt;
    const auto found = poses.find(rover.GetEntity());
    if (found == poses.end() || found->second.empty()) return std::nullopt;
    const auto& history = found->second;
    if (history.back().rover != rover || time < history.front().simulationTime - 1e-6 ||
        time > history.back().simulationTime + 1e-6)
      return std::nullopt;
    const auto next =
      std::lower_bound(history.begin(), history.end(), time, [](const auto& pose, double stamp) {
        return pose.simulationTime < stamp;
      });
    if (next == history.end()) return history.back();
    if (next == history.begin() || std::abs(next->simulationTime - time) < 1e-6) return *next;
    const auto& previous = *std::prev(next);
    const double t =
      (time - previous.simulationTime) / (next->simulationTime - previous.simulationTime);
    auto value = previous;
    value.x = std::lerp(previous.x, next->x, t);
    value.y = std::lerp(previous.y, next->y, t);
    value.heading += t * std::remainder(next->heading - previous.heading, 2 * std::numbers::pi);
    value.simulationTime = time;
    return value;
  }
  bool ParentActive(pipeframe::SceneObjectId id) const {
    if (!resolve) return false;
    const auto rover = resolve(id);
    if (!rover.IsValid() || !scene.IsActive(rover.GetEntity())) return false;
    const auto* drive = rover.GetComponent<RoverDriveComponent>();
    return drive && drive->enabled && drive->ValidSettings();
  }
  static RangeMount Sector(const LidarSensorComponent& config, int sector) {
    return {config.parent,
      config.localPosition,
      config.localAngleDegrees + sector * 30,
      config.minimumRange,
      config.maximumRange,
      30};
  }
  struct BeamObservation {
    RangeObservation value;
    RoverOdometryEvent pose;
  };
  struct Scan {
    pipeframe::SceneObject sensor;
    LidarSensorComponent settings;
    std::array<RangeObservation, 12> sectors;
    std::vector<BeamObservation> beams;
    bool preciseReady{};
  };
  std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)> resolve;
  pipeframe::BehaviourScene& scene;
  std::uint64_t sequence{};
  std::unordered_map<pipeframe::ecs::Entity, RangeObservation> ultrasonic;
  std::unordered_map<pipeframe::ecs::Entity, Scan> lidar;
  std::unordered_map<pipeframe::ecs::Entity, std::deque<RoverOdometryEvent>> poses;
  pipeframe::EventSubscription ultrasoundConnection, lidarConnection, odometryConnection;
};
}  // namespace RoverLab

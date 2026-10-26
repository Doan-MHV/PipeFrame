#pragma once

#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Environment/EnvironmentQueries.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <numbers>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Components/UltrasonicScheduleComponent.h"
#include "Components/UltrasonicSensorComponent.h"
#include "Events/SensorReadingEvent.h"

namespace RoverLab {

class UltrasonicSensorSystem final : public pipeframe::FixedUpdateSystem<void> {
  struct Member {
    pipeframe::SceneObject object;
    UltrasonicSensorComponent settings;
    double nextSample{};

    // Assigned from the group's sorted sensor order.
    std::uint64_t randomStream{};

    // Advances once per capture, including failed captures.
    std::uint64_t captureNumber{};
  };

  struct Pending {
    SensorReadingEvent reading;
    double deliverAt{};
  };

  struct Group {
    pipeframe::SceneObject rover;
    UltrasonicScheduleComponent settings;
    std::vector<Member> members;
    std::vector<Pending> pending;

    std::size_t cursor{};
    double nextTrigger{};
  };

  static constexpr double TimeTolerance = 1e-7;

public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;

  UltrasonicSensorSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events) {}

  std::string_view GetSystemId() const override { return "roverlab.ultrasonic-sampling"; }

  void SetObjectResolver(Resolver value) {
    resolveObject = std::move(value);
    Reset();
  }

  void Reset() {
    groups.clear();
    latest.clear();
    simulationTime = 0;
  }

  double SimulationTime() const { return simulationTime; }

  const SensorReadingEvent* Latest(pipeframe::ecs::Entity entity) const {
    const auto found = latest.find(entity);

    return found == latest.end() ? nullptr : &found->second;
  }

  // Editor alignment never advances the scheduling clock or takes a sample.
  void AlignMounts() {
    auto& components = scene.Components();

    auto align = [&](pipeframe::ecs::Entity entity) {
      const auto& sensor = *components.Get<UltrasonicSensorComponent>(entity);

      auto& pose = *components.Get<pipeframe::Transform2DComponent>(entity);

      AlignMount(scene.GetObject(entity), sensor, pose);
    };

    components.Each<UltrasonicSensorComponent, pipeframe::Transform2DComponent>(align);
  }

  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) {
      return;
    }

    simulationTime += delta;
    AlignMounts();

    auto* environment = scene.Services().Find<pipeframe::EnvironmentQueries>();

    // Rebuild the list of currently usable sensors.
    // std::map gives deterministic parent-group traversal.
    std::map<pipeframe::SceneObjectId, Group> candidates;

    if (environment && resolveObject) {
      auto& components = scene.Components();

      auto collect = [&](pipeframe::ecs::Entity entity) {
        const auto& sensor = *components.Get<UltrasonicSensorComponent>(entity);

        auto& pose = *components.Get<pipeframe::Transform2DComponent>(entity);

        const auto object = scene.GetObject(entity);

        if (!scene.IsActive(entity) || !sensor.enabled || !ValidSensor(sensor) ||
            !AlignMount(object, sensor, pose)) {
          return;
        }

        const auto rover = resolveObject(sensor.parent.objectId);

        UltrasonicScheduleComponent settings;

        if (const auto* configured = rover.GetComponent<UltrasonicScheduleComponent>()) {
          settings = *configured;
        }

        if (!settings.Valid()) {
          return;
        }

        auto& group = candidates[sensor.parent.objectId];
        group.rover = rover;
        group.settings = settings;
        group.members.push_back({object, sensor, 0});
      };

      components.Each<UltrasonicSensorComponent, pipeframe::Transform2DComponent>(collect);
    }

    // Cancel pending deliveries for removed/disabled groups.
    for (auto it = groups.begin(); it != groups.end();) {
      if (!candidates.contains(it->first)) {
        Invalidate(it->first, it->second);
        it = groups.erase(it);
      } else {
        ++it;
      }
    }

    for (auto& [roverId, candidate] : candidates) {
      // Stable order for the current scene. Names and compass
      // directions do not affect scheduling.
      std::sort(
        candidate.members.begin(), candidate.members.end(), [](const Member& a, const Member& b) {
          return a.object.GetEntity() < b.object.GetEntity();
        });

      auto found = groups.find(roverId);

      if (found == groups.end() || !SameGroup(found->second, candidate)) {
        if (found != groups.end()) {
          Invalidate(roverId, found->second);
        }

        // Treat the configuration as present from the beginning
        // of this fixed step. No historical samples are fabricated.
        const double start = simulationTime - delta;
        const double firstInterval = candidate.settings.sequential
                                       ? candidate.settings.slotPeriod
                                       : candidate.settings.samplePeriod;

        candidate.nextTrigger = start + firstInterval;

        for (std::size_t index = 0; index < candidate.members.size(); ++index) {
          auto& member = candidate.members[index];

          member.nextSample = start + firstInterval;
          member.randomStream = static_cast<std::uint64_t>(index);
          member.captureNumber = 0;
        }

        groups.insert_or_assign(roverId, std::move(candidate));
      }

      auto& group = groups.at(roverId);

      CaptureDue(roverId, group, *environment);
      DeliverDue(group);
    }
  }

private:
  // Fixed unsigned-integer mixing avoids dependence on a standard
  // library's floating-point random-distribution implementation.
  static std::uint64_t MixBits(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
  }

  // Produces a value in [0, 1).
  //
  // Separate lanes keep the dropout draw independent of the noise draw.
  // Changing noise amplitude therefore does not change the dropout pattern.
  static double RandomUnit(std::int64_t seed,
    pipeframe::SceneObjectId roverId,
    std::uint64_t stream,
    std::uint64_t capture,
    std::uint64_t lane) {
    auto bits = MixBits(static_cast<std::uint64_t>(seed));

    bits = MixBits(bits ^ static_cast<std::uint64_t>(roverId));
    bits = MixBits(bits ^ stream);
    bits = MixBits(bits ^ capture);
    bits = MixBits(bits ^ lane);

    // Keep 53 bits, matching double's integer precision.
    return static_cast<double>(bits >> 11) * (1.0 / 9007199254740992.0);
  }

  static void ApplyMeasurementEffects(SensorReadingEvent& reading,
    const UltrasonicSensorComponent& sensor,
    const UltrasonicScheduleComponent& settings,
    double dropoutDraw,
    double noiseDraw) {
    // Preserve genuine configuration/query failures as Unavailable.
    if (!reading.valid) {
      return;
    }

    if (dropoutDraw < settings.dropoutProbability) {
      reading.valid = false;
      reading.distance = 0;
      reading.tooClose = false;
      reading.status = SensorReadingStatus::Dropped;

      // Do not leave usable hit geometry on a failed measurement.
      reading.endpoints = {};
      reading.rangeEndpoints = {};
      reading.hits = {};
      reading.nearestRay = -1;
      return;
    }

    // This first model perturbs measurable echoes only.
    // It does not manufacture echoes for OutOfRange readings or make
    // a genuinely below-minimum obstacle appear safely measurable.
    if (reading.status != SensorReadingStatus::Hit || settings.noiseAmplitude == 0.0) {
      return;
    }

    const double error = (2.0 * noiseDraw - 1.0) * settings.noiseAmplitude;

    const double measured = reading.distance + error;

    if (measured < sensor.minimumRange) {
      reading.distance = 0;
      reading.tooClose = true;
      reading.status = SensorReadingStatus::BelowMinimumRange;
      return;
    }

    // An echo remains an echo when positive noise reaches the range limit.
    // Do not turn it into an OutOfRange/clear-space report.
    reading.distance = std::min(measured, sensor.maximumRange);
    reading.tooClose = false;
  }

  static bool ValidSensor(const UltrasonicSensorComponent& sensor) {
    return std::isfinite(sensor.localPosition.x) && std::isfinite(sensor.localPosition.y) &&
           std::isfinite(sensor.localAngleDegrees) && std::isfinite(sensor.minimumRange) &&
           std::isfinite(sensor.maximumRange) && std::isfinite(sensor.coneAngleDegrees) &&
           sensor.minimumRange > 0 && sensor.maximumRange > sensor.minimumRange &&
           sensor.coneAngleDegrees > 0 && sensor.coneAngleDegrees <= 90;
  }

  static bool SameSensor(const UltrasonicSensorComponent& a, const UltrasonicSensorComponent& b) {
    return a.parent.objectId == b.parent.objectId && a.localPosition == b.localPosition &&
           a.localAngleDegrees == b.localAngleDegrees && a.minimumRange == b.minimumRange &&
           a.maximumRange == b.maximumRange && a.coneAngleDegrees == b.coneAngleDegrees &&
           a.enabled == b.enabled;
  }

  static bool SameGroup(const Group& a, const Group& b) {
    if (!(a.rover == b.rover) || !(a.settings == b.settings) ||
        a.members.size() != b.members.size()) {
      return false;
    }

    for (std::size_t i = 0; i < a.members.size(); ++i) {
      if (!(a.members[i].object == b.members[i].object) ||
          !SameSensor(a.members[i].settings, b.members[i].settings)) {
        return false;
      }
    }

    return true;
  }

  bool AlignMount(pipeframe::SceneObject object,
    const UltrasonicSensorComponent& sensor,
    pipeframe::Transform2DComponent& pose) {
    if (!resolveObject || sensor.parent.objectId == 0 || !ValidSensor(sensor)) {
      return false;
    }

    const auto parent = resolveObject(sensor.parent.objectId);

    if (!parent.IsValid() || parent == object || !scene.IsActive(parent.GetEntity())) {
      return false;
    }

    const auto* parentPose = parent.GetComponent<pipeframe::Transform2DComponent>();

    if (!parentPose || !std::isfinite(parentPose->position.x) ||
        !std::isfinite(parentPose->position.y) || !std::isfinite(parentPose->rotation) ||
        !std::isfinite(parentPose->scale.x) || !std::isfinite(parentPose->scale.y) ||
        parentPose->scale.x <= 0 || parentPose->scale.y <= 0) {
      return false;
    }

    const double c = std::cos(parentPose->rotation);
    const double s = std::sin(parentPose->rotation);

    const double x = sensor.localPosition.x * parentPose->scale.x;

    const double y = sensor.localPosition.y * parentPose->scale.y;

    pose.position = {
      static_cast<float>(parentPose->position.x + x * c - y * s),
      static_cast<float>(parentPose->position.y + x * s + y * c),
    };

    pose.rotation = static_cast<float>(
      parentPose->rotation + sensor.localAngleDegrees * std::numbers::pi / 180.0);

    return std::isfinite(pose.position.x) && std::isfinite(pose.position.y) &&
           std::isfinite(pose.rotation);
  }

  void Invalidate(pipeframe::SceneObjectId roverId, const Group& group) {
    // Invalidations are immediate. A disabled or reconfigured sensor
    // must not leave an old "clear" reading usable during delivery delay.
    for (const auto& member : group.members) {
      SensorReadingEvent unavailable;
      unavailable.sensor = member.object.GetEntity();
      unavailable.roverId = roverId;
      unavailable.capturedAt = simulationTime;
      unavailable.simulationTime = simulationTime;
      unavailable.status = SensorReadingStatus::Unavailable;

      latest.erase(unavailable.sensor);
      events.Publish(unavailable);
    }

    // The caller removes/replaces the group, discarding its queue.
  }

  void CaptureDue(
    pipeframe::SceneObjectId roverId, Group& group, pipeframe::EnvironmentQueries& environment) {
    if (!group.settings.sequential) {
      for (auto& member : group.members) {
        if (simulationTime + TimeTolerance >= member.nextSample) {
          Capture(roverId, group, member, environment);
        }
      }

      return;
    }

    if (simulationTime + TimeTolerance < group.nextTrigger) {
      return;
    }

    const auto count = group.members.size();

    for (std::size_t offset = 0; offset < count; ++offset) {
      const auto index = (group.cursor + offset) % count;
      auto& member = group.members[index];

      if (simulationTime + TimeTolerance < member.nextSample) {
        continue;
      }

      Capture(roverId, group, member, environment);

      group.cursor = (index + 1) % count;
      group.nextTrigger = simulationTime + group.settings.slotPeriod;

      // At most one capture per rover per fixed step.
      // Never manufacture several old samples from today's pose.
      return;
    }
  }

  void Capture(pipeframe::SceneObjectId roverId,
    Group& group,
    Member& member,
    pipeframe::EnvironmentQueries& environment) {
    const auto* pose = member.object.GetComponent<pipeframe::Transform2DComponent>();

    if (!pose) {
      return;
    }

    const auto capture = member.captureNumber++;

    const double dropoutDraw =
      RandomUnit(group.settings.randomSeed, roverId, member.randomStream, capture, 0);

    const double noiseDraw =
      RandomUnit(group.settings.randomSeed, roverId, member.randomStream, capture, 1);

    auto reading = Measure(roverId, member, *pose, environment);

    ApplyMeasurementEffects(reading, member.settings, group.settings, dropoutDraw, noiseDraw);

    group.pending.push_back({
      std::move(reading),
      simulationTime + group.settings.deliveryDelay,
    });

    member.nextSample = simulationTime + group.settings.samplePeriod;
  }

  SensorReadingEvent Measure(pipeframe::SceneObjectId roverId,
    const Member& member,
    const pipeframe::Transform2DComponent& pose,
    pipeframe::EnvironmentQueries& environment) const {
    const auto& sensor = member.settings;

    SensorReadingEvent reading;
    reading.sensor = member.object.GetEntity();
    reading.roverId = roverId;
    reading.capturedAt = simulationTime;
    reading.origin = pose.position;

    constexpr auto mask = std::numeric_limits<std::uint32_t>::max();

    const double halfCone = sensor.coneAngleDegrees * std::numbers::pi / 360.0;

    float nearest = std::numeric_limits<float>::infinity();

    for (int ray = 0; ray < SensorReadingEvent::RayCount; ++ray) {
      const double fraction = double(ray) / double(SensorReadingEvent::RayCount - 1);

      const double angle = pose.rotation - halfCone + fraction * 2.0 * halfCone;

      const pipeframe::Vector2f direction{
        static_cast<float>(std::cos(angle)),
        static_cast<float>(std::sin(angle)),
      };

      const float maximum = static_cast<float>(sensor.maximumRange);

      const auto hit =
        environment.RaycastEnvironment(pose.position, direction, maximum, mask, roverId);

      if (hit && (!std::isfinite(hit->distance) || hit->distance < 0 || hit->distance > maximum)) {
        // A malformed query result is not evidence of clear space.
        reading.status = SensorReadingStatus::Unavailable;
        return reading;
      }

      reading.rangeEndpoints[ray] = pose.position + direction * maximum;

      reading.endpoints[ray] = pose.position + direction * (hit ? hit->distance : maximum);

      reading.hits[ray] = hit.has_value();

      if (hit && hit->distance < nearest) {
        nearest = hit->distance;
        reading.nearestRay = ray;
      }
    }

    reading.valid = true;
    reading.tooClose = std::isfinite(nearest) && nearest < sensor.minimumRange;

    if (!std::isfinite(nearest)) {
      reading.distance = sensor.maximumRange;
      reading.status = SensorReadingStatus::OutOfRange;
    } else if (reading.tooClose) {
      reading.distance = 0;
      reading.status = SensorReadingStatus::BelowMinimumRange;
    } else {
      reading.distance = nearest;
      reading.status = SensorReadingStatus::Hit;
    }

    return reading;
  }

  void DeliverDue(Group& group) {
    // All queued readings in a group share one delay. Captures were
    // appended in time order, so deliveries also remain ordered.
    std::size_t delivered = 0;

    while (delivered < group.pending.size()) {
      auto& pending = group.pending[delivered];

      if (simulationTime + TimeTolerance < pending.deliverAt) {
        break;
      }

      // Record actual publication time, including fixed-step rounding.
      pending.reading.simulationTime = simulationTime;

      latest.insert_or_assign(pending.reading.sensor, pending.reading);

      events.Publish(pending.reading);
      ++delivered;
    }

    group.pending.erase(group.pending.begin(), group.pending.begin() + delivered);
  }

  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  Resolver resolveObject;

  double simulationTime{};

  std::map<pipeframe::SceneObjectId, Group> groups;

  std::unordered_map<pipeframe::ecs::Entity, SensorReadingEvent> latest;
};

}  // namespace RoverLab

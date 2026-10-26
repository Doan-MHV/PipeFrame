#pragma once

#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Environment/EnvironmentQueries.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <map>
#include <numbers>

#include "Events/SensorReadingEvent.h"
#include "RangeMount.h"

namespace RoverLab {
// Ideal short-range optical reflection sensor. No acoustic echo delay or shared
// sonar schedule. Surface reflectance, ambient light and ADC calibration are not
// simulated; range noise and sampling cadence are explicit authoring settings.
class InfraredSensorSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;
  InfraredSensorSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events) {}
  std::string_view GetSystemId() const override { return "roverlab.infrared-sampling"; }
  void SetObjectResolver(Resolver value) {
    resolve = std::move(value);
    Reset();
  }
  void Reset() {
    states.clear();
    time = 0;
  }
  const SensorReadingEvent* Latest(pipeframe::ecs::Entity id) const {
    const auto found = states.find(id);
    return found == states.end() ? nullptr : &found->second.reading;
  }
  void AlignMounts() {
    scene.Components().Each<InfraredSensorComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity id) { Align(scene.GetObject(id)); });
  }
  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) return;
    time += delta;
    auto* environment = scene.Services().Find<pipeframe::EnvironmentQueries>();
    std::erase_if(states, [&](const auto& pair) {
      return !pair.second.object.IsValid() ||
             !pair.second.object.template GetComponent<InfraredSensorComponent>();
    });
    scene.Components().Each<InfraredSensorComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity id) {
        const auto object = scene.GetObject(id);
        const auto& config = *object.GetComponent<InfraredSensorComponent>();
        auto& state = states[id];
        if (state.object != object || state.settings != config) {
          state = {};
          state.object = object;
          state.settings = config;
        }
        if (!environment || !scene.IsActive(id) || !config.enabled || !Align(object)) {
          SensorReadingEvent missing;
          missing.sensor = id;
          missing.roverId = config.parent.objectId;
          missing.capturedAt = missing.simulationTime = time;
          state.reading = missing;
          state.next = time;
          events.Publish(missing);
          return;
        }
        if (time + 1e-7 < state.next) return;
        // Keep the configured phase when a sample interval is not an exact
        // multiple of the fixed step. Capture once at the current pose; never
        // fabricate historical samples after a long frame.
        const double periods =
          std::max(1.0, std::floor((time - state.next + 1e-7) / config.samplePeriod) + 1);
        state.next += periods * config.samplePeriod;
        auto reading = Measure(object, config, *environment);
        if (reading.status == SensorReadingStatus::Hit && config.noiseAmplitude > 0) {
          std::uint64_t bits = (++state.capture * 0x9e3779b97f4a7c15ull) ^
                               (std::uint64_t(config.randomSeed) * 0x94d049bb133111ebull) ^
                               std::uint64_t(id);
          bits ^= bits >> 30;
          bits *= 0xbf58476d1ce4e5b9ull;
          bits ^= bits >> 27;
          const double unit = double(bits >> 11) / 9007199254740992.0;
          reading.distance = std::clamp(reading.distance + (unit * 2 - 1) * config.noiseAmplitude,
            config.minimumRange,
            config.maximumRange);
        }
        state.reading = reading;
        events.Publish(reading);
      });
  }

private:
  bool Align(pipeframe::SceneObject object) {
    const auto* config = object.GetComponent<InfraredSensorComponent>();
    auto* pose = object.GetComponent<pipeframe::Transform2DComponent>();
    if (!config || !pose || !config->Valid() || !resolve) return false;
    const auto parent = resolve(config->parent.objectId);
    if (!parent.IsValid() || parent == object || !scene.IsActive(parent.GetEntity())) return false;
    const auto* body = parent.GetComponent<pipeframe::Transform2DComponent>();
    if (!body || !std::isfinite(body->rotation) || !std::isfinite(body->position.x) ||
        !std::isfinite(body->position.y) || body->scale.x != 1 || body->scale.y != 1)
      return false;
    const double c = std::cos(body->rotation), s = std::sin(body->rotation);
    pose->position = {
      float(body->position.x + c * config->localPosition.x - s * config->localPosition.y),
      float(body->position.y + s * config->localPosition.x + c * config->localPosition.y)};
    pose->rotation = float(body->rotation + config->localAngleDegrees * std::numbers::pi / 180);
    return true;
  }
  SensorReadingEvent Measure(pipeframe::SceneObject object,
    const InfraredSensorComponent& config,
    pipeframe::EnvironmentQueries& environment) const {
    const auto& pose = *object.GetComponent<pipeframe::Transform2DComponent>();
    SensorReadingEvent result;
    result.sensor = object.GetEntity();
    result.roverId = config.parent.objectId;
    result.capturedAt = result.simulationTime = time;
    result.origin = pose.position;
    const double halfAngle = config.coneAngleDegrees * std::numbers::pi / 360;
    double nearest = std::numeric_limits<double>::infinity();
    for (int ray = 0; ray < SensorReadingEvent::RayCount; ++ray) {
      const double angle =
        pose.rotation - halfAngle + 2 * halfAngle * ray / (SensorReadingEvent::RayCount - 1);
      const pipeframe::Vector2f direction{float(std::cos(angle)), float(std::sin(angle))};
      const auto hit = environment.RaycastEnvironment(pose.position,
        direction,
        float(config.maximumRange),
        std::numeric_limits<std::uint32_t>::max(),
        config.parent.objectId);
      if (hit && (!std::isfinite(hit->distance) || hit->distance < 0 ||
                   hit->distance > config.maximumRange))
        return result;
      result.hits[ray] = hit.has_value();
      result.rangeEndpoints[ray] = pose.position + direction * float(config.maximumRange);
      result.endpoints[ray] =
        pose.position + direction * (hit ? hit->distance : float(config.maximumRange));
      if (hit && hit->distance < nearest) {
        nearest = hit->distance;
        result.nearestRay = ray;
      }
    }
    result.valid = true;
    result.tooClose = nearest < config.minimumRange;
    result.distance = result.tooClose ? 0 : std::min(nearest, config.maximumRange);
    result.status = result.tooClose          ? SensorReadingStatus::BelowMinimumRange
                    : std::isfinite(nearest) ? SensorReadingStatus::Hit
                                             : SensorReadingStatus::OutOfRange;
    return result;
  }
  struct State {
    pipeframe::SceneObject object;
    InfraredSensorComponent settings;
    double next{};
    std::uint64_t capture{};
    SensorReadingEvent reading;
  };
  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  Resolver resolve;
  std::map<pipeframe::ecs::Entity, State> states;
  double time{};
};
}  // namespace RoverLab

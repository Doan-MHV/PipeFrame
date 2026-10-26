#pragma once

#include <PipeFrame/Components/EnvironmentCollider2DComponent.h>
#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Physics/ShapeQueries2D.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <string>
#include <unordered_map>
#include <utility>

#include "Components/MicromouseHardwareComponent.h"
#include "Components/RoverDriveComponent.h"
#include "Components/RoverSafetyComponent.h"
#include "Events/RoverMotionCommandEvent.h"
#include "Events/RoverSafetyEvent.h"
#include "Events/SensorReadingEvent.h"
#include "RoverRangeObservations.h"

namespace RoverLab {

class RoverDriveSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;

  RoverDriveSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events), ranges(scene, events) {
    motionConnection = events.Listen<RoverMotionCommandEvent>(
      [this](const RoverMotionCommandEvent& command) { OnMotionCommand(command); });
  }

  std::string_view GetSystemId() const override { return "roverlab.sensor-drive"; }

  void SetObjectResolver(Resolver resolver) {
    ranges.SetObjectResolver(resolver);
    resolveObject = std::move(resolver);
    Reset();
  }

  RoverSafetyStatus SafetyStatus(pipeframe::ecs::Entity entity) const {
    const auto found = safetyStates.find(entity);

    return found == safetyStates.end() ? RoverSafetyStatus::NoCommand : found->second;
  }

  std::string StopReason(pipeframe::ecs::Entity entity) const {
    const auto found = stopReasons.find(entity);
    return found == stopReasons.end() ? std::string(SafetyStatusText(SafetyStatus(entity)))
                                      : found->second;
  }

  void Reset() {
    ranges.Reset();
    commands.clear();
    safetyStates.clear();
    stopReasons.clear();
    appliedSpeeds.clear();
    simulationTime = 0;
  }

  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) return;

    simulationTime += delta;
    safetyStates.clear();
    stopReasons.clear();
    appliedSpeeds.clear();

    ranges.Update(delta);

    std::erase_if(commands, [&](const auto& entry) {
      const auto& command = entry.second;

      if (!command.rover.IsValid()) return true;

      const auto* drive = command.rover.template GetComponent<RoverDriveComponent>();

      return !drive || !drive->enabled || !drive->ValidSettings() ||
             !scene.IsActive(command.rover.GetEntity()) || command.age >= drive->commandTimeout;
    });

    auto& components = scene.Components();

    auto updateRover = [&](pipeframe::ecs::Entity entity) {
      // The query below guarantees these components exist.
      auto& drive = *components.Get<RoverDriveComponent>(entity);

      auto& body = *components.Get<pipeframe::KinematicBody2DComponent>(entity);

      auto& pose = *components.Get<pipeframe::Transform2DComponent>(entity);

      safetyStates[entity] = RoverSafetyStatus::NoCommand;

      const auto previousVelocity = body.velocity;
      const double previousTurn = body.angularVelocity;

      // Every failure path leaves the rover stopped.
      body.velocity = {};
      body.angularVelocity = 0;

      if (!drive.enabled || !body.enabled || !std::isfinite(body.radius) || body.radius <= 0 ||
          !scene.IsActive(entity) || !drive.ValidSettings()) {
        safetyStates[entity] = RoverSafetyStatus::Disabled;
        return;
      }

      const auto found = commands.find(entity);
      if (found == commands.end()) return;

      const auto& command = found->second;

      // Reject stale references, including reused entity IDs.
      if (!(command.rover == scene.GetObject(entity))) return;
      if (!std::isfinite(pose.rotation)) return;

      double forwardSpeed = command.forwardSpeed;
      double turnRate = command.turnRate;

      const auto* hardware = components.Get<MicromouseHardwareComponent>(entity);
      if (hardware && !hardware->Valid()) {
        safetyStates[entity] = RoverSafetyStatus::Disabled;
        stopReasons[entity] = "Invalid Micromouse Hardware settings";
        return;
      }
      if (hardware) {
        const double previousForward = previousVelocity.x * std::cos(pose.rotation) +
                                       previousVelocity.y * std::sin(pose.rotation);
        hardware->LimitAcceleration(previousForward, previousTurn, delta, forwardSpeed, turnRate);
      }

      // Positive turning is clockwise in the +Y-down convention.
      const double halfTrack = (hardware ? hardware->trackWidth : drive.trackWidth) * 0.5;
      const double wheelLimit = hardware ? std::min(drive.maximumWheelSpeed, hardware->WheelSpeed())
                                         : drive.maximumWheelSpeed;

      const double leftSpeed = forwardSpeed + turnRate * halfTrack;

      const double rightSpeed = forwardSpeed - turnRate * halfTrack;

      if (!std::isfinite(leftSpeed) || !std::isfinite(rightSpeed)) {
        return;
      }

      const double peakWheelSpeed = std::max(std::abs(leftSpeed), std::abs(rightSpeed));

      // Scale together to preserve command curvature.
      double scale = 1.0;

      if (std::abs(forwardSpeed) > drive.speed) {
        scale = std::min(scale, drive.speed / std::abs(forwardSpeed));
      }

      if (std::abs(turnRate) > drive.maximumTurnRate) {
        scale = std::min(scale, drive.maximumTurnRate / std::abs(turnRate));
      }

      if (peakWheelSpeed > wheelLimit) {
        scale = std::min(scale, wheelLimit / peakWheelSpeed);
      }

      forwardSpeed *= scale;
      turnRate *= scale;

      const auto status = EvaluateSafety(entity, drive, pose, forwardSpeed, turnRate, delta);

      safetyStates[entity] = status;

      if (status != RoverSafetyStatus::Clear && status != RoverSafetyStatus::Stopped) {
        return;
      }

      body.velocity = {static_cast<float>(forwardSpeed * std::cos(pose.rotation)),
        static_cast<float>(forwardSpeed * std::sin(pose.rotation))};

      body.angularVelocity = turnRate;
      appliedSpeeds[entity] = forwardSpeed;
    };

    components.Each<RoverDriveComponent,
      pipeframe::KinematicBody2DComponent,
      pipeframe::Transform2DComponent>(updateRover);

    for (const auto& [entity, status] : safetyStates) {
      RoverSafetyEvent event;
      event.simulationTime = simulationTime;
      event.rover = scene.GetObject(entity);
      event.status = status;
      event.reason = StopReason(entity);
      event.appliedForwardSpeed = appliedSpeeds[entity];

      events.Publish(event);
    }

    // A command published earlier this tick is fresh when consumed.
    for (auto& [entity, command] : commands) {
      command.age += delta;
    }
  }

private:
  struct MotionCommand {
    pipeframe::SceneObject rover;

    double age{};
    double forwardSpeed{};
    double turnRate{};
  };

  void OnMotionCommand(const RoverMotionCommandEvent& event) {
    if (!resolveObject) return;

    auto rover = resolveObject(event.roverId);
    if (!rover.IsValid()) return;

    const auto entity = rover.GetEntity();

    // An invalid new request cancels the previous request.
    commands.erase(entity);

    const auto* drive = rover.GetComponent<RoverDriveComponent>();

    if (!drive || !drive->enabled || !drive->ValidSettings() || !scene.IsActive(entity)) {
      return;
    }

    if (!std::isfinite(event.forwardSpeed) || !std::isfinite(event.turnRate)) {
      return;
    }

    commands[entity] = {rover, 0.0, event.forwardSpeed, event.turnRate};
  }

  static double AngleDifference(double a, double b) {
    return std::abs(std::remainder(a - b, 2.0 * std::numbers::pi));
  }

  static bool ConeContains(double centre, double halfCone, double direction) {
    return AngleDifference(centre, direction) <= halfCone + 1e-9;
  }

  static double RequiredDistance(const RangeMount& sensor,
    const RoverSafetyComponent& safety,
    const pipeframe::Transform2DComponent& pose,
    const RoverDriveComponent& drive,
    double forwardSpeed,
    double turnAngle) {
    constexpr double pi = std::numbers::pi;
    const double angle = sensor.localAngleDegrees * pi / 180.0;
    const double halfCone = sensor.coneAngleDegrees * pi / 360.0;
    const double length = safety.halfLength * pose.scale.x;
    const double width = safety.halfWidth * pose.scale.y;
    const double x = sensor.localPosition.x * pose.scale.x;
    const double y = sensor.localPosition.y * pose.scale.y;

    // Bound the rectangle over the requested angular interval.
    // A small correction does not require space for a complete spin.
    const double sweep = std::min(std::abs(turnAngle), pi / 2);
    const double radius = std::hypot(length, width);
    const double sweptLength = sweep >= std::atan2(width, length)
                                 ? radius
                                 : length * std::cos(sweep) + width * std::sin(sweep);
    const double sweptWidth = sweep >= std::atan2(length, width)
                                ? radius
                                : width * std::cos(sweep) + length * std::sin(sweep);

    // Extend the stopping envelope only in the requested travel direction.
    const double minX =
      -sweptLength - safety.clearance - (forwardSpeed < 0 ? drive.stopDistance : 0.0);
    const double maxX =
      sweptLength + safety.clearance + (forwardSpeed > 0 ? drive.stopDistance : 0.0);
    const double minY = -sweptWidth - safety.clearance;
    const double maxY = sweptWidth + safety.clearance;

    auto exitDistance = [&](double bearing) {
      double enter = 0;
      double exit = std::numeric_limits<double>::infinity();
      auto slab = [&](double origin, double direction, double low, double high) {
        if (std::abs(direction) < 1e-12) {
          return origin >= low && origin <= high;
        }
        double first = (low - origin) / direction;
        double last = (high - origin) / direction;
        if (first > last) std::swap(first, last);
        enter = std::max(enter, first);
        exit = std::min(exit, last);
        return enter <= exit;
      };
      if (!slab(x, std::cos(bearing), minX, maxX) || !slab(y, std::sin(bearing), minY, maxY)) {
        return 0.0;
      }
      return std::max(0.0, exit);
    };

    // The echo bearing is unknown. Bound every bearing in its cone.
    // A rectangle's farthest radial intersection occurs at a cone edge
    // or a visible corner; no debug rays or hidden map data are needed.
    double required = std::max(exitDistance(angle - halfCone), exitDistance(angle + halfCone));
    for (double cornerX : {minX, maxX}) {
      for (double cornerY : {minY, maxY}) {
        const double bearing = std::atan2(cornerY - y, cornerX - x);
        if (ConeContains(angle, halfCone, bearing)) {
          required = std::max(required, exitDistance(bearing));
        }
      }
    }
    return required + 0.0001;  // Numerical contact tolerance, in metres.
  }

  RoverSafetyStatus EvaluateSafety(pipeframe::ecs::Entity roverEntity,
    const RoverDriveComponent& drive,
    const pipeframe::Transform2DComponent& pose,
    double forwardSpeed,
    double turnRate,
    double delta) {
    // Stop requests never wait for sensor observations.
    if (forwardSpeed == 0 && turnRate == 0) {
      return RoverSafetyStatus::Stopped;
    }

    auto& components = scene.Components();

    const auto* safety = components.Get<RoverSafetyComponent>(roverEntity);

    if (!safety || !safety->ValidSettings() || !resolveObject || !std::isfinite(pose.scale.x) ||
        !std::isfinite(pose.scale.y) || pose.scale.x <= 0 || pose.scale.y <= 0) {
      return RoverSafetyStatus::MissingConfiguration;
    }

    // Safety dimensions may be conservative, but must never be smaller
    // than the actual collider around its rotation pivot. Derive in local
    // units; RequiredDistance applies the rover scale once.
    auto footprint = *safety;
    const auto* collider = components.Get<pipeframe::EnvironmentCollider2DComponent>(roverEntity);
    if (collider && collider->enabled && !collider->trigger) {
      const auto shape = pipeframe::ColliderShape(*collider, pipeframe::Transform2DComponent{});
      for (const auto& vertex : shape.points) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y)) {
          return RoverSafetyStatus::MissingConfiguration;
        }
        footprint.halfLength = std::max(footprint.halfLength, std::abs(double(vertex.x)));
        footprint.halfWidth = std::max(footprint.halfWidth, std::abs(double(vertex.y)));
      }
    }

    // Physics uses the body's world-space fallback radius unless an
    // enabled, non-trigger box replaces it. Convert to local units so the
    // envelope below applies scale exactly once.
    if (!collider || !collider->enabled || collider->trigger || collider->shape != "Box") {
      const auto& body = *components.Get<pipeframe::KinematicBody2DComponent>(roverEntity);
      footprint.halfLength = std::max(footprint.halfLength, double(body.radius) / pose.scale.x);
      footprint.halfWidth = std::max(footprint.halfWidth, double(body.radius) / pose.scale.y);
    }

    const bool turning = turnRate != 0;
    const bool forward = forwardSpeed > 0;

    // Local directions:
    // +X front, +Y right, -X rear, -Y left.
    constexpr unsigned front = 1;
    constexpr unsigned right = 2;
    constexpr unsigned rear = 4;
    constexpr unsigned left = 8;

    const unsigned requiredCoverage =
      turning ? front | right | rear | left : (forward ? front : rear);

    unsigned coverage = 0;
    auto result = RoverSafetyStatus::Clear;

    auto checkSensor = [&](const RangeObservation& reading) {
      const auto& sensor = reading.settings;
      const auto rover = resolveObject(sensor.parent.objectId);

      if (!rover.IsValid() || rover.GetEntity() != roverEntity) {
        return;
      }

      auto fail = [&](RoverSafetyStatus status) {
        if (result != RoverSafetyStatus::Clear) return;
        result = status;
        const double angle = sensor.localAngleDegrees;
        const char* direction = !std::isfinite(angle)                          ? "Sensor"
                                : std::abs(std::remainder(angle, 360.0)) <= 45 ? "Front sensor"
                                : std::abs(std::remainder(angle - 180, 360.0)) <= 45 ? "Rear sensor"
                                : std::remainder(angle, 360.0) < 0 ? "Left sensor"
                                                                   : "Right sensor";
        stopReasons[roverEntity] =
          std::string(direction) + ": " + std::string(SafetyStatusText(status));
      };

      const bool validSettings =
        std::isfinite(sensor.localAngleDegrees) && std::isfinite(sensor.localPosition.x) &&
        std::isfinite(sensor.localPosition.y) && std::isfinite(sensor.coneAngleDegrees) &&
        sensor.coneAngleDegrees > 0 && sensor.coneAngleDegrees <= 90 &&
        std::isfinite(sensor.minimumRange) && sensor.minimumRange > 0 &&
        std::isfinite(sensor.maximumRange) && sensor.maximumRange > sensor.minimumRange;

      if (!validSettings) {
        fail(RoverSafetyStatus::InvalidReading);
        return;
      }

      constexpr double pi = std::numbers::pi;

      const double angle = sensor.localAngleDegrees * pi / 180.0;

      const double halfCone = sensor.coneAngleDegrees * pi / 360.0;

      // Ignore only cones facing strictly away from straight travel.
      // Side cones still protect chassis clearance; turning needs all four.
      const bool rearOnly = AngleDifference(angle, pi) + halfCone < pi / 2;

      const bool frontOnly = AngleDifference(angle, 0) + halfCone < pi / 2;
      if (!turning && (forward ? rearOnly : frontOnly)) return;

      if (ConeContains(angle, halfCone, 0)) {
        coverage |= front;
      }

      if (ConeContains(angle, halfCone, pi / 2)) {
        coverage |= right;
      }

      if (ConeContains(angle, halfCone, pi)) {
        coverage |= rear;
      }

      if (ConeContains(angle, halfCone, -pi / 2)) {
        coverage |= left;
      }

      const double required = RequiredDistance(sensor,
        footprint,
        pose,
        drive,
        forwardSpeed,
        turnRate * std::max(delta, drive.readingTimeout));

      // OutOfRange proves clearance only up to sensor range.
      if (!std::isfinite(required) || sensor.maximumRange <= required) {
        fail(RoverSafetyStatus::InvalidReading);
        return;
      }

      if (!reading.present) {
        fail(RoverSafetyStatus::WaitingForReading);
        return;
      }
      if (reading.age >= drive.readingTimeout) {
        fail(RoverSafetyStatus::StaleReading);
        return;
      }

      if (!reading.valid) {
        fail(RoverSafetyStatus::InvalidReading);
        return;
      }

      if (reading.status == SensorReadingStatus::BelowMinimumRange) {
        fail(RoverSafetyStatus::Obstacle);
      } else if (reading.status == SensorReadingStatus::Hit) {
        if (reading.distance <= required) {
          fail(RoverSafetyStatus::Obstacle);
        }
      } else if (reading.status != SensorReadingStatus::OutOfRange) {
        fail(RoverSafetyStatus::InvalidReading);
      }
    };

    ranges.Each(checkSensor, true);

    if ((coverage & requiredCoverage) != requiredCoverage) {
      stopReasons[roverEntity] =
        turning ? "Turning requires front, rear, left and right sensors"
                : (forward ? "Front sensor coverage missing" : "Rear sensor coverage missing");
      return RoverSafetyStatus::MissingCoverage;
    }

    return result;
  }

  pipeframe::BehaviourScene& scene;
  Resolver resolveObject;
  pipeframe::EventChannels& events;

  double simulationTime{};
  std::unordered_map<pipeframe::ecs::Entity, std::string> stopReasons;
  std::unordered_map<pipeframe::ecs::Entity, double> appliedSpeeds;

  std::unordered_map<pipeframe::ecs::Entity, RoverSafetyStatus> safetyStates;

  RoverRangeObservations ranges;

  std::unordered_map<pipeframe::ecs::Entity, MotionCommand> commands;

  // Disconnect callbacks before the other members are destroyed.
  pipeframe::EventSubscription motionConnection;
};

}  // namespace RoverLab

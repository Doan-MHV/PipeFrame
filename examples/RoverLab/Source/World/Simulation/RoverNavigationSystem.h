#pragma once

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <string>
#include <unordered_map>
#include <utility>

#include "Components/RoverDriveComponent.h"
#include "Components/RoverNavigationComponent.h"
#include "Events/RoverMotionCommandEvent.h"
#include "Events/RoverOdometryEvent.h"
#include "Events/RoverSafetyEvent.h"
#include "Events/SensorReadingEvent.h"
#include "RoverRangeObservations.h"

namespace RoverLab {

class RoverNavigationSystem final : public pipeframe::FixedUpdateSystem<void> {
  enum class Mode {
    Follow,
    Turn,
    Settle,
    Advance,
    Retry,
    Reverse,
  };

  enum class Request {
    Stop,
    Straight,
    Reverse,
    Curve,
    Turn,
  };

  struct Pose {
    RoverOdometryEvent event;
    double age{};
  };

  struct Safety {
    RoverSafetyEvent event;
    double age{};
  };

  struct Observation {
    bool present{};
    bool hit{};
    double distance{};
    std::uint64_t sequence{};
    double capturedAt{};
  };

  struct State {
    pipeframe::SceneObject rover;
    pipeframe::SceneObject controller;

    Mode mode{Mode::Follow};
    Request last{Request::Stop};

    double timer{};
    double blockedTime{};
    double targetHeading{};
    double advanceUntil{};
    double advanceSpeed{};
    bool reverseUsed{};
    double reverseUntil{};
    double progressDistance{};
    double progressTimer{};
    std::string stopReason;

    bool corridorReady{};
    double previousSideError{};
    double sideDerivative{};

    std::uint64_t cutoff{};
    double settleAfter{};

    // Rejected directions use the odometry heading frame.
    std::array<double, 8> rejected{};
    std::size_t rejectedCount{};

    bool memoryReady{};
    bool waiting{};

    double memoryX{};
    double memoryY{};
    double waitHeading{};

    std::array<double, 4> waitDistances{};

    std::string message{"Waiting for observations"};
  };

public:
  using Resolver = std::function<pipeframe::SceneObject(pipeframe::SceneObjectId)>;

  RoverNavigationSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events), ranges(scene, events) {
    poseConnection = events.Listen<RoverOdometryEvent>([this](const auto& event) {
      if (event.rover.IsValid()) {
        poses[event.rover.GetEntity()] = {event, 0};
      }
    });

    safetyConnection = events.Listen<RoverSafetyEvent>([this](const auto& event) {
      if (event.rover.IsValid()) {
        safety[event.rover.GetEntity()] = {event, 0};
      }
    });
  }

  std::string_view GetSystemId() const override { return "roverlab.navigation"; }

  void SetObjectResolver(Resolver value) {
    ranges.SetObjectResolver(value);
    resolveObject = std::move(value);
    Reset();
  }

  void Reset() {
    ranges.Reset();
    poses.clear();
    safety.clear();
    states.clear();

    time = 0;
  }

  std::string Status(pipeframe::ecs::Entity entity) const {
    const auto found = states.find(entity);

    return found == states.end() ? "Navigation inactive" : found->second.message;
  }

  std::string StopReason(pipeframe::ecs::Entity entity) const {
    const auto found = states.find(entity);
    return found == states.end() ? "" : found->second.stopReason;
  }

  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) {
      return;
    }

    time += delta;

    ranges.Update(delta);

    std::erase_if(poses, [](const auto& entry) { return !entry.second.event.rover.IsValid(); });

    std::erase_if(safety, [](const auto& entry) { return !entry.second.event.rover.IsValid(); });

    std::erase_if(states, [&](const auto& entry) {
      const auto& state = entry.second;

      const auto* config = state.controller.template GetComponent<RoverNavigationComponent>();

      return !state.rover.IsValid() || !state.controller.IsValid() || !config || !config->enabled ||
             !scene.IsActive(state.controller.GetEntity()) || !resolveObject ||
             !(resolveObject(config->rover.objectId) == state.rover);
    });

    for (auto& [id, pose] : poses) {
      pose.age += delta;
    }

    for (auto& [id, decision] : safety) {
      decision.age += delta;
    }

    auto& components = scene.Components();

    auto navigate = [&](pipeframe::ecs::Entity entity) {
      const auto& config = *components.Get<RoverNavigationComponent>(entity);

      if (!config.enabled || !scene.IsActive(entity) || !resolveObject) {
        return;
      }

      const auto rover = resolveObject(config.rover.objectId);

      if (!rover.IsValid() || !scene.IsActive(rover.GetEntity())) {
        return;
      }

      auto& state = states[rover.GetEntity()];

      if (!(state.rover == rover) || !(state.controller == scene.GetObject(entity))) {
        state = State{};
        state.rover = rover;
        state.controller = scene.GetObject(entity);
      }

      auto send = [&](double speed, double turn, std::string message) {
        RoverMotionCommandEvent event;
        event.simulationTime = time;
        event.roverId = config.rover.objectId;
        event.forwardSpeed = speed;
        event.turnRate = turn;

        state.last = speed < 0   ? Request::Reverse
                     : speed > 0 ? (turn == 0 ? Request::Straight : Request::Curve)
                                 : (turn == 0 ? Request::Stop : Request::Turn);

        state.stopReason = speed == 0 && turn == 0 ? message : "";
        state.message = std::move(message);
        events.Publish(event);
      };

      auto retry = [&](std::string message) {
        state.corridorReady = false;
        state.mode = Mode::Retry;
        state.timer = 0;
        state.blockedTime = 0;

        send(0, 0, std::move(message));
      };

      const auto* drive = rover.GetComponent<RoverDriveComponent>();

      int controllers = 0;

      auto countControllers = [&](pipeframe::ecs::Entity candidate) {
        const auto& other = *components.Get<RoverNavigationComponent>(candidate);

        if (scene.IsActive(candidate) && other.enabled &&
            other.rover.objectId == config.rover.objectId) {
          ++controllers;
        }
      };

      components.Each<RoverNavigationComponent>(countControllers);

      if (controllers != 1) {
        retry("Conflicting navigation controllers");
        return;
      }

      if (!config.Valid() || !drive || !drive->enabled || !drive->ValidSettings() ||
          config.speed > drive->speed || config.turnRate > drive->maximumTurnRate ||
          config.turnDistance <= drive->stopDistance) {
        retry("Check navigation and drive settings");
        return;
      }

      std::array<Observation, 4> observations{};

      if (!Observe(config.rover.objectId, rover, drive->readingTimeout, observations)) {
        retry("Waiting for four fresh, usable sensor directions");
        return;
      }

      const auto foundPose = poses.find(rover.GetEntity());

      if (foundPose == poses.end() || foundPose->second.age >= drive->readingTimeout ||
          !(foundPose->second.event.rover == rover) ||
          !std::isfinite(foundPose->second.event.heading) ||
          !std::isfinite(foundPose->second.event.distance)) {
        retry("Waiting for fresh odometry");
        return;
      }

      const auto& pose = foundPose->second.event;

      if (!std::isfinite(pose.x) || !std::isfinite(pose.y)) {
        retry("Invalid odometry position");
        return;
      }

      // Clear local failure memory after meaningful translation.
      // Rotation alone does not reset it.
      if (!state.memoryReady || std::hypot(pose.x - state.memoryX, pose.y - state.memoryY) >= .35) {
        state.memoryReady = true;
        state.memoryX = pose.x;
        state.memoryY = pose.y;

        state.rejectedCount = 0;
        state.waiting = false;
        if (state.last == Request::Straight || state.last == Request::Curve) {
          state.reverseUsed = false;
        }
      }

      auto rejected = [&](double heading) {
        for (std::size_t i = 0; i < state.rejectedCount; ++i) {
          const double difference =
            std::abs(std::remainder(heading - state.rejected[i], 2 * std::numbers::pi));

          if (difference < std::numbers::pi / 4) {
            return true;
          }
        }

        return false;
      };

      auto reject = [&](double heading) {
        if (!rejected(heading) && state.rejectedCount < state.rejected.size()) {
          state.rejected[state.rejectedCount++] = heading;
        }
      };

      // Reconsider failures when clearance improves while stopped
      // at approximately the same heading.
      if (state.waiting &&
          std::abs(std::remainder(pose.heading - state.waitHeading, 2 * std::numbers::pi)) < .08) {
        for (std::size_t i = 0; i < observations.size(); ++i) {
          if (observations[i].distance > state.waitDistances[i] + .20) {
            state.rejectedCount = 0;
            state.waiting = false;
            state.reverseUsed = false;
            break;
          }
        }
      }

      auto waitForChange = [&](std::string reason =
                                 "No untried safe direction; waiting for clearance change") {
        if (!state.waiting) {
          state.waiting = true;
          state.waitHeading = pose.heading;

          for (std::size_t i = 0; i < observations.size(); ++i) {
            state.waitDistances[i] = observations[i].distance;
          }
        }

        retry(std::move(reason));
      };

      if (state.waiting) {
        send(0, 0, state.message);
        return;
      }

      const auto decision = safety.find(rover.GetEntity());

      const bool obstacle = decision != safety.end() &&
                            decision->second.age < drive->readingTimeout &&
                            decision->second.event.rover == rover &&
                            decision->second.event.status == RoverSafetyStatus::Obstacle;

      const double front = observations[0].distance;

      const bool forwardCandidate = front > drive->stopDistance + .10 && !rejected(pose.heading);

      // Side echoes can shorten the front cone's reading even when
      // forward motion is safe. In a corridor, let Drive evaluate the
      // swept footprint; a rejected straight command still triggers
      // the existing passage-selection logic below.
      const bool corridor = observations[1].hit && observations[2].hit &&
                            observations[1].distance < config.passageDistance &&
                            observations[2].distance < config.passageDistance;
      const bool frontBlocked =
        (!corridor && front <= config.turnDistance) || rejected(pose.heading);

      state.timer += delta;

      state.blockedTime = obstacle && state.last != Request::Stop ? state.blockedTime + delta : 0;

      auto advance = [&](std::string message, bool recovery = true) {
        state.waiting = false;
        state.corridorReady = false;
        state.mode = Mode::Advance;
        state.timer = 0;
        state.blockedTime = 0;

        state.advanceUntil = pose.distance + (recovery ? .12 : .25);

        state.advanceSpeed = recovery ? std::min(config.speed, .05) : config.speed;

        send(state.advanceSpeed, 0, std::move(message));
      };

      auto reverse = [&] {
        if (state.reverseUsed) {
          waitForChange("Reverse recovery exhausted here; waiting for clearance change");
          return;
        }
        // This is only a candidate. Drive independently checks the rear
        // stopping envelope and both sides on every reverse command.
        if (observations[3].distance <= drive->stopDistance + .10) {
          waitForChange("Reverse unavailable: rear sensor reports insufficient clearance");
          return;
        }
        state.reverseUsed = true;
        state.corridorReady = false;
        state.mode = Mode::Reverse;
        state.timer = 0;
        state.progressTimer = 0;
        state.progressDistance = pose.distance;
        state.reverseUntil = pose.distance + .25;
        send(
          -std::min(config.speed, .05), 0, "Backing away from blocked turn (rear safety active)");
      };

      if (state.mode == Mode::Reverse) {
        if (decision != safety.end() && decision->second.age < drive->readingTimeout &&
            decision->second.event.rover == rover &&
            decision->second.event.status != RoverSafetyStatus::Clear) {
          waitForChange("Reverse stopped: " + decision->second.event.reason);
          return;
        }
        state.progressTimer += delta;
        if (pose.distance - state.progressDistance >= .005) {
          state.progressDistance = pose.distance;
          state.progressTimer = 0;
        }
        if (pose.distance >= state.reverseUntil) {
          state.rejectedCount = 0;
          state.mode = Mode::Settle;
          state.timer = 0;
          state.cutoff = ranges.Sequence();
          state.settleAfter = time;
          send(0, 0, "Reverse complete; reassessing fresh sensor readings");
        } else if (state.progressTimer >= 1 || state.timer >= 6) {
          waitForChange("Reverse stopped: no progress or recovery time limit reached");
        } else {
          send(
            -std::min(config.speed, .05), 0, "Backing away from blocked turn (rear safety active)");
        }
        return;
      }

      if (state.mode == Mode::Retry) {
        if (state.timer < .5) {
          send(0, 0, state.message);
          return;
        }

        state.mode = Mode::Follow;
        state.timer = 0;
      }

      if (state.mode == Mode::Turn) {
        const double error =
          std::remainder(state.targetHeading - pose.heading, 2 * std::numbers::pi);

        if (std::abs(error) <= .04) {
          state.mode = Mode::Settle;
          state.timer = 0;
          state.cutoff = ranges.Sequence();
          state.settleAfter = time;

          send(0, 0, "Turn complete; waiting for new readings");
        } else if (state.blockedTime >= .25 || state.timer >= 6) {
          // Remember the intended heading, even if the
          // rotation only completed partway.
          reject(state.targetHeading);

          if (forwardCandidate) {
            advance("Turn blocked; trying straight clearance");
          } else {
            reverse();
          }
        } else {
          send(
            0, std::clamp(2 * error, -config.turnRate, config.turnRate), "Turning toward passage");
        }

        return;
      }

      if (state.mode == Mode::Settle) {
        for (const auto& observation : observations) {
          if (observation.sequence <= state.cutoff || observation.capturedAt < state.settleAfter) {
            if (state.timer > drive->readingTimeout + .5) {
              retry("Post-turn readings unavailable");
            } else {
              send(0, 0, "Waiting for post-turn readings");
            }

            return;
          }
        }

        if (forwardCandidate) {
          advance("Advancing into passage", false);
          return;
        }

        state.mode = Mode::Follow;
        state.timer = 0;
      }

      if (state.mode == Mode::Advance) {
        if (state.blockedTime >= .25) {
          reject(pose.heading);
          reverse();
          return;
        }

        if (!forwardCandidate || pose.distance >= state.advanceUntil) {
          state.mode = Mode::Follow;
          state.timer = 0;
        } else if (state.timer > 5) {
          reject(pose.heading);
          reverse();
          return;
        } else {
          send(state.advanceSpeed, 0, "Moving straight for clearance");
          return;
        }
      }

      const bool straightRejected = obstacle && state.last == Request::Straight;

      if (frontBlocked || straightRejected) {
        const double leftHeading = pose.heading - std::numbers::pi / 2;

        const double rightHeading = pose.heading + std::numbers::pi / 2;

        const double rearHeading = pose.heading + std::numbers::pi;

        const bool leftCandidate =
          observations[1].distance > config.passageDistance && !rejected(leftHeading);

        const bool rightCandidate =
          observations[2].distance > config.passageDistance && !rejected(rightHeading);

        double angle = 0;

        // Choose the side with more measured clearance.
        // The rear is a fallback when neither side is usable.
        if (leftCandidate &&
            (!rightCandidate || observations[1].distance >= observations[2].distance)) {
          angle = -std::numbers::pi / 2;
        } else if (rightCandidate) {
          angle = std::numbers::pi / 2;
        } else if (observations[3].distance > config.passageDistance && !rejected(rearHeading)) {
          angle = std::numbers::pi;
        } else {
          if (forwardCandidate && !straightRejected) {
            advance("No side passage; trying forward clearance");
          } else {
            reverse();
          }

          return;
        }

        state.waiting = false;
        state.corridorReady = false;
        state.mode = Mode::Turn;
        state.timer = 0;
        state.blockedTime = 0;
        state.targetHeading = pose.heading + angle;

        send(0, angle < 0 ? -config.turnRate : config.turnRate, "Starting passage turn");
        return;
      }

      if (obstacle && state.last == Request::Curve) {
        state.corridorReady = false;
        send(config.speed, 0, "Correction blocked; checking straight clearance");
        return;
      }

      const double correction = CorridorTurn(state, observations, config, delta);
      send(config.speed,
        correction,
        correction == 0 ? "Following clear passage" : "Aligning within corridor");
    };

    components.Each<RoverNavigationComponent>(navigate);
  }

private:
  static double CorridorTurn(State& state,
    const std::array<Observation, 4>& observations,
    const RoverNavigationComponent& config,
    double delta) {
    const auto& left = observations[1];
    const auto& right = observations[2];
    if (!left.hit || !right.hit || left.distance >= config.passageDistance ||
        right.distance >= config.passageDistance) {
      state.corridorReady = false;
      return 0;
    }

    // +Y is right. Less clearance on the right requests a left turn.
    const double error = right.distance - left.distance;
    if (!state.corridorReady) {
      state.corridorReady = true;
      state.sideDerivative = 0;
    } else {
      const double derivative = (error - state.previousSideError) / delta;
      state.sideDerivative += delta / (.15 + delta) * (derivative - state.sideDerivative);
    }
    state.previousSideError = error;
    const double limit = std::min(.30, config.turnRate);
    return std::clamp(1.5 * error + 5.0 * state.sideDerivative, -limit, limit);
  }

  bool Observe(pipeframe::SceneObjectId id,
    pipeframe::SceneObject rover,
    double timeout,
    std::array<Observation, 4>& out) {
    bool valid = true;

    constexpr std::array<double, 4> directions{0, -90, 90, 180};

    auto collect = [&](const RangeObservation& sample) {
      const auto& sensor = sample.settings;
      if (sensor.parent.objectId != id) return;
      if (!(resolveObject(id) == rover) || !sample.present || sample.age >= timeout ||
          !sample.valid || !std::isfinite(sample.distance) || sample.distance < 0 ||
          !std::isfinite(sensor.localAngleDegrees) || !std::isfinite(sensor.coneAngleDegrees) ||
          sensor.coneAngleDegrees <= 0 || sensor.coneAngleDegrees > 90 ||
          (sample.status != SensorReadingStatus::Hit &&
            sample.status != SensorReadingStatus::OutOfRange &&
            sample.status != SensorReadingStatus::BelowMinimumRange)) {
        valid = false;
        return;
      }

      for (std::size_t i = 0; i < out.size(); ++i) {
        const double angle =
          std::abs(std::remainder(sensor.localAngleDegrees - directions[i], 360.0));

        if (angle > sensor.coneAngleDegrees / 2) {
          continue;
        }

        const double distance =
          sample.status == SensorReadingStatus::BelowMinimumRange ? 0.0 : sample.distance;
        if (!out[i].present || distance < out[i].distance) {
          out[i].distance = distance;
          out[i].hit = sample.status != SensorReadingStatus::OutOfRange;
        }

        out[i].capturedAt =
          out[i].present ? std::min(out[i].capturedAt, sample.capturedAt) : sample.capturedAt;

        out[i].sequence =
          out[i].present ? std::min(out[i].sequence, sample.sequence) : sample.sequence;

        out[i].present = true;
      }
    };

    ranges.Each(collect);

    return valid &&
           std::ranges::all_of(out, [](const auto& observation) { return observation.present; });
  }

  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  Resolver resolveObject;

  double time{};
  RoverRangeObservations ranges;
  std::unordered_map<pipeframe::ecs::Entity, Pose> poses;
  std::unordered_map<pipeframe::ecs::Entity, Safety> safety;
  std::unordered_map<pipeframe::ecs::Entity, State> states;

  pipeframe::EventSubscription poseConnection;
  pipeframe::EventSubscription safetyConnection;
};

}  // namespace RoverLab

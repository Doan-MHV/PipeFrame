#pragma once

#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_map>

#include "Components/RoverDriveComponent.h"
#include "Events/RoverOdometryEvent.h"
#include "Events/RoverSafetyEvent.h"

namespace RoverLab {

class RoverOdometrySystem final : public pipeframe::FixedUpdateSystem<void> {
public:
    struct Estimate {
        double x{};
        double y{};
        double heading{};
        double distance{};
    };

    RoverOdometrySystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
        : scene(scene), events(events) {
        driveConnection = events.Listen<RoverSafetyEvent>([this](const auto& event) {
            if (event.rover.IsValid()) outputs[event.rover.GetEntity()] = event;
        });
    }

    std::string_view GetSystemId() const override { return "roverlab.odometry"; }

    void Reset() {
        estimates.clear();
        outputs.clear();
        simulationTime = 0;
    }

    const Estimate* Find(pipeframe::ecs::Entity entity) const {
        const auto found = estimates.find(entity);

        if (found == estimates.end()) return nullptr;

        // Protect against destroyed objects or reused entity identifiers.
        if (!(found->second.rover == scene.GetObject(entity))) {
            return nullptr;
        }

        return &found->second.pose;
    }

    void Update(float delta) override {
        if (!std::isfinite(delta) || delta <= 0) return;

        simulationTime += delta;

        std::erase_if(estimates, [](const auto& entry) {
            const auto& rover = entry.second.rover;

            return !rover.IsValid() || !rover.template GetComponent<RoverDriveComponent>() ||
                   !rover.template GetComponent<pipeframe::KinematicBody2DComponent>();
        });

        auto& components = scene.Components();
        auto updateEstimate = [&](pipeframe::ecs::Entity entity) {
            const auto& drive = *components.Get<RoverDriveComponent>(entity);
            const auto& body = *components.Get<pipeframe::KinematicBody2DComponent>(entity);

            if (!scene.IsActive(entity)) return;

            const auto rover = scene.GetObject(entity);
            auto& state = estimates[entity];

            if (!(state.rover == rover)) {
                state = State{};
                state.rover = rover;
            }

            double forward = 0;
            double angular = 0;

            if (drive.enabled && drive.ValidSettings() && body.enabled) {
                // Drive supplies direction, physics supplies constrained magnitude.
                // Do not read ground-truth position or heading for the estimate.
                const auto output = outputs.find(entity);
                if (output != outputs.end() && output->second.rover == rover &&
                    output->second.status == RoverSafetyStatus::Clear &&
                    std::isfinite(output->second.appliedForwardSpeed)) {
                    const double signedSpeed = output->second.appliedForwardSpeed;
                    if (signedSpeed != 0) {
                        forward = std::copysign(std::hypot(double(body.velocity.x),
                                                          double(body.velocity.y)), signedSpeed);
                    }
                }

                angular = body.angularVelocity;
            }

            if (!std::isfinite(forward) || !std::isfinite(angular)) {
                return;
            }

            Integrate(state.pose, forward, angular, delta);

            RoverOdometryEvent event;
            event.simulationTime = simulationTime;
            event.rover = rover;
            event.x = state.pose.x;
            event.y = state.pose.y;
            event.heading = state.pose.heading;
            event.distance = state.pose.distance;

            events.Publish(event);
        };
        components.Each<RoverDriveComponent, pipeframe::KinematicBody2DComponent>(updateEstimate);
        outputs.clear(); // A direction is consumed once, never reused on a later step.
    }

private:
    struct State {
        pipeframe::SceneObject rover;
        Estimate pose;
    };

    static void Integrate(Estimate& pose, double forward, double angular, double delta) {
        const double angle = angular * delta;
        const double halfAngle = angle * 0.5;

        // Stable exact constant-speed arc integration.
        const double sinc = std::abs(halfAngle) < 1e-6 ? 1.0 - halfAngle * halfAngle / 6.0
                                                       : std::sin(halfAngle) / halfAngle;

        const double travel = forward * delta;
        const double midpointHeading = pose.heading + halfAngle;

        pose.x += travel * sinc * std::cos(midpointHeading);
        pose.y += travel * sinc * std::sin(midpointHeading);

        pose.heading = std::remainder(pose.heading + angle, 2.0 * std::numbers::pi);

        pose.distance += std::abs(travel);
    }

    pipeframe::BehaviourScene& scene;
    pipeframe::EventChannels& events;

    double simulationTime{};
    std::unordered_map<pipeframe::ecs::Entity, State> estimates;
    std::unordered_map<pipeframe::ecs::Entity, RoverSafetyEvent> outputs;
    pipeframe::EventSubscription driveConnection;
};

}  // namespace RoverLab
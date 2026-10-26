#pragma once

#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Simulation/System.h>

#include <cmath>

#include "Components/RoverDriveComponent.h"
#include "Components/RoverGoalComponent.h"
#include "Events/RoverGoalEvent.h"

namespace RoverLab {

// Simulator-side floor-marker detector. Ground truth is confined to detection,
// just as raycasting is confined to the range sensor implementations.
class RoverGoalSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  RoverGoalSystem(pipeframe::BehaviourScene& scene, pipeframe::EventChannels& events)
    : scene(scene), events(events) {}

  std::string_view GetSystemId() const override { return "roverlab.goal-detector"; }

  void Reset() { time = 0; }

  void Update(float delta) override {
    if (!std::isfinite(delta) || delta <= 0) return;

    time += delta;
    auto& components = scene.Components();

    components.Each<RoverGoalComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity markerId) {
        if (!scene.IsActive(markerId)) return;

        const auto marker = scene.GetObject(markerId);
        const auto& goal = *marker.GetComponent<RoverGoalComponent>();
        const auto& location = *marker.GetComponent<pipeframe::Transform2DComponent>();

        if (!goal.enabled || !goal.Valid()) return;

        components.Each<RoverDriveComponent, pipeframe::Transform2DComponent>(
          [&](pipeframe::ecs::Entity roverId) {
            if (!scene.IsActive(roverId)) return;

            const auto rover = scene.GetObject(roverId);
            const auto& pose = *rover.GetComponent<pipeframe::Transform2DComponent>();

            if (std::hypot(double(pose.position.x - location.position.x),
                  double(pose.position.y - location.position.y)) <= goal.radius) {
              RoverGoalEvent event;
              event.simulationTime = time;
              event.rover = rover;
              event.marker = marker;
              event.tag = goal.tag;
              events.Publish(event);
            }
          });
      });
  }

private:
  pipeframe::BehaviourScene& scene;
  pipeframe::EventChannels& events;
  double time{};
};

}  // namespace RoverLab

#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Render/GeometryDraw.h>
#include <PipeFrame/Render/RenderSystem.h>

#include <unordered_map>

#include "Events/LidarScanEvent.h"

namespace RoverLab {
class LidarSensorRenderer final : public pipeframe::RenderSystem {
public:
  explicit LidarSensorRenderer(pipeframe::EventChannels& events) {
    debugConnection = events.Listen<LidarDebugEvent>([this](const auto& event) {
      if (event.sensor.IsValid()) samples.insert_or_assign(event.sensor.GetEntity(), event);
    });
    scanConnection = events.Listen<LidarScanEvent>([this](const auto& event) {
      if (!event.valid && event.sensor.IsValid()) samples.erase(event.sensor.GetEntity());
    });
  }
  void Reset() { samples.clear(); }

protected:
  void Draw(pipeframe::Canvas canvas, pipeframe::BehaviourScene& scene) override {
    pipeframe::GeometryDraw draw;
    std::erase_if(samples, [&](const auto& item) {
      return !item.second.sensor.IsValid() ||
             !item.second.sensor.template GetComponent<LidarSensorComponent>();
    });
    scene.Components().Each<LidarSensorComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity entity) {
        if (!scene.IsActive(entity)) return;
        const auto& sensor = *scene.Components().Get<LidarSensorComponent>(entity);
        const auto& pose = *scene.Components().Get<pipeframe::Transform2DComponent>(entity);
        draw.Circle(pose.position, .035f, {255, 210, 70, 255});
        if (!sensor.enabled || !sensor.Valid()) return;
        const auto found = samples.find(entity);
        if (found == samples.end()) {
          draw.Circle(pose.position, float(sensor.maximumRange), {140, 150, 160, 180});
          return;
        }
        for (const auto& ray : found->second.rays)
          draw.Line(ray.origin,
            ray.end,
            ray.hit ? pipeframe::Color{70, 230, 140, 100} : pipeframe::Color{80, 170, 255, 70});
      });
    draw.Draw(canvas);
  }

private:
  std::unordered_map<pipeframe::ecs::Entity, LidarDebugEvent> samples;
  pipeframe::EventSubscription debugConnection, scanConnection;
};
}  // namespace RoverLab

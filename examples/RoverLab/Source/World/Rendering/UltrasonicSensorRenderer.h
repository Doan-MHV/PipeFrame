#pragma once
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/Render/GeometryDraw.h>
#include <PipeFrame/Render/RenderSystem.h>

#include <cmath>
#include <numbers>
#include <unordered_map>

#include "Components/InfraredSensorComponent.h"
#include "Components/UltrasonicSensorComponent.h"
#include "Events/SensorReadingEvent.h"

namespace RoverLab {
template <class SensorComponent = UltrasonicSensorComponent>
class SingleRangeSensorRenderer final : public pipeframe::RenderSystem {
public:
  explicit SingleRangeSensorRenderer(pipeframe::EventChannels& events)
    : sensorReadingSubscription(events.Listen<SensorReadingEvent>([this](const auto& reading) {
        if (reading.valid)
          samples.insert_or_assign(reading.sensor, reading);
        else
          samples.erase(reading.sensor);
      })) {}
  void Reset() { samples.clear(); }

protected:
  void Draw(pipeframe::Canvas canvas, pipeframe::BehaviourScene& scene) override {
    pipeframe::GeometryDraw draw;
    std::erase_if(samples,
      [&](const auto& entry) { return !scene.Components().Get<SensorComponent>(entry.first); });
    using pipeframe::Color;
    using pipeframe::Transform2DComponent;
    using pipeframe::Vector2f;

    const Color idle{140, 150, 160, 255};
    const Color clear{80, 170, 255, 255};
    const Color hit{70, 230, 140, 255};
    const Color danger{255, 80, 80, 255};
    const Color outline{255, 210, 70, 255};

    auto& components = scene.Components();
    auto drawSensor = [&](pipeframe::ecs::Entity entity) {
      const auto& sensor = *components.Get<SensorComponent>(entity);
      const auto& pose = *components.Get<Transform2DComponent>(entity);

      if (!scene.IsActive(entity)) return;

      draw.Circle(pose.position, 0.025f, sensor.enabled ? outline : idle);

      if (!sensor.enabled) return;

      const auto found = samples.find(entity);
      if (found == samples.end()) {
        if (!std::isfinite(sensor.maximumRange) || sensor.maximumRange <= 0 ||
            !std::isfinite(sensor.coneAngleDegrees) || sensor.coneAngleDegrees <= 0 ||
            sensor.coneAngleDegrees > 90) {
          return;
        }

        const double halfCone = sensor.coneAngleDegrees * std::numbers::pi / 360.0;

        for (int side : {-1, 1}) {
          const double angle = pose.rotation + side * halfCone;

          const Vector2f direction{
            static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))};

          draw.Line(pose.position,
            pose.position + direction * static_cast<float>(sensor.maximumRange),
            idle);
        }

        return;
      }

      const auto& sample = found->second;

      draw.Line(sample.origin, sample.rangeEndpoints.front(), outline);
      draw.Line(sample.origin, sample.rangeEndpoints.back(), outline);

      for (int ray = 0; ray < SensorReadingEvent::RayCount; ++ray) {
        Color color = sample.hits[ray] ? hit : clear;

        if (ray == sample.nearestRay && sample.tooClose) {
          color = danger;
        }

        draw.Line(sample.origin, sample.endpoints[ray], color);
      }

      if (sample.nearestRay >= 0) {
        draw.Circle(sample.endpoints[sample.nearestRay], 0.04f, sample.tooClose ? danger : outline);
      }
    };
    components.Each<SensorComponent, Transform2DComponent>(drawSensor);
    draw.Draw(canvas);
  }

private:
  std::unordered_map<pipeframe::ecs::Entity, SensorReadingEvent> samples;
  pipeframe::EventSubscription sensorReadingSubscription;
};
using UltrasonicSensorRenderer = SingleRangeSensorRenderer<>;
using InfraredSensorRenderer = SingleRangeSensorRenderer<InfraredSensorComponent>;
}  // namespace RoverLab

#pragma once

#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/ViewPanel.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "Components/UltrasonicSensorComponent.h"
#include "World/ProjectWorld.h"

namespace RoverLab {

class SensorTimingPanel final : public pipeframe::ui::ViewPanel {
public:
  SensorTimingPanel(ProjectWorld& world, pipeframe::ui::ProjectUIHost&) : world(world) {}

  void OnUpdate(float) override {
    std::ostringstream next;
    next << std::fixed << std::setprecision(3);

    auto& simulation = world.simulation;
    auto& scene = simulation.GetScene();
    auto& components = scene.Components();

    const double now = simulation.ultrasonic.SimulationTime();

    next << "Simulation: " << now << " s\n\n";

    int count = 0;

    auto append = [&](pipeframe::ecs::Entity entity) {
      const auto& sensor = *components.Get<UltrasonicSensorComponent>(entity);

      if (!scene.IsActive(entity)) {
        return;
      }

      ++count;

      next << "Rover " << sensor.parent.objectId << " / mount " << sensor.localAngleDegrees
           << " deg\n";

      if (!sensor.enabled) {
        next << "Disabled\n\n";
        return;
      }

      const auto* reading = simulation.ultrasonic.Latest(entity);

      if (!reading) {
        next << "Waiting for delivery or usable configuration\n\n";
        return;
      }

      next << "Captured: " << reading->capturedAt << " s\n";
      next << "Delivered: " << reading->simulationTime << " s\n";
      next << "Delivery age: " << reading->AgeAtDelivery() << " s\n";
      next << "Current age: " << std::max(0.0, now - reading->capturedAt) << " s\n";

      next << "Status: " << StatusText(reading->status) << '\n';

      if (reading->valid) {
        next << "Distance: " << reading->distance << " m\n";
      }

      next << '\n';
    };

    components.Each<UltrasonicSensorComponent>(append);

    components.Each<InfraredSensorComponent>([&](pipeframe::ecs::Entity entity) {
      if (!scene.IsActive(entity)) return;
      ++count;
      const auto& sensor = *components.Get<InfraredSensorComponent>(entity);
      next << "Infrared / rover " << sensor.parent.objectId << " / mount "
           << sensor.localAngleDegrees << " deg\n";
      next << "Optical FOV: " << sensor.coneAngleDegrees
           << " deg | Interval: " << sensor.samplePeriod << " s\n";
      const auto* reading = simulation.infrared.Latest(entity);
      if (!sensor.enabled)
        next << "Disabled\n\n";
      else if (!reading)
        next << "Waiting for optical reading\n\n";
      else {
        next << "Captured: " << reading->capturedAt
             << " s | Age: " << std::max(0.0, now - reading->capturedAt) << " s\n";
        next << "Status: " << StatusText(reading->status) << '\n';
        if (reading->valid) next << "Distance: " << reading->distance << " m\n";
        next << '\n';
      }
    });

    components.Each<LidarSensorComponent>([&](pipeframe::ecs::Entity entity) {
      if (!scene.IsActive(entity)) return;
      ++count;
      const auto& sensor = *components.Get<LidarSensorComponent>(entity);
      next << "LiDAR / rover " << sensor.parent.objectId << "\n";
      next << sensor.samplesPerScan << " beams / " << sensor.scanRate << " scans/s\n";
      if (!sensor.enabled) {
        next << "Disabled\n\n";
        return;
      }
      const auto* scan = simulation.lidar.Latest(entity);
      if (!scan || scan->beams.empty()) {
        next << "Waiting for scan; check parent, settings and unit rover scale\n\n";
        return;
      }
      next << "Capture span: " << scan->beams.front().capturedAt << " .. "
           << scan->beams.back().capturedAt << " s\n";
      next << "Delivered: " << scan->simulationTime << " s\n";
      next << "Oldest beam age: "
           << simulation.lidar.SimulationTime() - scan->beams.front().capturedAt << " s\n";
      const auto dropped = std::count_if(scan->beams.begin(),
        scan->beams.end(),
        [](const auto& beam) { return beam.status == SensorReadingStatus::Dropped; });
      next << "Dropped beams: " << dropped << "\n\n";
    });
    const auto value = count ? next.str() : "No range sensors";

    if (value != readout) {
      readout = value;
      RefreshView();
    }
  }

protected:
  pipeframe::ui::View BuildView() override {
    using namespace pipeframe::ui;

    return views::Scroll("sensor-timing-scroll",
      views::Column("sensor-timing",
        {
          views::Text("title", "SENSOR TIMING"),
          views::Text("readout", readout).FitHeight(),
          views::Button("hide", "HIDE", [this] { Hide(); }),
        })
        .Padding(12)
        .Spacing(8)
        .FitHeight())
      .FillHeight();
  }

private:
  static const char* StatusText(SensorReadingStatus status) {
    switch (status) {
      case SensorReadingStatus::Hit:
        return "Hit";

      case SensorReadingStatus::OutOfRange:
        return "Out of range";

      case SensorReadingStatus::BelowMinimumRange:
        return "Below minimum range";

      case SensorReadingStatus::Unavailable:
        return "Unavailable";

      case SensorReadingStatus::Dropped:
        return "Dropped measurement";
    }

    return "Unknown";
  }

  ProjectWorld& world;
  std::string readout{"Waiting for simulation"};
};

}  // namespace RoverLab

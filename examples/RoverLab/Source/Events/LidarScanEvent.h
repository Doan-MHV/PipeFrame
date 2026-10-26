#pragma once
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Events/Event.h>

#include <vector>

#include "Components/LidarSensorComponent.h"
#include "Events/SensorReadingEvent.h"

namespace RoverLab {
struct LidarBeam {
  double angle{};       // Radians relative to the LiDAR mount; +Y down.
  double capturedAt{};  // Actual fixed-step capture time, not scan delivery time.
  double distance{};
  SensorReadingStatus status{SensorReadingStatus::Unavailable};
};

struct LidarScanEvent final : pipeframe::Event {
  pipeframe::SceneObject sensor;
  pipeframe::SceneObjectId roverId{};
  LidarSensorComponent settings;  // Capture configuration, for change validation.
  std::vector<LidarBeam> beams;
  bool valid{};
};

// Ground-truth geometry is isolated from the scan consumed by mapping/control.
struct LidarDebugEvent final : pipeframe::Event {
  pipeframe::SceneObject sensor;
  struct Ray {
    pipeframe::Vector2f origin, end;
    bool hit{};
  };
  std::vector<Ray> rays;
};
}  // namespace RoverLab

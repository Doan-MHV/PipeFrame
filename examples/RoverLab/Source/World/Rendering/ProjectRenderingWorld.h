#pragma once
#include <PipeFrame/World/World.h>

#include "LidarSensorRenderer.h"
#include "RoverGoalRenderer.h"
#include "UltrasonicSensorRenderer.h"

namespace RoverLab {
class ProjectRenderingWorld final : public pipeframe::RenderingWorld {
public:
  explicit ProjectRenderingWorld(pipeframe::EventChannels& events)
    : ultrasonic(events), lidar(events), infrared(events) {
    ultrasonic.SetVisibility(pipeframe::RenderVisibility::Physics);
    AddSystem(ultrasonic);
    lidar.SetVisibility(pipeframe::RenderVisibility::Physics);
    AddSystem(lidar);
    infrared.SetVisibility(pipeframe::RenderVisibility::Physics);
    AddSystem(infrared);
    AddSystem(goals);
  }

  void ResetSensorReadings() {
    ultrasonic.Reset();
    lidar.Reset();
    infrared.Reset();
  }
  template <class Draw>
  void DrawScene(pipeframe::Canvas canvas,
    pipeframe::BehaviourScene& scene,
    pipeframe::RenderOptions options,
    Draw drawScene) {
    drawScene();
    Render(canvas);
    RenderSystems(canvas, scene, options);
  }

private:
  UltrasonicSensorRenderer ultrasonic;
  LidarSensorRenderer lidar;
  InfraredSensorRenderer infrared;
  RoverGoalRenderer goals;
};
}  // namespace RoverLab

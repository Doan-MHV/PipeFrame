#pragma once

#include "Editor/RoverDestinationTool.h"
#include "World/Physics/ProjectPhysicsWorld.h"
#include "World/Rendering/ProjectRenderingWorld.h"
#include "World/Simulation/ProjectSimulationWorld.h"

namespace RoverLab {

class ProjectWorld final : public pipeframe::World {
public:
  explicit ProjectWorld(pipeframe::BehaviourScene& scene)
    : scene(scene),
      simulation(scene),
      rendering(std::make_shared<ProjectRenderingWorld>(simulation.events)) {
    SetRenderingWorld(rendering);
  }

  void UpdateAuthoringScene() {
    ResetSensorState();
    simulation.hardware.Apply();
    simulation.ultrasonic.AlignMounts();
    simulation.lidar.AlignMounts();
    simulation.infrared.AlignMounts();
  }

  void BeforeFixedUpdate(float) {
    destinationTool.CaptureFrames(scene);
  }

  void Reset() {
    ResetSensorState();
  }

  void CollectWorldDebug(pipeframe::WorldDebugDraw& draw, pipeframe::WorldDebugOptions options) {
    if (options.physics) {
      physics.CollectDebug(draw);
    }
  }

  void ResetSensorState() {
    destinationTool.Reset();
    simulation.ultrasonic.Reset();
    simulation.lidar.Reset();
    simulation.infrared.Reset();
    simulation.navigation.Reset();
    simulation.solver.Reset();
    simulation.drive.Reset();
    simulation.odometry.Reset();
    simulation.mapping.Reset();
    simulation.goals.Reset();

    rendering->ResetSensorReadings();
  }

  pipeframe::BehaviourScene& scene;
  RoverDestinationTool destinationTool;
  ProjectSimulationWorld simulation;
  ProjectPhysicsWorld physics;
  std::shared_ptr<ProjectRenderingWorld> rendering;
};

}  // namespace RoverLab

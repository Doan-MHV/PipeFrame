#pragma once

#include <PipeFrame/Events/EventChannels.h>
#include <PipeFrame/World/World.h>

#include "InfraredSensorSystem.h"
#include "LidarSensorSystem.h"
#include "MicromouseHardwareSystem.h"
#include "RoverDriveSystem.h"
#include "RoverGoalSystem.h"
#include "RoverMappingSystem.h"
#include "RoverNavigationSystem.h"
#include "RoverOdometrySystem.h"
#include "RoverSolverSystem.h"
#include "UltrasonicSensorSystem.h"

namespace RoverLab {

class ProjectSimulationWorld final : public pipeframe::SimulationWorld {
public:
  explicit ProjectSimulationWorld(pipeframe::BehaviourScene& scene)
    : SimulationWorld(scene),
      hardware(scene),
      ultrasonic(scene, events),
      lidar(scene, events),
      infrared(scene, events),
      navigation(scene, events),
      drive(scene, events),
      odometry(scene, events),
      mapping(scene, events),
      goals(scene, events),
      solver(scene, events, mapping, drive) {
    using pipeframe::UpdatePhase;
    AddSystem(hardware, UpdatePhase::BeforePhysics);
    AddSystem(navigation, UpdatePhase::BeforePhysics);
    AddSystem(solver, UpdatePhase::BeforePhysics);
    AddSystem(drive, UpdatePhase::BeforePhysics);
    // Capture-time mapping needs odometry before sensor delivery.
    AddSystem(odometry, UpdatePhase::AfterPhysics);
    AddSystem(ultrasonic, UpdatePhase::AfterPhysics);
    AddSystem(lidar, UpdatePhase::AfterPhysics);
    AddSystem(infrared, UpdatePhase::AfterPhysics);
    AddSystem(mapping, UpdatePhase::AfterPhysics);
    AddSystem(goals, UpdatePhase::AfterPhysics);
  }

  pipeframe::EventChannels events;

  MicromouseHardwareSystem hardware;
  UltrasonicSensorSystem ultrasonic;
  LidarSensorSystem lidar;
  InfraredSensorSystem infrared;
  RoverNavigationSystem navigation;
  RoverDriveSystem drive;
  RoverOdometrySystem odometry;
  RoverMappingSystem mapping;
  RoverGoalSystem goals;
  RoverSolverSystem solver;
};

}  // namespace RoverLab

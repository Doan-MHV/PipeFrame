#pragma once

#include <PipeFrame/Project/ProjectDefinition.h>

#include "Components/MicromouseHardwareComponent.h"
#include "Components/MicromouseMazeComponent.h"
#include "Components/RoverMappingComponent.h"
#include "Components/RoverNavigationComponent.h"
#include "Components/RoverSafetyComponent.h"
#include "Components/RoverSolverComponent.h"
#include "Components/UltrasonicScheduleComponent.h"
#include "Editor/RoverMapPanel.h"
#include "Editor/RoverMissionPanel.h"
#include "Editor/RoverResultsPanel.h"
#include "Editor/RoverSpeedPanel.h"
#include "Editor/SensorTimingPanel.h"
#include "Entities/InfraredSensorEntity.h"
#include "Entities/LidarSensorEntity.h"
#include "Entities/RoverGoalEntity.h"
#include "GeneratedRegistration.h"
#include "World/ProjectWorld.h"

namespace RoverLab {

inline auto CreateProjectDefinition() {
  pipeframe::ProjectDefinition<ProjectWorld> project;

  pipeframe_generated::Register(project);

  project.Components<MicromouseHardwareComponent,
    MicromouseMazeComponent,
    RoverSafetyComponent,
    RoverNavigationComponent,
    UltrasonicScheduleComponent,
    RoverMappingComponent,
    LidarSensorComponent,
    InfraredSensorComponent,
    RoverSolverComponent,
    RoverGoalComponent>();
  project.RegisterEntity<LidarSensorEntity>(
    {"project.LidarSensorEntity", "LiDAR Sensor", {}, {pipeframe::Transform2DComponentTypeId}});

  project.RegisterEntity<InfraredSensorEntity>({"project.InfraredSensorEntity",
    "Infrared Wall Sensor",
    {},
    {pipeframe::Transform2DComponentTypeId}});

  project.RegisterEntity<RoverGoalEntity>(
    {"project.RoverGoalEntity", "Goal Marker", {}, {pipeframe::Transform2DComponentTypeId}});

  project.UI<RoverSpeedPanel>({
    .title = "ROVER SPEED",
    .size = {340, 320},
    .placement = pipeframe::ui::PanelPlacement::AtPercent(100, 20)
      .Anchor(pipeframe::ui::PanelAnchor::TopRight)
      .Offset(-16, 0),
    .drawer = true,
  });

  project.UI<SensorTimingPanel>({
    .title = "SENSOR TIMING",
    .size = {380, 460},
    .placement = pipeframe::ui::PanelPlacement::AtPercent(0, 20)
      .Anchor(pipeframe::ui::PanelAnchor::TopLeft)
      .Offset(16, 0),
    .drawer = true,
  });

  project.UI<RoverMapPanel>({
    .title = "DISCOVERED MAP",
    .size = {380, 560},
    .placement = pipeframe::ui::PanelPlacement::AtPercent(100, 70)
      .Anchor(pipeframe::ui::PanelAnchor::BottomRight)
      .Offset(-16, 0),
    .drawer = true,
  });

  project.UI<RoverMissionPanel>({
    .title = "ROVER MISSION",
    .size = {380, 420},
    .placement = pipeframe::ui::PanelPlacement::AtPercent(0, 70)
      .Anchor(pipeframe::ui::PanelAnchor::BottomLeft)
      .Offset(16, 0),
    .drawer = true,
  });

  project.UI<RoverResultsPanel>({
    .title = "BENCHMARK RESULTS",
    .size = {620, 680},
    .placement = pipeframe::ui::PanelPlacement::AtPercent(50, 0)
      .Anchor(pipeframe::ui::PanelAnchor::TopCenter)
      .Offset(0, 16),
    .drawer = true,
  });

  project.ConfigureWorld([](ProjectWorld& world) { (void)world; });

  return project;
}

}  // namespace RoverLab

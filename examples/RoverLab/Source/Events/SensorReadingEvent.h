#pragma once

#include <PipeFrame/ECS/World.h>
#include <PipeFrame/Events/Event.h>
#include <PipeFrame/Project/ProjectTypes.h>

#include <array>
#include <cmath>
#include <limits>

namespace RoverLab {

enum class SensorReadingStatus { Hit, OutOfRange, BelowMinimumRange, Unavailable, Dropped };

struct SensorReadingEvent final : pipeframe::Event {
  static constexpr int RayCount = 9;

  pipeframe::ecs::Entity sensor{};
  pipeframe::SceneObjectId roverId{};

  // Capture time: when the sensor observed the environment.
  double capturedAt{};

  // Inherited simulationTime is the actual delivery/publication time.
  // Both timestamps use the same simulation clock.
  double AgeAtDelivery() const {
    if (!std::isfinite(capturedAt) || !std::isfinite(simulationTime) || capturedAt < 0 ||
        simulationTime < capturedAt) {
      return std::numeric_limits<double>::infinity();
    }

    return simulationTime - capturedAt;
  }

  bool valid{};
  double distance{};
  bool tooClose{};
  SensorReadingStatus status{SensorReadingStatus::Unavailable};

  // Ideal ray geometry captured before measurement noise.
  // These fields are for visualization only.
  // Controllers must use valid, distance, status and timestamps.
  pipeframe::Vector2f origin{};
  std::array<pipeframe::Vector2f, RayCount> endpoints{};
  std::array<pipeframe::Vector2f, RayCount> rangeEndpoints{};
  std::array<bool, RayCount> hits{};
  int nearestRay{-1};
};

}  // namespace RoverLab

#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "World/Simulation/UltrasonicSensorSystem.h"

using namespace pipeframe;
using namespace RoverLab;

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool Near(double a, double b) {
  return std::abs(a - b) < 1e-5;
}

struct Wall final : EnvironmentQueries {
  float x{1.0f};

  std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin,
    Vector2f direction,
    float maximum,
    std::uint32_t,
    std::uint64_t ignoredObject) override {
    Check(ignoredObject == 10, "Exclude parent rover");

    if (std::abs(direction.x) < 1e-6f) {
      return {};
    }

    const float distance = (x - origin.x) / direction.x;

    if (distance < 0 || distance > maximum) {
      return {};
    }

    TilemapQueryHit hit;
    hit.distance = distance;
    hit.point = origin + direction * distance;
    return hit;
  }

  std::vector<TilemapQueryHit> OverlapEnvironment(Circle2D, std::uint32_t) override { return {}; }

  bool HasEnvironmentClearance(Circle2D, std::uint32_t) override { return true; }

  std::optional<TilemapQueryHit> SweepEnvironment(
    Circle2D, Vector2f, std::uint32_t, std::uint64_t, bool) override {
    return {};
  }
};

}  // namespace

int main() {
  try {
    BehaviourScene scene;
    EventChannels events;
    Wall wall;

    scene.Services().Provide<EnvironmentQueries>(wall);

    auto rover = scene.CreateObject();
    rover.AddComponent<Transform2DComponent>();

    auto& schedule = rover.AddComponent<UltrasonicScheduleComponent>();

    schedule.sequential = true;
    schedule.samplePeriod = .12;
    schedule.slotPeriod = .03;
    schedule.deliveryDelay = .05;

    std::array<SceneObject, 4> sensors;

    for (auto& object : sensors) {
      object = scene.CreateObject();
      object.AddComponent<Transform2DComponent>();

      auto& sensor = object.AddComponent<UltrasonicSensorComponent>();

      sensor.parent.objectId = 10;
    }

    UltrasonicSensorSystem system(scene, events);

    system.SetObjectResolver([&](SceneObjectId id) { return id == 10 ? rover : SceneObject{}; });

    std::vector<SensorReadingEvent> delivered;

    auto subscription = events.Listen<SensorReadingEvent>(
      [&](const SensorReadingEvent& event) { delivered.push_back(event); });

    auto step = [&](int count) {
      for (int i = 0; i < count; ++i) {
        system.Update(.01f);
      }
    };

    step(7);
    Check(delivered.empty(), "No delivery before capture plus delay");

    step(1);
    Check(delivered.size() == 1, "First delayed delivery");
    Check(Near(delivered[0].capturedAt, .03), "First capture time");
    Check(Near(delivered[0].simulationTime, .08), "First delivery time");
    Check(Near(delivered[0].AgeAtDelivery(), .05), "Delay contributes to age");

    step(9);
    Check(delivered.size() == 4, "Four sequential deliveries");

    for (int i = 0; i < 4; ++i) {
      Check(delivered[i].sensor == sensors[i].GetEntity(), "Stable round-robin order");

      Check(Near(delivered[i].capturedAt, .03 * (i + 1)), "Sequential capture spacing");

      Check(Near(delivered[i].AgeAtDelivery(), .05), "Each sample includes delivery age");
    }

    const auto firstRun = delivered;

    // Pause/alignment do not advance timestamps or flush the queue.
    const auto countBeforePause = delivered.size();
    const auto timeBeforePause = system.SimulationTime();

    system.Update(0);
    system.AlignMounts();

    Check(delivered.size() == countBeforePause, "Pause does not deliver");
    Check(system.SimulationTime() == timeBeforePause, "Pause freezes clock");

    // Reset reproduces the same sequence and timestamps.
    system.Reset();
    delivered.clear();
    step(17);

    Check(delivered.size() == firstRun.size(), "Reset repeats delivery count");

    for (std::size_t i = 0; i < delivered.size(); ++i) {
      Check(delivered[i].sensor == firstRun[i].sensor, "Reset repeats sensor order");

      Check(Near(delivered[i].capturedAt, firstRun[i].capturedAt), "Reset repeats capture times");

      Check(Near(delivered[i].simulationTime, firstRun[i].simulationTime),
        "Reset repeats delivery times");
    }

    // A delayed result contains geometry from capture, not delivery.
    system.Reset();
    delivered.clear();
    wall.x = 1.0f;

    step(3);  // First sensor captures the wall at X=1.
    wall.x = 1.5f;
    step(5);  // First sample now arrives.

    Check(delivered.size() == 1, "One historical sample delivered");
    Check(Near(delivered[0].distance, 1.0), "Queued measurement preserves capture-time distance");

    // Disabling a sensor cancels its queued measurement.
    system.Reset();
    delivered.clear();
    step(3);

    sensors[0].GetComponent<UltrasonicSensorComponent>()->enabled = false;
    step(20);

    for (const auto& event : delivered) {
      Check(!(event.sensor == sensors[0].GetEntity() && event.valid),
        "Disabled sensor cannot deliver queued valid data");
    }

    // Removing the schedule restores immediate, simultaneous defaults.
    sensors[0].GetComponent<UltrasonicSensorComponent>()->enabled = true;
    schedule.sequential = false;
    schedule.samplePeriod = .10;
    schedule.deliveryDelay = 0;

    system.Reset();
    delivered.clear();
    step(10);

    Check(delivered.size() == 4, "Simultaneous sampling");
    for (const auto& event : delivered) {
      Check(Near(event.capturedAt, .10), "Default capture interval");
      Check(Near(event.AgeAtDelivery(), 0), "Immediate delivery");
    }

    SensorReadingEvent malformed;
    malformed.capturedAt = 2;
    malformed.simulationTime = 1;

    Check(!std::isfinite(malformed.AgeAtDelivery()), "Invalid timestamps cannot appear fresh");

    std::cout << "Ultrasonic scheduling and timestamp checks passed.\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}

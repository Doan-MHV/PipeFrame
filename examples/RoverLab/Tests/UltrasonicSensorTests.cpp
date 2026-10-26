#include "World/Simulation/UltrasonicSensorSystem.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace pipeframe;
using namespace RoverLab;
namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool Near(double a, double b) { return std::abs(a - b) < 1e-5; }
// Analytic vertical wall: isolates sensor behavior from tilemap implementation.
struct Wall final : EnvironmentQueries {
    float x{3};
    int rays{};
    std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin, Vector2f direction,
                                                    float maximum, std::uint32_t mask, std::uint64_t ignoredObject) override {
        ++rays;
        Check(ignoredObject == 10, "Every sensor ray must exclude its owning scene object");
        Check(mask == std::numeric_limits<std::uint32_t>::max(), "Sensor must query obstacles regardless of visual layer order");
        if (std::abs(direction.x) < 1e-6f) return {};
        const float distance = (x - origin.x) / direction.x;
        if (distance < 0 || distance > maximum) return {};
        TilemapQueryHit hit;
        hit.distance = distance;
        hit.point = origin + direction * distance;
        return hit;
    }
    std::vector<TilemapQueryHit> OverlapEnvironment(Circle2D, std::uint32_t) override { return {}; }
    bool HasEnvironmentClearance(Circle2D, std::uint32_t) override { return true; }
    std::optional<TilemapQueryHit> SweepEnvironment(Circle2D, Vector2f, std::uint32_t,
                                                  std::uint64_t, bool) override { return {}; }
};
}
int main() {
    BehaviourScene scene;
    EventChannels events;
    Wall wall;
    scene.Services().Provide<EnvironmentQueries>(wall);
    auto rover = scene.CreateObject();
    rover.AddComponent<Transform2DComponent>();
    auto sensorObject = scene.CreateObject();
    sensorObject.AddComponent<Transform2DComponent>();
    sensorObject.AddComponent<UltrasonicSensorComponent>();
    auto& pose = *rover.GetComponent<Transform2DComponent>();
    auto& mount = *sensorObject.GetComponent<Transform2DComponent>();
    auto& settings = *sensorObject.GetComponent<UltrasonicSensorComponent>();
    Check(Near(settings.minimumRange, .02), "C++ default must agree with Inspector default");
    settings.parent.objectId = 10;
    settings.localPosition = {.16f, 0};
    pose.position = {2, 1};
    UltrasonicSensorSystem system(scene, events);
    system.SetObjectResolver([&](SceneObjectId id) { return id == 10 ? rover : SceneObject{}; });
    std::vector<SensorReadingEvent> readings;
    auto subscription = events.Listen<SensorReadingEvent>([&](const auto& event) { readings.push_back(event); });

    system.AlignMounts();
    Check(Near(mount.position.x, 2.16) && Near(mount.position.y, 1), "Mount offset at zero rotation");
    Check(readings.empty() && wall.rays == 0, "Editor alignment must not sample");
    pose.rotation = float(std::numbers::pi / 2);
    system.AlignMounts();
    Check(Near(mount.position.x, 2) && Near(mount.position.y, 1.16), "Rotated mount offset");
    settings.localAngleDegrees = -90;
    system.AlignMounts();
    Check(Near(mount.rotation, 0), "Local angle combines with parent rotation");
    pose.rotation = 0;
    settings.localAngleDegrees = 0;

    system.Update(.05f);
    Check(readings.empty(), "No measurement before sample interval");
    system.Update(.05f);
    Check(readings.size() == 1 && wall.rays == 9, "One nine-ray sample at 0.1 seconds");
    Check(readings.back().valid && readings.back().status == SensorReadingStatus::Hit,
          "Wall must produce a valid hit");
    Check(Near(readings.back().distance, .84) && readings.back().nearestRay == 4,
          "Distance is measured from mount, with nearest central ray");
    Check(Near(readings.back().simulationTime, .1), "Sample timestamp uses simulation time");
    system.Update(0);
    Check(readings.size() == 1, "Pause does not produce measurements");

    wall.x = 5;
    system.Update(.1f);
    Check(readings.back().status == SensorReadingStatus::OutOfRange &&
          Near(readings.back().distance, settings.maximumRange), "Out-of-range is not zero distance");
    wall.x = 2.17f;
    system.Update(.1f);
    Check(readings.back().status == SensorReadingStatus::BelowMinimumRange &&
          readings.back().tooClose && readings.back().distance == 0, "Explicit below-minimum status");
    const auto count = readings.size();
    settings.enabled = false;
    system.Update(.1f);
    Check(readings.size() == count + 1 && !readings.back().valid &&
          readings.back().status == SensorReadingStatus::Unavailable, "Disabled sensor invalidates reading");
    system.Update(.1f);
    Check(readings.size() == count + 1, "Unavailable notification is not repeated each tick");

    settings.enabled = true;
    wall.x = 3;
    system.Reset();
    readings.clear();
    system.Update(.05f);
    Check(readings.empty(), "Reset restarts sampling interval");
    system.Update(.05f);
    Check(readings.size() == 1 && Near(readings.back().simulationTime, .1), "Reset restarts sensor clock");
    settings.parent.objectId = 99;
    system.Update(.1f);
    Check(!readings.back().valid && readings.back().status == SensorReadingStatus::Unavailable,
          "Missing parent invalidates existing reading");
    std::cout << "Ultrasonic baseline: mounting, numerical range, timing, pause, reset and invalidation passed\n";
}

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "World/Simulation/RoverDriveSystem.h"
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
    Check(ignoredObject == 10, "Sensor must exclude its parent rover");

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

struct Fixture {
  BehaviourScene scene;
  EventChannels events;
  Wall wall;

  SceneObject rover;
  std::vector<SceneObject> sensors;

  UltrasonicSensorSystem ultrasonic;
  RoverDriveSystem drive;

  explicit Fixture(std::size_t sensorCount = 1) : ultrasonic(scene, events), drive(scene, events) {
    scene.Services().Provide<EnvironmentQueries>(wall);

    rover = scene.CreateObject();
    rover.AddComponent<Transform2DComponent>();
    rover.AddComponent<KinematicBody2DComponent>();

    rover.AddComponent<EnvironmentCollider2DComponent>().size = {.3f, .18f};

    rover.AddComponent<RoverDriveComponent>();
    rover.AddComponent<RoverSafetyComponent>();

    auto& schedule = rover.AddComponent<UltrasonicScheduleComponent>();

    schedule.samplePeriod = .01;

    for (std::size_t i = 0; i < sensorCount; ++i) {
      auto object = scene.CreateObject();

      object.AddComponent<Transform2DComponent>();

      auto& sensor = object.AddComponent<UltrasonicSensorComponent>();

      sensor.parent.objectId = 10;
      sensors.push_back(object);
    }

    const auto resolver = [this](SceneObjectId id) {
      return id == 10 ? rover : SceneObject{};
    };

    ultrasonic.SetObjectResolver(resolver);
    drive.SetObjectResolver(resolver);
  }

  UltrasonicScheduleComponent& Settings() {
    return *rover.GetComponent<UltrasonicScheduleComponent>();
  }

  void Reset() {
    ultrasonic.Reset();
    drive.Reset();

    auto& body = *rover.GetComponent<KinematicBody2DComponent>();

    body.velocity = {};
    body.angularVelocity = 0;
  }

  std::vector<SensorReadingEvent> Run(int steps) {
    std::vector<SensorReadingEvent> result;

    auto connection = events.Listen<SensorReadingEvent>(
      [&](const SensorReadingEvent& event) { result.push_back(event); });

    for (int step = 0; step < steps; ++step) {
      ultrasonic.Update(.01f);
    }

    return result;
  }

  void RequestForward() {
    RoverMotionCommandEvent command;
    command.roverId = 10;
    command.forwardSpeed = .1;
    command.turnRate = 0;

    events.Publish(command);
    drive.Update(.01f);
  }

  bool Moving() const {
    const auto* body = rover.GetComponent<KinematicBody2DComponent>();

    return std::hypot(body->velocity.x, body->velocity.y) > 1e-6;
  }
};

void TestIdealReadings() {
  Fixture fixture;
  const auto readings = fixture.Run(20);

  Check(readings.size() == 20, "One capture per step");

  for (const auto& reading : readings) {
    Check(reading.valid, "Default readings must be valid");
    Check(reading.status == SensorReadingStatus::Hit, "Default hit");
    Check(Near(reading.distance, 1.0), "Zero noise preserves distance");
  }
}

void TestNoiseAndReset() {
  Fixture fixture;
  fixture.Settings().noiseAmplitude = .01;

  const auto first = fixture.Run(128);
  bool changed = false;

  for (const auto& reading : first) {
    Check(reading.valid, "Noise alone keeps measurable hits valid");

    Check(reading.distance >= .99 - 1e-6 && reading.distance <= 1.01 + 1e-6,
      "Noise remains inside its configured bound");

    changed = changed || !Near(reading.distance, 1.0);
  }

  Check(changed, "Noise must change some distances");

  fixture.Reset();
  const auto repeated = fixture.Run(128);

  Check(first.size() == repeated.size(), "Reset repeats sample count");

  for (std::size_t i = 0; i < first.size(); ++i) {
    Check(first[i].distance == repeated[i].distance, "Reset repeats seeded distances");

    Check(first[i].status == repeated[i].status, "Reset repeats statuses");
  }

  fixture.Settings().randomSeed = 43;
  fixture.Reset();

  const auto differentSeed = fixture.Run(128);
  bool different = false;

  for (std::size_t i = 0; i < first.size(); ++i) {
    different = different || first[i].distance != differentSeed[i].distance;
  }

  Check(different, "Changing the seed changes the noise sequence");
}

void TestDropoutPattern() {
  Fixture fixture;
  fixture.Settings().dropoutProbability = .5;

  const auto first = fixture.Run(256);

  std::size_t dropped = 0;

  for (const auto& reading : first) {
    if (reading.status == SensorReadingStatus::Dropped) {
      ++dropped;
      Check(!reading.valid, "Dropped measurements must be invalid");
      Check(reading.distance == 0, "Dropout carries no usable distance");
    }
  }

  Check(dropped > 0 && dropped < first.size(),
    "Mixed dropout configuration produces hits and failures");

  // Noise amplitude must not alter the dropout random stream.
  fixture.Settings().noiseAmplitude = .02;
  fixture.Reset();

  const auto second = fixture.Run(256);

  Check(first.size() == second.size(), "Same capture count");

  for (std::size_t i = 0; i < first.size(); ++i) {
    Check(first[i].status == second[i].status, "Noise amplitude does not change dropout decisions");
  }
}

void TestSeparateSensorStreams() {
  Fixture fixture(4);
  fixture.Settings().noiseAmplitude = .02;

  const auto readings = fixture.Run(1);

  Check(readings.size() == 4, "All four sensors sampled");

  bool different = false;

  for (std::size_t i = 1; i < readings.size(); ++i) {
    different = different || readings[i].distance != readings[0].distance;
  }

  Check(different, "Sensors must not share identical noise draws");
}

void TestDelayedDropout() {
  Fixture fixture;
  fixture.Settings().dropoutProbability = 1.0;
  fixture.Settings().deliveryDelay = .05;

  Check(fixture.Run(5).empty(), "Dropout report must respect delivery delay");

  const auto delivered = fixture.Run(1);

  Check(delivered.size() == 1, "First dropout delivered at 0.06 s");
  Check(!delivered[0].valid, "Delayed dropout remains invalid");

  Check(delivered[0].status == SensorReadingStatus::Dropped, "Explicit dropout status");

  Check(Near(delivered[0].capturedAt, .01), "Capture timestamp");
  Check(Near(delivered[0].simulationTime, .06), "Delivery timestamp");
  Check(Near(delivered[0].AgeAtDelivery(), .05), "Delivery age");
}

void TestStopAndResume() {
  Fixture fixture;

  fixture.Run(1);
  fixture.RequestForward();
  Check(fixture.Moving(), "Valid clear reading permits forward motion");

  fixture.Settings().dropoutProbability = 1.0;
  fixture.Run(1);
  fixture.RequestForward();

  Check(!fixture.Moving(), "A dropout replaces the previous clear reading and stops motion");

  fixture.Settings().dropoutProbability = 0.0;
  fixture.Run(1);
  fixture.RequestForward();

  Check(fixture.Moving(), "A fresh valid reading permits motion again");
}

void TestPendingCancellation() {
  Fixture fixture;
  fixture.Settings().deliveryDelay = .05;

  fixture.Run(1);  // Queue a valid reading.

  fixture.Settings().dropoutProbability = 1.0;

  // Reconfiguration must discard the previously queued clear reading.
  const auto readings = fixture.Run(20);

  bool sawDropout = false;

  for (const auto& reading : readings) {
    Check(!reading.valid, "Old clear reading must not survive reconfiguration");

    sawDropout = sawDropout || reading.status == SensorReadingStatus::Dropped;
  }

  Check(sawDropout, "New dropout measurements must eventually arrive");
}

void TestRangeSemantics() {
  Fixture fixture;
  fixture.Settings().noiseAmplitude = 1.0;

  fixture.wall.x = 3.0f;
  const auto far = fixture.Run(1);

  Check(far.size() == 1, "One out-of-range reading");

  Check(
    far[0].status == SensorReadingStatus::OutOfRange, "Noise does not manufacture a distant echo");

  fixture.wall.x = .005f;
  fixture.Reset();

  const auto close = fixture.Run(1);

  Check(close.size() == 1, "One below-minimum reading");

  Check(close[0].status == SensorReadingStatus::BelowMinimumRange,
    "Noise cannot turn a below-minimum obstacle into clear space");
}

void TestValidation() {
  UltrasonicScheduleComponent settings;

  settings.dropoutProbability = 1.1;
  Check(!settings.Valid(), "Reject probability above one");

  settings.dropoutProbability = -.1;
  Check(!settings.Valid(), "Reject negative probability");

  settings.dropoutProbability = 0;
  settings.noiseAmplitude = std::numeric_limits<double>::quiet_NaN();
  Check(!settings.Valid(), "Reject NaN noise");

  settings.noiseAmplitude = 0;
  settings.randomSeed = -1;
  Check(!settings.Valid(), "Reject negative seed");
}

}  // namespace

int main() {
  try {
    TestIdealReadings();
    TestNoiseAndReset();
    TestDropoutPattern();
    TestSeparateSensorStreams();
    TestDelayedDropout();
    TestStopAndResume();
    TestPendingCancellation();
    TestRangeSemantics();
    TestValidation();

    std::cout << "Ultrasonic noise and dropout checks passed.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}

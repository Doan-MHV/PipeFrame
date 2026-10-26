#include <cassert>
#include <iostream>
#include <limits>
#include <map>

#include "World/Simulation/RoverDriveSystem.h"
#include "World/Simulation/MicromouseHardwareSystem.h"

int main() {
    using namespace pipeframe;
    using namespace RoverLab;

    BehaviourScene scene;
    EventChannels events;
    RoverDriveSystem drive(scene, events);

    std::map<SceneObjectId, SceneObject> objects;

    for (const auto id : {10, 20, 21, 22, 30, 40, 50, 61, 62, 63}) {
        objects[id] = scene.CreateObject();
    }

    for (const auto id : {10, 30}) {
        objects[id].AddComponent<Transform2DComponent>();
        objects[id].AddComponent<KinematicBody2DComponent>();
        // Explicit chassis geometry keeps these sensor-policy fixtures independent
        // of the kinematic fallback circle's default radius.
        objects[id].AddComponent<EnvironmentCollider2DComponent>().size = {.3f, .18f};
        objects[id].AddComponent<RoverDriveComponent>();
        objects[id].AddComponent<RoverSafetyComponent>();
    }

    // These original fixtures all face forward.
    for (const auto id : {20, 21, 22, 40}) {
        objects[id].AddComponent<Transform2DComponent>();

        objects[id].AddComponent<UltrasonicSensorComponent>().parent.objectId = id == 40 ? 30 : 10;
    }

    // Extra directional sensors are enabled for turning checks.
    for (int id : {61, 62, 63}) {
        auto& sensor = objects[id].AddComponent<UltrasonicSensorComponent>();

        sensor.parent.objectId = 10;

        sensor.localAngleDegrees = id == 61 ? -90 : id == 62 ? 90 : 180;

        sensor.enabled = false;
    }

    objects[50].AddComponent<KinematicBody2DComponent>().velocity = {.7f, 0};

    drive.SetObjectResolver([&](SceneObjectId id) {
        const auto found = objects.find(id);

        return found == objects.end() ? SceneObject{} : found->second;
    });

    const auto moving = [&](int id) {
        return objects[id].GetComponent<KinematicBody2DComponent>()->velocity.x > 0;
    };

    const auto publish = [&](int rover, int sensor, double distance, bool valid = true) {
        SensorReadingEvent reading;
        reading.roverId = rover;
        reading.sensor = objects[sensor].GetEntity();
        reading.valid = valid;
        reading.distance = distance;

        reading.status = valid ? SensorReadingStatus::Hit : SensorReadingStatus::Unavailable;

        events.Publish(reading);
    };

    const auto command = [&](int rover, double speed, double turn = 0) {
        RoverMotionCommandEvent event;
        event.roverId = rover;
        event.forwardSpeed = speed;
        event.turnRate = turn;

        events.Publish(event);
    };

    const auto tick = [&](float delta) {
        command(10, .16);
        command(30, .16);
        drive.Update(delta);
    };

    const auto clearAll = [&] {
        for (int sensor : {20, 21, 22, 61, 62, 63}) {
            publish(10, sensor, 1);
        }
    };

    tick(.01f);
    assert(!moving(10) && !moving(30));

    publish(10, 40, 1);
    tick(.01f);
    assert(!moving(10));  // Wrong parent.

    publish(10, 20, 1);
    tick(.01f);
    assert(!moving(10));  // Other enabled sensors have no readings.

    clearAll();
    tick(.01f);
    assert(moving(10) && !moving(30));

    publish(30, 40, 1);
    publish(10, 21, .2);
    publish(10, 20, 1);
    publish(10, 22, 1);
    tick(.01f);
    assert(!moving(10) && moving(30));

    clearAll();
    tick(.2f);

    publish(10, 20, 1);
    publish(10, 22, 1);
    tick(.11f);
    assert(!moving(10));  // One relevant sensor is stale.

    clearAll();
    tick(.01f);
    assert(moving(10));

    publish(10, 22, 1, false);
    tick(.01f);
    assert(!moving(10));

    objects[22].GetComponent<UltrasonicSensorComponent>()->enabled = false;

    tick(.01f);
    assert(moving(10));

    objects[22].GetComponent<UltrasonicSensorComponent>()->enabled = true;

    tick(.01f);
    assert(!moving(10));  // Requires a fresh reading.

    clearAll();
    tick(.01f);
    assert(moving(10));

    drive.Reset();
    tick(.01f);
    assert(!moving(10));

    clearAll();

    objects[10].GetComponent<RoverDriveComponent>()->enabled = false;

    tick(.01f);
    assert(!moving(10));

    objects[10].GetComponent<RoverDriveComponent>()->enabled = true;

    tick(.01f);
    assert(!moving(10));

    clearAll();

    objects[21].GetComponent<UltrasonicSensorComponent>()->parent.objectId = 30;

    publish(30, 40, 1);
    tick(.01f);
    assert(moving(10) && !moving(30));

    publish(30, 21, 1);
    tick(.01f);
    assert(moving(30));

    // Sensor readings alone do not start movement.
    drive.Reset();

    clearAll();
    drive.Update(.01f);
    assert(!moving(10));

    command(10, .1);
    drive.Update(.01f);
    assert(moving(10));

    assert(
        std::abs(objects[10].GetComponent<KinematicBody2DComponent>()->velocity.x - .1f) < 1e-6f);

    command(10, 2);
    drive.Update(.01f);

    assert(
        std::abs(objects[10].GetComponent<KinematicBody2DComponent>()->velocity.x - .16f) < 1e-6f);

    command(10, 0);
    drive.Update(.01f);
    assert(!moving(10));

    command(10, .1);
    drive.Update(.01f);

    command(10, std::numeric_limits<double>::quiet_NaN());
    drive.Update(.01f);
    assert(!moving(10));

    for (int id : {61, 62, 63}) {
        objects[id].GetComponent<UltrasonicSensorComponent>()->enabled = true;
    }

    clearAll();
    command(10, 0, 1);
    drive.Update(.01f);

    const auto* turningBody = objects[10].GetComponent<KinematicBody2DComponent>();

    assert(!moving(10));
    assert(std::abs(turningBody->angularVelocity - 1.0) < 1e-6);

    command(10, 0, 0);
    drive.Update(.01f);
    assert(turningBody->angularVelocity == 0);

    command(10, .1, std::numeric_limits<double>::quiet_NaN());
    drive.Update(.01f);
    assert(!moving(10));
    assert(turningBody->angularVelocity == 0);

    clearAll();
    command(10, 0, 1);
    drive.Update(.01f);
    assert(turningBody->angularVelocity > 0);

    for (int i = 0; i < 30; ++i) {
        clearAll();
        drive.Update(.01f);
    }

    assert(turningBody->angularVelocity == 0);

    for (int id : {61, 62, 63}) {
        objects[id].GetComponent<UltrasonicSensorComponent>()->enabled = false;
    }

    command(10, -.1);
    drive.Update(.01f);
    assert(!moving(10));  // Reverse requires rear sensor coverage.

    // Publish requests directly through the existing test command helper.
    command(10, .1);
    clearAll();
    drive.Update(.01f);
    assert(moving(10));

    // Stop publishing while sensors remain fresh.
    // The command timeout must stop the rover.
    for (int i = 0; i < 30; ++i) {
        clearAll();
        drive.Update(.01f);
    }

    assert(!moving(10));

    // A fresh command restores movement.
    command(10, .1);
    drive.Update(.01f);
    assert(moving(10));

    // An explicit zero command stops immediately.
    command(10, 0);
    drive.Update(.01f);
    assert(!moving(10));

    // Reset must discard the previous command.
    drive.Reset();
    clearAll();
    drive.Update(.01f);
    assert(!moving(10));

    // Hardware is optional: previous fixtures exercise unchanged legacy driving.
    auto& hardware = objects[10].AddComponent<MicromouseHardwareComponent>();
    MicromouseHardwareSystem hardwareSystem(scene);
    hardwareSystem.Apply();
    assert(objects[10].GetComponent<EnvironmentCollider2DComponent>()->size ==
           Vector2f(float(hardware.length), float(hardware.width)));
    assert(objects[10].GetComponent<RoverSafetyComponent>()->halfLength == hardware.length / 2);

    clearAll();
    command(10, .16);
    drive.Update(.01f);
    const double lightSpeed = turningBody->velocity.x;
    assert(lightSpeed > 0 && lightSpeed < .16);
    assert(std::abs(lightSpeed - hardware.Acceleration() * .01) < 1e-6);
    objects[10].GetComponent<KinematicBody2DComponent>()->velocity = {};
    hardware.mass *= 2;
    clearAll();
    command(10, .16);
    drive.Update(.01f);
    assert(std::abs(turningBody->velocity.x - lightSpeed / 2) < 1e-6);

    double arcSpeed = .16, arcTurn = 1;
    hardware.LimitAcceleration(0, 0, .01, arcSpeed, arcTurn);
    assert(arcSpeed > 0 && arcTurn > 0);
    assert(std::abs(arcSpeed / hardware.Acceleration() +
                   arcTurn / hardware.AngularAcceleration() - .01) < 1e-8);

    hardware.maximumRPM = 30;
    for (int i = 0; i < 100; ++i) {
        clearAll();
        command(10, .16);
        drive.Update(.01f);
    }
    assert(std::abs(turningBody->velocity.x - hardware.WheelSpeed()) < 1e-6);
    command(10, 0);
    drive.Update(.01f);
    assert(turningBody->velocity == Vector2f{});

    clearAll();
    command(10, .16);
    drive.Update(.01f);
    publish(10, 20, .001);
    command(10, .16);
    drive.Update(.01f);
    assert(turningBody->velocity == Vector2f{}); // Hardware never bypasses sensor safety.

    auto hardwareSchema = MicromouseHardwareComponent::Schema();
    std::string hardwareError;
    MicromouseHardwareComponent restored;
    assert(hardwareSchema.Apply(restored, hardwareSchema.Serialize(hardware).properties, hardwareError));
    assert(restored == hardware);
    assert(!hardwareSchema.Apply(restored, {{"mass", -1.0}}, hardwareError));
    assert(!hardwareSchema.Apply(restored, {{"trackWidth", .9}}, hardwareError));
    hardware.mass = 0; // Runtime corruption also fails closed.
    clearAll();
    command(10, .16);
    drive.Update(.01f);
    assert(turningBody->velocity == Vector2f{});
    assert(drive.StopReason(objects[10].GetEntity()) == "Invalid Micromouse Hardware settings");

    objects[20].Destroy();
    objects[22].Destroy();

    tick(.01f);
    assert(!moving(10));

    assert(objects[50].GetComponent<KinematicBody2DComponent>()->velocity.x == .7f);

    RoverDriveComponent legacy;
    std::string error;

    auto schema = RoverDriveComponent::Schema();

    assert(schema.Apply(legacy, {{"frontSensor", SceneObjectReference{20}}}, error));

    std::cout << "Rover drive: sensors, independent rovers, "
                 "timeouts, reparenting, reset and standalone body passed\n";
}
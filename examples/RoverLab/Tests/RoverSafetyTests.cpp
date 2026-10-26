#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>

#include "Editor/RoverSpeedPanel.h"

static void Require(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    using namespace pipeframe;
    using namespace RoverLab;

    BehaviourScene scene;
    ProjectWorld world(scene);
    auto& events = world.simulation.events;
    auto& drive = world.simulation.drive;
    // Use the same event bus as the hosted project panel and drive.


    std::map<SceneObjectId, SceneObject> objects;

    for (int id : {10, 20, 21, 22, 23, 30, 40}) {
        objects[id] = scene.CreateObject();
    }

    for (int id : {10, 30}) {
        objects[id].AddComponent<Transform2DComponent>();
        objects[id].AddComponent<KinematicBody2DComponent>();
        // Explicit chassis geometry keeps these sensor-policy fixtures independent
        // of the kinematic fallback circle's default radius.
        objects[id].AddComponent<EnvironmentCollider2DComponent>().size = {.3f, .18f};
        objects[id].AddComponent<RoverDriveComponent>();
        objects[id].AddComponent<RoverSafetyComponent>();
    }

    auto mount = [&](
        int id,
        int parent,
        double angle,
        Vector2f position
    ) {
        auto& sensor =
            objects[id].AddComponent<UltrasonicSensorComponent>();

        sensor.parent.objectId = parent;
        sensor.localAngleDegrees = angle;
        sensor.localPosition = position;
    };

    mount(20, 10, 0, {.16f, 0});
    mount(21, 10, -90, {0, -.09f});
    mount(22, 10, 90, {0, .09f});
    mount(23, 10, 180, {-.16f, 0});

    mount(40, 30, 0, {.16f, 0});

    drive.SetObjectResolver([&](SceneObjectId id) {
        const auto it = objects.find(id);

        return it == objects.end()
            ? SceneObject{}
            : it->second;
    });

    const auto rover = objects[10].GetEntity();

    auto body = [&]() {
        return objects[10].GetComponent<KinematicBody2DComponent>();
    };

    auto sample = [&](
        int sensor,
        double distance = 1,
        SensorReadingStatus status = SensorReadingStatus::Hit,
        bool valid = true
    ) {
        SensorReadingEvent event;

        event.sensor = objects[sensor].GetEntity();

        event.roverId =
            objects[sensor]
                .GetComponent<UltrasonicSensorComponent>()
                ->parent.objectId;

        event.distance = distance;
        event.status = status;
        event.valid = valid;

        events.Publish(event);
    };

    auto all = [&](double side = .2) {
        sample(20);
        sample(21, side);
        sample(22, side);
        sample(23);
    };

    auto tick = [&](
        double forward = .1,
        double turn = 0,
        float delta = .01f
    ) {
        RoverMotionCommandEvent event;
        event.roverId = 10;
        event.forwardSpeed = forward;
        event.turnRate = turn;

        events.Publish(event);
        drive.Update(delta);
    };

    auto status = [&]() {
        return drive.SafetyStatus(rover);
    };

    auto moving = [&]() {
        return body()->velocity.x > 0;
    };

    int notifications = 0;

    auto subscription = events.Listen<RoverSafetyEvent>(
        [&](const auto& event) {
            if (event.rover == objects[10]) {
                ++notifications;
            }
        }
    );

    tick();
    Require(!moving(), "Wait for readings");

    all();
    tick();
    Require(
        moving() && status() == RoverSafetyStatus::Clear,
        "Side walls at 0.2 m allow straight driving"
    );

    sample(20, .2);
    tick();
    Require(
        !moving() && status() == RoverSafetyStatus::Obstacle,
        "Front wall stops rover"
    );

    all();
    sample(21, .02);
    tick();
    Require(!moving(), "Dangerously close side stops rover");

    all();
    sample(23, .001);
    tick();
    Require(moving(), "Rear obstacle does not block forward motion");

    sample(23, 0, SensorReadingStatus::Unavailable, false);
    tick();
    Require(
        moving(),
        "Invalid rear reading is irrelevant to straight driving"
    );

    all();
    sample(21, 0, SensorReadingStatus::Unavailable, false);
    tick();
    Require(
        !moving() && status() == RoverSafetyStatus::InvalidReading,
        "Invalid enabled side reading stops rover"
    );

    all();
    sample(20, 0, SensorReadingStatus::BelowMinimumRange);
    tick();
    Require(!moving(), "Below minimum range is blocked");

    all();
    sample(20, 2, SensorReadingStatus::OutOfRange);
    tick();
    Require(
        moving(),
        "Out of range with adequate range permits driving"
    );

    all();
    tick(0, 1);
    Require(
        body()->angularVelocity > 0 &&
        body()->velocity == Vector2f{},
        "Turn in place"
    );

    all();
    tick(0, -1);
    Require(body()->angularVelocity < 0, "Opposite turn");

    all(.4);
    tick(.1, .5);
    Require(
        moving() && body()->angularVelocity > 0,
        "Forward curve"
    );

    all();
    sample(21, .1);
    tick(0, 1);
    Require(
        body()->angularVelocity > 0 &&
        status() == RoverSafetyStatus::Clear,
        "A limited turn does not require full-circle clearance"
    );

    all();
    sample(21, .04);
    tick(0, 1);
    Require(body()->angularVelocity == 0 && status() == RoverSafetyStatus::Obstacle,
            "Turn stops when its finite sweep lacks side clearance");

    all();
    sample(23, .01);
    tick(0, -1);
    Require(
        body()->angularVelocity == 0,
        "Rear obstacle blocks turning"
    );

    objects[23].GetComponent<UltrasonicSensorComponent>()->enabled = false;

    all();
    tick(0, 1);
    Require(
        status() == RoverSafetyStatus::MissingCoverage &&
        body()->angularVelocity == 0,
        "Missing rear coverage blocks turning"
    );

    tick();
    Require(
        moving(),
        "Rear coverage not needed for straight forward"
    );

    objects[23].GetComponent<UltrasonicSensorComponent>()->enabled = true;
    objects[20].GetComponent<UltrasonicSensorComponent>()->enabled = false;

    tick();
    Require(
        !moving() &&
        status() == RoverSafetyStatus::MissingCoverage,
        "Missing front coverage blocks forward"
    );

    objects[20].GetComponent<UltrasonicSensorComponent>()->enabled = true;

    all();
    tick(.1, 0, .31f);
    Require(
        !moving() &&
        status() == RoverSafetyStatus::StaleReading,
        "Stale readings stop rover"
    );

    all();

    objects[20]
        .GetComponent<UltrasonicSensorComponent>()
        ->localPosition.x = .15f;

    tick();
    Require(
        !moving() &&
        status() == RoverSafetyStatus::InvalidReading,
        "Mount changes require a new reading"
    );

    all();
    tick();
    Require(moving(), "New reading restores movement");

    objects[20]
        .GetComponent<UltrasonicSensorComponent>()
        ->maximumRange = .1;

    sample(20, .1, SensorReadingStatus::OutOfRange);
    tick();
    Require(!moving(), "Insufficient range cannot prove clearance");

    objects[20]
        .GetComponent<UltrasonicSensorComponent>()
        ->maximumRange = 2;

    objects[20]
        .GetComponent<UltrasonicSensorComponent>()
        ->localAngleDegrees = 45;

    all();
    tick();
    Require(
        status() == RoverSafetyStatus::MissingCoverage,
        "A diagonal cone is not front-axis coverage"
    );

    objects[20]
        .GetComponent<UltrasonicSensorComponent>()
        ->localAngleDegrees = 0;

    objects[10].GetComponent<EnvironmentCollider2DComponent>()->size = {.3f, .3f};
    all(.1);
    tick();
    Require(moving(), "Unscaled wider chassis fits this side clearance");

    objects[10].GetComponent<Transform2DComponent>()->scale = {2, 2};

    all(.1);
    tick();
    Require(
        !moving(),
        "Scaled footprint increases clearance requirement"
    );

    objects[10].GetComponent<Transform2DComponent>()->scale = {1, 1};
    objects[10].GetComponent<EnvironmentCollider2DComponent>()->size = {.3f, .18f};

    all();
    tick(2, 10);
    Require(
        std::hypot(body()->velocity.x, body()->velocity.y) <= .160001 &&
        std::abs(body()->angularVelocity) <= 2.000001,
        "Drive limits retained"
    );

    objects[10].GetComponent<RoverSafetyComponent>()->halfWidth = -1;

    tick();
    Require(
        status() == RoverSafetyStatus::MissingConfiguration,
        "Invalid footprint blocks movement"
    );

    objects[10].GetComponent<RoverSafetyComponent>()->halfWidth = .09;

    all();
    tick();
    Require(moving(), "Configuration restored");

    for (int i = 0; i < 30; ++i) {
        all();
        drive.Update(.01f);
    }

    Require(
        !moving() &&
        status() == RoverSafetyStatus::NoCommand,
        "Command timeout retained"
    );

    drive.Reset();

    tick(0, 0);
    Require(
        body()->velocity == Vector2f{} &&
        body()->angularVelocity == 0 &&
        status() == RoverSafetyStatus::Stopped,
        "Stop does not wait for readings"
    );

    tick();
    Require(!moving(), "Reset removes old samples");

    all();
    tick();
    Require(moving(), "Fresh data resumes motion after reset");

    objects[22]
        .GetComponent<UltrasonicSensorComponent>()
        ->parent.objectId = 30;

    sample(22);
    sample(40);

    tick();
    Require(moving(), "Other rover readings are excluded");

    tick(0, 1);
    Require(
        status() == RoverSafetyStatus::MissingCoverage,
        "Reparenting removes turn coverage"
    );

    objects[22]
        .GetComponent<UltrasonicSensorComponent>()
        ->parent.objectId = 10;

    all();
    tick();

    // Reverse ignores only the front cone; rear and sides remain mandatory
    // whenever present. Every failure must zero both linear and angular output.
    all();
    sample(20, .001, SensorReadingStatus::BelowMinimumRange);
    tick(-.05);
    Require(body()->velocity.x < 0 && status() == RoverSafetyStatus::Clear,
            "Reverse can escape a below-minimum front obstacle");
    tick(-2);
    Require(std::abs(body()->velocity.x + .16f) < 1e-5,
            "Reverse obeys configured speed cap");
    sample(23, .1);
    tick(-.05);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::Obstacle,
            "Rear obstacle blocks reverse");
    Require(drive.StopReason(rover).find("Rear sensor") != std::string::npos,
            "Stop reason identifies the rear sensor");
    ui::ProjectUIHost host;
    RoverSpeedPanel panel(world, host);
    panel.OnUpdate(0);
    const auto view = panel.DescribeView();
    ui::ValidateView(view);
    Require(view.children.at(0).children.at(1).text.find("Stop reason: Rear sensor:") !=
                std::string::npos,
            "Rover panel displays the actual rear-obstacle stop reason");
    sample(23, .001, SensorReadingStatus::BelowMinimumRange);
    tick(-.05);
    Require(body()->velocity.x == 0, "Below-minimum rear blocks reverse");
    sample(23, 1, SensorReadingStatus::Hit, false);
    tick(-.05);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::InvalidReading,
            "Invalid rear reading blocks reverse");
    all();
    tick(-.05, 0, .31f);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::StaleReading,
            "Expired readings block reverse");
    drive.Reset();
    sample(21, .2);
    sample(22, .2);
    tick(-.05);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::WaitingForReading,
            "Missing rear sample blocks reverse");
    objects[23].GetComponent<UltrasonicSensorComponent>()->enabled = false;
    tick(-.05);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::MissingCoverage,
            "Disabled rear sensor blocks reverse");
    objects[23].GetComponent<UltrasonicSensorComponent>()->enabled = true;
    all();
    sample(21, .01);
    tick(-.05);
    Require(body()->velocity.x == 0 && status() == RoverSafetyStatus::Obstacle,
            "Reverse still protects side clearance");
    all();

    objects[20].Destroy();

    tick();
    Require(
        !moving() &&
        status() == RoverSafetyStatus::MissingCoverage,
        "Destroyed front sensor removes coverage"
    );

    auto freeBody = scene.CreateObject();

    freeBody.AddComponent<KinematicBody2DComponent>().velocity = {
        .7f, 0
    };

    tick();
    Require(
        freeBody.GetComponent<KinematicBody2DComponent>()->velocity.x == .7f,
        "Standalone bodies are unaffected"
    );

    Require(notifications > 0, "Safety events delivered");

    std::cout
        << "RL3 direction, coverage, clearance, lifetime "
           "and timeout checks passed.\n";
}

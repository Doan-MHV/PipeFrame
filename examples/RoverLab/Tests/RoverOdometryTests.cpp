#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

#include "World/Simulation/RoverOdometrySystem.h"

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double actual, double expected) {
    return std::abs(actual - expected) < 1e-5;
}

} // namespace

int main() {
    using namespace pipeframe;
    using namespace RoverLab;

    BehaviourScene scene;
    EventChannels events;
    RoverOdometrySystem odometry(scene, events);

    auto rover = scene.CreateObject();
    rover.AddComponent<RoverDriveComponent>();
    auto& body = rover.AddComponent<KinematicBody2DComponent>();

    int eventCount = 0;
    RoverOdometryEvent latest;

    auto connection = events.Listen<RoverOdometryEvent>(
        [&](const RoverOdometryEvent& event) {
            latest = event;
            ++eventCount;
        });

    auto tick = [&](float delta, double signedSpeed = .1) {
        RoverSafetyEvent output;
        output.rover = rover;
        output.status = RoverSafetyStatus::Clear;
        output.appliedForwardSpeed = signedSpeed;
        events.Publish(output);
        odometry.Update(delta);
    };

    // Straight: 0.1 m/s for one second.
    body.velocity = {0.1f, 0};
    body.angularVelocity = 0;

    for (int i = 0; i < 100; ++i) {
        tick(0.01f);
    }

    Require(Near(latest.x, 0.1), "Straight X");
    Require(Near(latest.y, 0), "Straight Y");
    Require(Near(latest.distance, 0.1), "Straight distance");
    Require(eventCount == 100, "One event per active rover per step");

    // Turning in place adds heading but no centre-path distance.
    odometry.Reset();
    body.velocity = {};
    body.angularVelocity = std::numbers::pi / 2;

    for (int i = 0; i < 100; ++i) {
        tick(0.01f);
    }

    Require(Near(latest.heading, std::numbers::pi / 2),
            "Quarter turn");
    Require(Near(latest.x, 0) && Near(latest.y, 0),
            "Turn keeps estimated centre stationary");
    Require(Near(latest.distance, 0), "Turn centre-path distance");

    // Arc: radius = v / omega = 0.2 m.
    odometry.Reset();
    body.velocity = {0.1f, 0};
    body.angularVelocity = 0.5;

    for (int i = 0; i < 100; ++i) {
        tick(0.01f);
    }

    Require(Near(latest.x, 0.2 * std::sin(0.5)), "Arc X");
    Require(Near(latest.y, 0.2 * (1 - std::cos(0.5))), "Arc Y");
    Require(Near(latest.heading, 0.5), "Arc heading");

    // Backward translation is negative in the local odometry frame even
    // when world velocity points along +Y (rover has a different world heading).
    odometry.Reset();
    body.velocity = {0, .1f};
    body.angularVelocity = 0;
    for (int i = 0; i < 100; ++i) tick(.01f, -.1);
    Require(Near(latest.x, -.1) && Near(latest.y, 0), "Reverse local displacement");
    Require(Near(latest.distance, .1), "Reverse adds positive path length");
    odometry.Update(.1f);
    Require(Near(latest.x, -.1), "Consumed direction is not reused without new drive output");

    odometry.Reset();
    body.angularVelocity = .5;
    for (int i = 0; i < 100; ++i) tick(.01f, -.1);
    Require(Near(latest.x, -.2 * std::sin(.5)), "Reverse arc X");
    Require(Near(latest.y, -.2 * (1 - std::cos(.5))), "Reverse arc Y");

    // Stopping preserves pose and distance.
    const auto stopped = latest;
    body.velocity = {};
    body.angularVelocity = 0;
    odometry.Update(0.1f);

    Require(Near(latest.x, stopped.x) &&
            Near(latest.y, stopped.y) &&
            Near(latest.heading, stopped.heading) &&
            Near(latest.distance, stopped.distance),
            "Stopped estimate stays unchanged");

    // Zero elapsed time must not produce another sample.
    const int before = eventCount;
    odometry.Update(0);
    Require(eventCount == before, "No sample without elapsed time");

    // Reset removes the previous estimate.
    odometry.Reset();
    Require(odometry.Find(rover.GetEntity()) == nullptr,
            "Reset clears estimate");

    // A destroyed rover must not retain a snapshot.
    odometry.Update(0.01f);
    const auto entity = rover.GetEntity();
    rover.Destroy();
    odometry.Update(0.01f);

    Require(odometry.Find(entity) == nullptr,
            "Destroyed rover is removed");

    std::cout << "Rover odometry checks passed.\n";
}

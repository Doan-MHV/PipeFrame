#include <PipeFrame/Project/SceneProjectRuntime.h>

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Integration/GeneratedRegistration.h"
#include "World/Simulation/ProjectSimulationWorld.h"

namespace {
using namespace pipeframe;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

// Headless host: real project simulation and engine physics, without project UI.
class CorridorTestRuntime final : public SceneProjectRuntime {
public:
    CorridorTestRuntime()
        : SceneProjectRuntime("Corridor regression"), simulation(GetRuntimeScene()) {
        pipeframe_generated::Register(*this);
        RegisterComponent<RoverNavigationComponent>();
        RegisterComponent<RoverSafetyComponent>();
        const auto resolver = [this](SceneObjectId id) { return ResolveSceneObject(id); };
        simulation.navigation.SetObjectResolver(resolver);
        simulation.drive.SetObjectResolver(resolver);
        simulation.ultrasonic.SetObjectResolver(resolver);
    }

    RoverLab::ProjectSimulationWorld simulation;
    bool stallReverse{};
    bool rearObstacleOnReverse{};
    bool rearObstacleDetected{};

protected:
    void UpdateRuntimeScene(BehaviourScene&, float delta) override {
        simulation.Update(delta);
    }

    void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
        if (stallReverse) {
            const auto rover = ResolveSceneObject(10);
            if (rover.IsValid()) {
                auto* body = rover.GetComponent<KinematicBody2DComponent>();
                const auto* pose = rover.GetComponent<Transform2DComponent>();
                if (body && pose && body->velocity.x * std::cos(pose->rotation) +
                    body->velocity.y * std::sin(pose->rotation) < 0) body->velocity = {};
            }
        }
        SceneProjectRuntime::UpdatePhysics(delta, previous);
        simulation.UpdateAfterPhysics(delta);
        // Inject a new obstacle observation after actual reversing starts.
        // The real drive must veto the next command, then navigation must latch it.
        if (rearObstacleOnReverse) {
            const auto rover = ResolveSceneObject(10);
            const auto* body = rover.GetComponent<KinematicBody2DComponent>();
            const auto* pose = rover.GetComponent<Transform2DComponent>();
            rearObstacleDetected = rearObstacleDetected ||
                (body && pose && body->velocity.x * std::cos(pose->rotation) +
                 body->velocity.y * std::sin(pose->rotation) < 0);
            if (rearObstacleDetected) {
                RoverLab::SensorReadingEvent event;
                event.roverId = 10;
                event.sensor = ResolveSceneObject(23).GetEntity();
                event.valid = true;
                event.status = RoverLab::SensorReadingStatus::Hit;
                event.distance = .1;
                simulation.events.Publish(event);
            }
        }
    }
};

template <class T>
SceneComponentData Component(const T& value) {
    return T::Schema().Serialize(value);
}

std::vector<SceneObjectData> Fixture(CorridorTestRuntime& runtime) {
    auto rover = runtime.CreateDefaultObject("project.RoverEntity");
    rover.id = 10;
    EnvironmentCollider2DComponent collider;
    collider.size = {.3f, .2f};
    rover.components = {
        Component(Transform2DComponent{}),
        Component(collider),
        Component(KinematicBody2DComponent{}),
        Component(RoverDriveComponent{}),
        Component(RoverSafetyComponent{}),
    };
    std::vector<SceneObjectData> objects{rover};
    for (int id : {20, 21, 22, 23}) {
        auto sensor = runtime.CreateDefaultObject("project.UltrasonicSensorEntity");
        sensor.id = id;
        UltrasonicSensorComponent settings;
        settings.parent.objectId = 10;
        settings.localAngleDegrees = id == 20 ? 0 : id == 21 ? -90 : id == 22 ? 90 : 180;
        settings.localPosition = id == 20   ? Vector2f{.16f, 0}
                                 : id == 21 ? Vector2f{0, -.09f}
                                 : id == 22 ? Vector2f{0, .09f}
                                            : Vector2f{-.16f, 0};
        sensor.components = {Component(Transform2DComponent{}), Component(settings)};
        objects.push_back(sensor);
    }
    return objects;
}

SceneObjectData Wall(CorridorTestRuntime& runtime, SceneObjectId id, float y) {
    auto object = runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);
    object.id = id;
    object.transform.position = {0, y};
    Transform2DComponent transform;
    transform.position = object.transform.position;
    EnvironmentCollider2DComponent collider;
    collider.size = {20, .1f};
    object.components = {Component(transform), Component(collider)};
    return object;
}

void CheckNoWallOverlap(CorridorTestRuntime& runtime, const SceneObject& rover) {
    const auto shape = ColliderShape(
        *rover.GetComponent<EnvironmentCollider2DComponent>(),
        *rover.GetComponent<Transform2DComponent>()
    );
    for (int wallId : {40, 41}) {
        const auto wall = runtime.ResolveSceneObject(wallId);
        Check(
            !SweepBox(shape, {}, ColliderShape(
                *wall.GetComponent<EnvironmentCollider2DComponent>(),
                *wall.GetComponent<Transform2DComponent>()
            )),
            "Rover must not overlap a corridor wall"
        );
    }
}

void CheckCorridors() {
    constexpr std::array<std::array<double, 3>, 9> cases{{
        {.8, 0, 0},
        {.8, .22, 0},
        {.4, 0, 0},
        {.4, .04, 0},
        {.3, 0, 0},
        {.8, .22, .08},
        {.8, -.22, -.08},
        {.4, .04, .08},
        {.4, -.04, -.08},
    }};
    for (auto [width, offset, angle] : cases) {
        CorridorTestRuntime runtime;
        std::string error;
        Check(runtime.Load({}, error), "Load corridor fixture");
        auto objects = Fixture(runtime);
        auto& data = objects.front();
        Transform2DComponent pose;
        pose.position = {0, float(offset)};
        pose.rotation = float(angle);
        data.transform.position = pose.position;
        data.transform.rotation = pose.rotation;
        data.components[0] = Component(pose);
        RoverNavigationComponent navigation;
        navigation.rover.objectId = 10;
        data.components.push_back(Component(navigation));
        objects.push_back(Wall(runtime, 40, float(-width / 2 - .05)));
        objects.push_back(Wall(runtime, 41, float(width / 2 + .05)));
        runtime.SynchronizeScene(objects);
        runtime.Start();
        const auto rover = runtime.ResolveSceneObject(10);
        for (int i = 0; i < 1200; ++i) {
            runtime.FixedUpdate(1.f / 60);
            CheckNoWallOverlap(runtime, rover);
        }
        // Ground truth is used only for scoring, never for navigation decisions.
        const auto* actual = rover.GetComponent<Transform2DComponent>();
        std::cout << "width=" << width << " offset=" << offset << " angle=" << angle
                  << " x=" << actual->position.x << " y=" << actual->position.y << '\n';
        Check(actual->position.x > 1.5f, "Rover progresses through corridor");
        Check(std::abs(actual->position.y) < .02f, "Rover centres within corridor");
    }
}

void CheckWallTipRecovery(
    bool rearBlocked = false, bool stalled = false, bool mirrored = false,
    bool rearAppears = false
) {
    CorridorTestRuntime runtime;
    std::string error;
    Check(runtime.Load({}, error), "Load wall-tip fixture");
    auto objects = Fixture(runtime);
    Transform2DComponent pose;
    pose.rotation = mirrored ? .25f : -.25f;
    runtime.stallReverse = stalled;
    runtime.rearObstacleOnReverse = rearAppears;
    objects.front().transform.rotation = pose.rotation;
    objects.front().components[0] = Component(pose);
    RoverNavigationComponent navigation;
    navigation.rover.objectId = 10;
    objects.front().components.push_back(Component(navigation));
    auto wall = [&](SceneObjectId id, Vector2f position, Vector2f size) {
        auto data = Wall(runtime, id, 0);
        Transform2DComponent transform;
        transform.position = position;
        data.transform.position = position;
        EnvironmentCollider2DComponent collider;
        collider.size = size;
        data.components = {Component(transform), Component(collider)};
        return data;
    };
    objects.push_back(wall(40, {.24f, mirrored ? .95f : -.95f}, {.1f, 2.f}));
    objects.push_back(wall(41, {rearBlocked ? -.55f : -.95f, 0}, {.1f, 8.f}));
    runtime.SynchronizeScene(objects);
    runtime.Start();
    const auto rover = runtime.ResolveSceneObject(10);
    bool reversed = false;
    bool advancedAfterReverse = false;
    double reverseDistance = 0;
    double forwardAfterReverse = 0;
    auto connection = runtime.simulation.events.Listen<RoverLab::RoverSafetyEvent>(
        [&](const auto& event) {
            if (!(event.rover == rover)) return;
            if (event.appliedForwardSpeed < 0) {
                reversed = true;
                reverseDistance += -event.appliedForwardSpeed / 60;
            } else if (reversed && event.appliedForwardSpeed > 0) {
                forwardAfterReverse += event.appliedForwardSpeed / 60;
                advancedAfterReverse = true;
            }
        });
    std::string previous;
    double actualReverseDistance = 0;
    auto previousPosition = rover.GetComponent<Transform2DComponent>()->position;
    for (int i = 0; i < 3600; ++i) {
        runtime.FixedUpdate(1.f / 60);
        CheckNoWallOverlap(runtime, rover);
        const auto* currentPose = rover.GetComponent<Transform2DComponent>();
        const auto* body = rover.GetComponent<KinematicBody2DComponent>();
        if (body->velocity.x * std::cos(currentPose->rotation) +
            body->velocity.y * std::sin(currentPose->rotation) < 0) {
            actualReverseDistance += std::hypot(currentPose->position.x - previousPosition.x,
                                                currentPose->position.y - previousPosition.y);
        }
        previousPosition = currentPose->position;
        const auto message = runtime.simulation.navigation.Status(rover.GetEntity());
        if (message != previous) {
            std::cout << "tip t=" << i / 60.0 << " " << message << "\n";
            previous = message;
        }
    }
    const auto* actual = rover.GetComponent<Transform2DComponent>();
    std::cout << "tip reverse=" << reverseDistance << " forward=" << forwardAfterReverse
              << " final=" << actual->position.x << "," << actual->position.y << "\n";
    if (rearBlocked || stalled || rearAppears) {
        const auto reason = runtime.simulation.navigation.StopReason(rover.GetEntity());
        Check(!reason.empty(), "Blocked recovery retains its stop reason");
        if (rearAppears) {
            Check(reversed && reverseDistance < .002 && actualReverseDistance < .002 &&
                  reason.find("Rear sensor") != std::string::npos,
                  "A new rear obstacle stops reverse on the next tick and retains its reason");
        } else if (rearBlocked) {
            Check(!reversed && reason.find("rear sensor") != std::string::npos,
                  "Blocked rear refuses reverse and explains why");
        } else {
            Check(reversed && reverseDistance < .1 && actualReverseDistance == 0 &&
                  reason.find("no progress") != std::string::npos,
                  "Physics-stalled reverse stops promptly and does not retry forever");
        }
        return;
    }
    Check(reversed, "Wall-tip fixture must exercise reverse recovery");
    Check(actualReverseDistance > .24 && actualReverseDistance < .26,
          "Physical retreat matches the bounded reverse estimate");
    Check(std::hypot(actual->position.x, actual->position.y) > .5,
          "Rover physically leaves the wall tip, not just issuing commands");
    Check(reverseDistance >= .20 && reverseDistance < .30, "Retreat is bounded to a short distance");
    Check(advancedAfterReverse && forwardAfterReverse > .5,
          "Rover finds forward progress after backing away from wall tip");
}

void CheckSweptRotation() {
    CorridorTestRuntime runtime;
    std::string error;
    Check(runtime.Load({}, error), "Load rotation fixture");
    auto objects = Fixture(runtime);
    // No drive component: exercise engine angular motion directly.
    objects.front().components.resize(3);
    objects.push_back(Wall(runtime, 40, -.2f));
    objects.push_back(Wall(runtime, 41, .2f));
    runtime.SynchronizeScene(objects);
    runtime.Start();
    const auto rover = runtime.ResolveSceneObject(10);
    auto* pose = rover.GetComponent<Transform2DComponent>();
    auto* body = rover.GetComponent<KinematicBody2DComponent>();
    for (int i = 0; i < 120; ++i) {
        body->angularVelocity = 6;
        runtime.FixedUpdate(1.f / 60);
        CheckNoWallOverlap(runtime, rover);
    }
    Check(pose->rotation > .05 && pose->rotation < .45,
          "A partial turn fits but a complete turn is constrained by the wall");
    Check(body->angularVelocity == 0, "Blocked rotation reports zero angular velocity");
}
} // namespace

int main() {
    try {
        CheckCorridors();
        CheckSweptRotation();
        CheckWallTipRecovery();
        CheckWallTipRecovery(false, false, true);
        CheckWallTipRecovery(true);
        CheckWallTipRecovery(false, true);
        CheckWallTipRecovery(false, false, false, true);
        std::cout << "Rover corridor and swept rotation checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

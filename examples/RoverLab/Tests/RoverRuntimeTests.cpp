#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Integration/RoverLabAdapter.h"

namespace {
using namespace pipeframe;
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template <class T>
SceneComponentData Component(const T& value) {
    const auto schema = T::Schema();
    return schema.Serialize(value);
}

class TestRuntime final : public pipeframe::SceneProjectRuntime {
public:
    TestRuntime() : SceneProjectRuntime("RoverLab test"), world(GetRuntimeScene()) {
        SetSimulationWorld(world.simulation);
        RegisterProject(RoverLab::CreateProjectDefinition(), world);

        auto resolver = [this](pipeframe::SceneObjectId id) {
            return ResolveSceneObject(id);
        };

        world.simulation.ultrasonic.SetObjectResolver(resolver);
        world.simulation.navigation.SetObjectResolver(resolver);
        world.simulation.drive.SetObjectResolver(resolver);
        world.simulation.mapping.SetObjectResolver(resolver);
    }

    const RoverLab::RoverMappingSystem::Map* Map() {
        const auto rover = ResolveSceneObject(10);
        return rover.IsValid() ? world.simulation.mapping.Find(rover.GetEntity()) : nullptr;
    }

    void UpdateAuthoringScene() override {
        world.ResetSensorState();
        world.simulation.ultrasonic.AlignMounts();
    }

    void Reset() override {
        world.ResetSensorState();
        SceneProjectRuntime::Reset();
    }

protected:
    void UpdateRuntimeScene(pipeframe::BehaviourScene& scene, float delta) override {
        RoverLab::RoverMotionCommandEvent command;
        command.roverId = 10;
        command.forwardSpeed = .1;

        world.simulation.events.Publish(command);
        SceneProjectRuntime::UpdateRuntimeScene(scene, delta);
    }

    void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
        world.physics.Simulate(
            delta, [&](float step) { SceneProjectRuntime::UpdatePhysics(step, previous); });

    }

private:
    RoverLab::ProjectWorld world;
};

template <class Runtime>
std::vector<SceneObjectData> Fixture(Runtime& runtime, float bodyWidth = .2f) {
    auto rover = runtime.CreateDefaultObject("project.RoverEntity");
    rover.id = 10;

    EnvironmentCollider2DComponent collider;
    collider.size = {.3f, bodyWidth};

    rover.components = {
        Component(Transform2DComponent{}),
        Component(collider),
        Component(KinematicBody2DComponent{}),
        Component(RoverDriveComponent{}),
        Component(RoverSafetyComponent{}),
        Component(RoverMappingComponent{}),
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

        sensor.components = {
            Component(Transform2DComponent{}),
            Component(settings),
        };

        objects.push_back(sensor);
    }

    return objects;
}

template <class Runtime>
SceneObjectData Obstacle(Runtime& runtime, SceneObjectId id, Vector2f position, Vector2f size) {
    auto object = runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);

    object.id = id;
    object.transform.position = position;

    Transform2DComponent transform;
    transform.position = position;

    EnvironmentCollider2DComponent collider;
    collider.size = size;

    object.components = {
        Component(transform),
        Component(collider),
    };

    return object;
}

void CheckNavigationScenarios() {
    for (bool corner : {false, true}) {
        RoverLab::RoverLabAdapter runtime;
        std::string error;

        Check(runtime.Load({}, error), "Load navigation fixture");

        auto objects = Fixture(runtime);

        RoverNavigationComponent navigation;
        navigation.rover.objectId = 10;

        objects.front().components.push_back(Component(navigation));

        // Front wall.
        objects.push_back(
            Obstacle(runtime, 40, {.9f, 0}, {.1f, 8})
        );

        // Right wall.
        objects.push_back(
            Obstacle(runtime, 41, {0, .5f}, {8, .1f})
        );

        if (corner) {
            // A left opening followed by a corridor going upward.
            objects.push_back(
                Obstacle(runtime, 42, {-2.2f, -.5f}, {4, .1f})
            );

            objects.push_back(
                Obstacle(runtime, 43, {-.25f, -2.25f}, {.1f, 4})
            );
        } else {
            // Left wall closes the corridor into a dead end.
            objects.push_back(
                Obstacle(runtime, 42, {0, -.5f}, {8, .1f})
            );
        }

        runtime.SynchronizeScene(objects);
        runtime.UpdateAuthoringScene();
        runtime.Start();

        const auto rover = runtime.ResolveSceneObject(10);

        double maximumX = 0;
        bool turned = false;

        for (int i = 0; i < 720; ++i) {
            runtime.FixedUpdate(1.f / 60);

            // Ground truth is used for test scoring only.
            const auto* pose =
                rover.GetComponent<Transform2DComponent>();

            maximumX = std::max(
                maximumX,
                static_cast<double>(pose->position.x)
            );

            if (corner
                    ? pose->rotation < -1.2
                    : std::abs(pose->rotation) > 2.7) {
                turned = true;
            }
        }

        const auto* pose =
            rover.GetComponent<Transform2DComponent>();

        Check(turned, "Navigation performs the expected turn");

        Check(
            corner
                ? pose->position.y < -.2
                : pose->position.x < maximumX - .1,
            "Navigation proceeds after the turn"
        );
    }
}

}  // namespace
int main() {
    try {
        using namespace RoverLab;
        TestRuntime runtime;
        std::string error;
        Check(runtime.Load({}, error), "Load actual project adapter");
        auto objects = Fixture(runtime);
        objects.push_back(Obstacle(runtime, 40, {1, 0}, {.1f, 4}));
        runtime.SynchronizeScene(objects);
        runtime.UpdateAuthoringScene();
        const auto rover = runtime.ResolveSceneObject(10);
        auto* pose = rover.GetComponent<Transform2DComponent>();
        auto* body = rover.GetComponent<KinematicBody2DComponent>();
        const auto unfiltered = runtime.RaycastEnvironment({0, .09f}, {0, 1}, 2);
        Check(unfiltered && unfiltered->objectId == 10 && unfiltered->distance == 0,
            "Fixture reproduces a side mount inside its own collider");
        Check(!runtime.RaycastEnvironment({0, .09f}, {0, 1}, 2, ~std::uint32_t{}, 10),
            "Owner exclusion removes the chassis without a false obstacle");
        const auto external = runtime.RaycastEnvironment({0, 0}, {1, 0}, 2, ~std::uint32_t{}, 10);
        Check(external && external->objectId == 40 && std::abs(external->distance - .95f) < 1e-5,
            "Owner exclusion preserves another object's collider");
        runtime.Start();
        for (int i = 0; i < 15; ++i)
            runtime.FixedUpdate(1.f / 60);
        Check(body->velocity.x > .09f && pose->position.x > 0,
            "Real sensor queries, events, drive and physics move the assembled rover");
        for (int i = 0; i < 600; ++i)
            runtime.FixedUpdate(1.f / 60);
        Check(body->velocity == Vector2f{} && pose->position.x < .8f,
            "Real front observations stop the chassis before the wall");
        Check(runtime.Map() && runtime.Map()->accepted > 0,
            "Post-physics odometry precedes sensor delivery and mapping consumes observations");
        runtime.Reset();
        Check(!runtime.Map(), "Reset clears the registered mapper");
        Check(runtime.ResolveSceneObject(10).GetComponent<Transform2DComponent>()->position ==
                  Vector2f{},
            "Reset restores authored pose");
        runtime.Start();
        runtime.FixedUpdate(1.f / 60);
        Check(runtime.ResolveSceneObject(10).GetComponent<KinematicBody2DComponent>()->velocity ==
                  Vector2f{},
            "Reset requires fresh sensor readings");
        for (int i = 0; i < 15; ++i)
            runtime.FixedUpdate(1.f / 60);
        Check(
            runtime.ResolveSceneObject(10).GetComponent<KinematicBody2DComponent>()->velocity.x > 0,
            "Fresh readings restore motion after reset");

        // An undersized configured footprint must not hide a wide body's side clearance.
        TestRuntime wide;
        Check(wide.Load({}, error), "Load wide rover fixture");

        auto wideObjects = Fixture(wide, .6f);

        wideObjects.push_back(Obstacle(wide, 40, {0, .37f}, {4, .1f}));
        wide.SynchronizeScene(wideObjects);
        wide.UpdateAuthoringScene();
        wide.Start();

        for (int i = 0; i < 30; ++i)
            wide.FixedUpdate(1.f / 60);

        const auto wideRover = wide.ResolveSceneObject(10);

        Check(wideRover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{} &&
                  wideRover.GetComponent<Transform2DComponent>()->position == Vector2f{},
            "Safety envelope encloses actual collider despite smaller inspector dimensions");

        TestRuntime fallback;
        Check(fallback.Load({}, error), "Load fallback-circle fixture");
        auto fallbackObjects = Fixture(fallback);

        std::erase_if(fallbackObjects.front().components, [](const auto& component) {
            return component.typeId == EnvironmentCollider2DComponentTypeId;
        });

        fallbackObjects.push_back(Obstacle(fallback, 40, {0, .57f}, {4, .1f}));
        fallback.SynchronizeScene(fallbackObjects);
        fallback.UpdateAuthoringScene();
        fallback.Start();

        for (int i = 0; i < 30; ++i)
            fallback.FixedUpdate(1.f / 60);

        Check(fallback.ResolveSceneObject(10).GetComponent<KinematicBody2DComponent>()->velocity ==
                  Vector2f{},
            "Safety envelope includes the actual fallback circle radius");

        CheckNavigationScenarios();

        std::cout << "Rover runtime: owner exclusion, real sampling, driving, wall stop, reset and "
                     "footprint passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

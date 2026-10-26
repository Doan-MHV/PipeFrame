#include <PipeFrame/Project/AssetDatabase.h>

#include <chrono>
#include <iostream>
#include <stdexcept>

#include "Integration/RoverLabAdapter.h"
#include "SceneSerializer.h"

using namespace pipeframe;
using namespace RoverLab;

namespace {
void Check(bool value, const std::string& message) {
  if (!value) throw std::runtime_error(message);
}

class Runtime final : public SceneProjectRuntime {
public:
  Runtime() : SceneProjectRuntime("RL7 solver scene test"), world(GetRuntimeScene()) {
    SetSimulationWorld(world.simulation);
    RegisterProject(CreateProjectDefinition(), world);
    auto resolve = [&](SceneObjectId id) {
      return ResolveSceneObject(id);
    };
    world.simulation.ultrasonic.SetObjectResolver(resolve);
    world.simulation.lidar.SetObjectResolver(resolve);
    world.simulation.navigation.SetObjectResolver(resolve);
    world.simulation.drive.SetObjectResolver(resolve);
    world.simulation.mapping.SetObjectResolver(resolve);
    world.simulation.solver.SetObjectResolver(resolve);
  }

  void UpdateAuthoringScene() override {
    world.ResetSensorState();
    world.simulation.ultrasonic.AlignMounts();
    world.simulation.lidar.AlignMounts();
  }

  void Reset() override {
    world.ResetSensorState();
    SceneProjectRuntime::Reset();
  }

  ProjectWorld world;
  bool freeze{};

protected:
  void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
    if (freeze) {
      GetRuntimeScene().Components().Each<KinematicBody2DComponent>([&](ecs::Entity id) {
        auto& body = *GetRuntimeScene().Components().Get<KinematicBody2DComponent>(id);
        body.velocity = {};
        body.angularVelocity = 0;
      });
      return;
    }
    world.physics.Simulate(
      delta, [&](float step) { SceneProjectRuntime::UpdatePhysics(step, previous); });
  }
};
}  // namespace

int main() {
  const std::filesystem::path root = ROVER_PROJECT_ROOT;
  std::string error;
  const auto sandbox =
    std::filesystem::temp_directory_path() /
    ("rover-solver-scenes-" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Check(std::filesystem::create_directory(sandbox), "Create isolated asset fixture");

  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{sandbox};

  std::filesystem::copy(
    root / "Assets", sandbox / "Assets", std::filesystem::copy_options::recursive);
  std::filesystem::create_directory(sandbox / ".pipeframe");
  std::filesystem::copy_file(root / ".pipeframe/assets.db", sandbox / ".pipeframe/assets.db");

  assets::AssetDatabase database;
  Check(database.Open(sandbox, &error), error);
  ServiceRegistry services;
  services.Provide(database);

  auto ultrasound = editor::SceneSerializer::Load(root / "Scenes/Ultrasonic.pfscene", &error);
  auto laser = editor::SceneSerializer::Load(root / "Scenes/LiDAR.pfscene", &error);
  Check(ultrasound.has_value() && laser.has_value(), error);
  Check(ultrasound->FindObject(1)->components == laser->FindObject(1)->components,
    "Comparison scenes must share the same map");
  Check(ultrasound->FindObject(2)->transform == laser->FindObject(2)->transform,
    "Comparison scenes must share the same starting pose");

  for (bool useLaser : {false, true}) {
    Runtime runtime;
    Check(runtime.Load({.projectDirectory = sandbox, .services = &services}, error), error);

    auto document = useLaser ? *laser : *ultrasound;
    document.SetComponentProperty(2, "project.RoverNavigationComponent", "enabled", false);
    document.SetComponentProperty(2, "project.RoverMappingComponent", "cellSize", .05);

    RoverSolverComponent solverConfig;
    document.RemoveComponent(2, "project.RoverSolverComponent");
    Check(document.AddComponent(2, solverConfig.Schema().Serialize(solverConfig)),
      "Install fresh solver fixture");

    runtime.SynchronizeScene(document.GetObjects());
    runtime.UpdateAuthoringScene();

    auto rover = runtime.ResolveSceneObject(2);
    Check(rover.IsValid(), "Saved scene rover loads");
    Check(rover.GetComponent<RoverSolverComponent>()->rover.objectId == 2,
      "Solver automatically targets its containing rover without an authored ID");

    int sonar = 0, lidar = 0;
    runtime.world.simulation.GetScene().Components().Each<UltrasonicSensorComponent>(
      [&](ecs::Entity) { ++sonar; });

    runtime.world.simulation.GetScene().Components().Each<LidarSensorComponent>(
      [&](ecs::Entity) { ++lidar; });
    Check(useLaser ? sonar == 0 && lidar == 1 : sonar == 4 && lidar == 0,
      "Each scene must contain only its chosen sensor technology");

    const auto start = rover.GetComponent<Transform2DComponent>()->position;
    Check(runtime.RaycastEnvironment(start, {0, -1}, 10, ~std::uint32_t{}, 2).has_value(),
      "Saved map collision asset loads");

    runtime.Start();
    double travelled = 0;
    std::set<std::pair<int, int>> visited;
    auto last = start;

    for (int i = 0; i < 18000; ++i) {
      runtime.FixedUpdate(1.f / 60);
      const auto current = rover.GetComponent<Transform2DComponent>()->position;
      visited.insert({int(std::floor(current.x / .1)), int(std::floor(current.y / .1))});
      travelled += std::hypot(current.x - last.x, current.y - last.y);
      last = current;
    }

    const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
    std::cerr << "TRIAL " << useLaser << " distance " << travelled << " unique cells "
              << visited.size() << " status "
              << runtime.world.simulation.solver.Status(rover.GetEntity()) << " safety "
              << runtime.world.simulation.drive.StopReason(rover.GetEntity()) << "\n";
    Check(travelled > 5 && visited.size() > 50,
      "RL7 must visit new ground, not only oscillate, in 300 seconds");
    Check(map && map->accepted > 100, "Both sensor technologies populate the discovered map");
    std::cout << (useLaser ? "LiDAR" : "Ultrasonic") << ": path=" << travelled
              << " m; unique cells=" << visited.size() << "; accepted=" << map->accepted
              << "; stop=" << runtime.world.simulation.drive.StopReason(rover.GetEntity()) << '\n';

    rover.GetComponent<RoverNavigationComponent>()->enabled = true;

    runtime.FixedUpdate(1.f / 60);
    Check(runtime.world.simulation.solver.Status(rover.GetEntity())
            .starts_with("Disable Rover Navigation"),
      "Reject competing command owners");
    Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{},
      "Conflict stops motion");

    runtime.Reset();
    rover = runtime.ResolveSceneObject(2);

    runtime.Start();
    for (int i = 0; i < 30; ++i)
      runtime.FixedUpdate(1.f / 60);
    Check(runtime.world.simulation.mapping.Find(rover.GetEntity()), "Mapping restarts after Reset");
    Check(!runtime.world.simulation.solver.Status(rover.GetEntity()).starts_with("Disable"),
      "Reset clears runtime conflict");

    rover.GetComponent<RoverSolverComponent>()->noNewCellTimeout = 5;
    // This RL7 watchdog checks cell visitation in arrival-marker missions.
    // TimedExplore now uses the separate RL8 information-gain watchdog.
    rover.GetComponent<RoverSolverComponent>()->missionMode = "ArrivalMarker";
    runtime.freeze = true;
    for (int i = 0; i < 600; ++i)
      runtime.FixedUpdate(1.f / 60);
    Check(
      runtime.world.simulation.solver.Status(rover.GetEntity()).starts_with("Exploration stalled"),
      "Frozen rover must stop retrying within configured watchdog");

    runtime.freeze = false;
    runtime.Reset();
    runtime.Start();
    for (int i = 0; i < 30; ++i)
      runtime.FixedUpdate(1.f / 60);
    rover = runtime.ResolveSceneObject(2);
    runtime.world.simulation.GetScene().Components().Each<UltrasonicSensorComponent>([&](ecs::Entity
                                                                                         id) {
      runtime.world.simulation.GetScene().Components().Get<UltrasonicSensorComponent>(id)->enabled =
        false;
    });

    runtime.world.simulation.GetScene().Components().Each<LidarSensorComponent>(
      [&](ecs::Entity id) {
        runtime.world.simulation.GetScene().Components().Get<LidarSensorComponent>(id)->enabled =
          false;
      });

    runtime.FixedUpdate(1.f / 60);
    Check(runtime.world.simulation.solver.Status(rover.GetEntity()).starts_with("Waiting for four"),
      "Missing sensors block navigation");
    Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{},
      "Missing sensors stop motion");
  }
}

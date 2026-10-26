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
  Runtime() : SceneProjectRuntime("Sensor comparison test"), world(GetRuntimeScene()) {
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

protected:
  void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
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
    ("rover-sensor-scenes-" +
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
    // Keep the baseline controller fixture independent of the user's RL7 editor setup.
    document.RemoveComponent(2, "project.RoverSolverComponent");
    document.SetComponentProperty(2, "project.RoverNavigationComponent", "enabled", true);
    runtime.SynchronizeScene(document.GetObjects());
    runtime.UpdateAuthoringScene();
    auto rover = runtime.ResolveSceneObject(2);
    Check(rover.IsValid(), "Saved scene rover loads");
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
    auto last = start;
    for (int i = 0; i < 1800; ++i) {
      runtime.FixedUpdate(1.f / 60);
      const auto current = rover.GetComponent<Transform2DComponent>()->position;
      travelled += std::hypot(current.x - last.x, current.y - last.y);
      last = current;
    }
    const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
    Check(travelled > .5, "Comparison rover must move over half a metre in 30 simulated seconds");
    Check(map && map->accepted > 100, "Both sensor technologies populate the discovered map");
    std::cout << (useLaser ? "LiDAR" : "Ultrasonic") << ": path=" << travelled
              << " m; accepted=" << map->accepted
              << "; stop=" << runtime.world.simulation.drive.StopReason(rover.GetEntity()) << '\n';
    runtime.Reset();
    rover = runtime.ResolveSceneObject(2);
    runtime.Start();
    for (int i = 0; i < 30; ++i)
      runtime.FixedUpdate(1.f / 60);
    Check(runtime.world.simulation.mapping.Find(rover.GetEntity()), "Mapping restarts after Reset");
  }
}

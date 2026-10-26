#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Project/AssetDatabase.h>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

#include "Editor/RoverMapPanel.h"
#include "Integration/RoverLabAdapter.h"
#include "SceneSerializer.h"

using namespace pipeframe;
using namespace RoverLab;

namespace {

void Check(bool value, const std::string& message) {
  if (!value) throw std::runtime_error(message);
}

template <class T>
SceneComponentData Data(const T& value) {
  return T::Schema().Serialize(value);
}

class Runtime final : public SceneProjectRuntime {
public:
  Runtime() : SceneProjectRuntime("RL8 test"), world(GetRuntimeScene()) {
    SetSimulationWorld(world.simulation);
    RegisterProject(CreateProjectDefinition(), world);

    auto resolver = [this](SceneObjectId id) {
      return ResolveSceneObject(id);
    };
    world.simulation.ultrasonic.SetObjectResolver(resolver);
    world.simulation.lidar.SetObjectResolver(resolver);
    world.simulation.infrared.SetObjectResolver(resolver);
    world.simulation.navigation.SetObjectResolver(resolver);
    world.simulation.drive.SetObjectResolver(resolver);
    world.simulation.mapping.SetObjectResolver(resolver);
    world.simulation.solver.SetObjectResolver(resolver);
  }

  void UpdateAuthoringScene() override {
    world.ResetSensorState();
    world.simulation.hardware.Apply();
    world.simulation.ultrasonic.AlignMounts();
    world.simulation.lidar.AlignMounts();
    world.simulation.infrared.AlignMounts();
  }

  void Reset() override {
    world.ResetSensorState();
    SceneProjectRuntime::Reset();
  }

  void FixedUpdate(float delta) override {
    world.destinationTool.CaptureFrames(GetRuntimeScene());
    SceneProjectRuntime::FixedUpdate(delta);
  }

  void Step(int count = 1) {
    for (int i = 0; i < count; ++i)
      FixedUpdate(1.f / 60);
  }

  const RoverMission* Mission() {
    return world.simulation.solver.Mission(ResolveSceneObject(2).GetEntity());
  }

  ProjectWorld world;

protected:
  void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
    world.physics.Simulate(
      delta, [&](float step) { SceneProjectRuntime::UpdatePhysics(step, previous); });
  }
};

SceneObjectData Wall(Runtime& runtime, SceneObjectId id, Vector2f p, Vector2f size) {
  auto object = runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);
  object.id = id;
  object.transform.position = p;
  Transform2DComponent transform;
  transform.position = p;
  EnvironmentCollider2DComponent collider;
  collider.size = size;
  object.components = {Data(transform), Data(collider)};
  return object;
}

std::vector<SceneObjectData> Fixture(
  Runtime& runtime, bool laser, Vector2f goalPosition = {1.2f, 0}) {
  auto rover = runtime.CreateDefaultObject("project.RoverEntity");
  rover.id = 2;

  EnvironmentCollider2DComponent collider;
  collider.size = {.3f, .2f};
  RoverMappingComponent mapping;
  mapping.cellSize = .05;
  RoverSolverComponent solver;
  solver.missionMode = "ArrivalMarker";
  solver.noNewCellTimeout = 120;
  solver.runTimeout = 600;

  rover.components = {Data(Transform2DComponent{}),
    Data(KinematicBody2DComponent{}),
    Data(collider),
    Data(RoverDriveComponent{}),
    Data(RoverSafetyComponent{}),
    Data(mapping),
    Data(solver)};

  std::vector<SceneObjectData> objects{rover};
  if (laser) {
    auto sensor = runtime.CreateDefaultObject("project.LidarSensorEntity");
    sensor.id = 8;
    LidarSensorComponent lidar;
    lidar.parent.objectId = 2;
    sensor.components = {Data(Transform2DComponent{}), Data(lidar)};
    objects.push_back(sensor);
  } else {
    for (int direction = 0; direction < 4; ++direction) {
      auto sensor = runtime.CreateDefaultObject("project.UltrasonicSensorEntity");
      sensor.id = 8 + direction;
      UltrasonicSensorComponent sonar;
      sonar.parent.objectId = 2;
      sonar.localAngleDegrees = direction == 3 ? -90 : 90 * direction;
      sonar.localPosition = direction == 0   ? Vector2f{.16f, 0}
                            : direction == 1 ? Vector2f{0, .09f}
                            : direction == 2 ? Vector2f{-.16f, 0}
                                             : Vector2f{0, -.09f};
      sensor.components = {Data(Transform2DComponent{}), Data(sonar)};
      objects.push_back(sensor);
    }
  }

  objects.push_back(Wall(runtime, 20, {1, -.55f}, {4, .1f}));
  objects.push_back(Wall(runtime, 21, {1, .55f}, {4, .1f}));
  objects.push_back(Wall(runtime, 22, {-.6f, 0}, {.1f, 1.2f}));
  objects.push_back(Wall(runtime, 23, {2.6f, 0}, {.1f, 1.2f}));

  auto goal = runtime.CreateDefaultObject("project.RoverGoalEntity");
  goal.id = 30;
  goal.transform.position = goalPosition;
  Transform2DComponent transform;
  transform.position = goalPosition;
  RoverGoalComponent marker;
  marker.radius = .30;
  goal.components = {Data(transform), Data(marker)};
  objects.push_back(goal);
  return objects;
}

void Load(Runtime& runtime, bool laser, Vector2f goal = {1.2f, 0}) {
  std::string error;
  Check(runtime.Load({}, error), error);
  runtime.SynchronizeScene(Fixture(runtime, laser, goal));
  runtime.UpdateAuthoringScene();
  runtime.Start();
}

void RunUntilFinished(Runtime& runtime, const std::function<void()>& afterStep = {}) {
  std::string lastStatus;
  const auto& config = *runtime.ResolveSceneObject(2).GetComponent<RoverSolverComponent>();
  // An unlimited mission has no production timer. Bound this diagnostic replay
  // independently so a genuine loop still fails the test rather than hanging.
  double limit =
    config.missionMode == "TimedExplore" && config.explorationDuration == "Until complete"
      ? 3600
      : config.runTimeout + 10;
  // Return & Repeat contains two independently bounded legs.
  if (runtime.Mission() && runtime.Mission()->phase == MissionPhase::ReturnToStart) limit *= 2;
  for (int i = 0; i < 60 * limit; ++i) {
    runtime.Step();
    if (afterStep) afterStep();
    const auto* mission = runtime.Mission();
    if (mission && mission->Finished()) return;
    const auto rover = runtime.ResolveSceneObject(2);
    const auto status = runtime.world.simulation.solver.Status(rover.GetEntity());
    if (status != lastStatus &&
        (status.starts_with("No turning space") || status.starts_with("Recovery ended") ||
          status.starts_with("Reverse recovery") || status.starts_with("Forward retreat") ||
          status.starts_with("Recorded return") || status.starts_with("Small trace") ||
          status.starts_with("Learned route uncertain") ||
          status.starts_with("Drive refused route") ||
          status.starts_with("Map changed along route") ||
          status.starts_with("Route made no progress"))) {
      const auto* pose = rover.GetComponent<Transform2DComponent>();
      std::cerr << "t=" << i / 60.0 << " " << status << " at " << pose->position.x << ","
                << pose->position.y << '\n';
    }
    lastStatus = status;
    if (i % 3600 == 0) {
      const auto rover = runtime.ResolveSceneObject(2);
      const auto* pose = rover.GetComponent<Transform2DComponent>();
      std::cerr << "tick " << i << " position " << pose->position.x << "," << pose->position.y
                << " " << runtime.world.simulation.solver.Status(rover.GetEntity()) << '\n';
    }
  }
  throw std::runtime_error("Mission did not terminate within bounded simulation time");
}

void CheckFloorSearch() {
  MazeGrid grid;
  grid.width = 80;
  grid.height = 40;
  grid.cell = .05;
  grid.open.assign(grid.Count(), 1);
  grid.clear.assign(grid.Count(), 3);
  grid.frontier.assign(grid.Count(), 255);
  grid.evidence.assign(grid.Count(), -12);
  grid.visits.assign(grid.Count(), 0);
  MazeMemory memory;
  memory.searchRegion = MazeKey{0, 0};
  const int start = grid.At({2.3, .8});

  // A nearer target in a new patch must not starve the current patch interior.
  int target = ChooseCoverageTarget(grid, start, memory);
  Check(target >= 0 && grid.Point(target).x < 2,
    "Finish current search patch despite nearer free floor and abundant frontiers");

  // Seeing / physically covering floor is not proof the centre reached a marker.
  for (int i = 0; i < grid.Count(); ++i)
    memory.covered.insert(grid.Key(i));
  target = ChooseCoverageTarget(grid, start, memory);
  Check(target >= 0, "Chassis coverage must not suppress arrival search");

  for (int i = 0; i < grid.Count(); ++i)
    if (grid.Point(i).x < 2) memory.searched.insert(grid.Key(i));
  target = ChooseCoverageTarget(grid, start, memory);
  Check(target >= 0 && grid.Point(target).x >= 2,
    "Move to another patch once the current one is searched");

  for (int i = 0; i < grid.Count(); ++i)
    memory.searched.insert(grid.Key(i));
  Check(ChooseCoverageTarget(grid, start, memory) < 0, "Do not repeatedly search finished floor");

  // The last strip of a patch must not wait for discovery of a neighbouring patch.
  memory.searched.clear();
  memory.searchRegion = MazeKey{0, 0};
  for (int i = 0; i < grid.Count(); ++i)
    if (grid.Point(i).y < 1.9) memory.searched.insert(grid.Key(i));
  target = ChooseCoverageTarget(grid, grid.At({1, 1.8}), memory);
  Check(target >= 0 && grid.Point(target).y >= 1.9 && grid.Point(target).x < 2,
    "Search the final strip before declaring a patch finished");

  // Expanding left/up must preserve both pending work and physical search history.
  MazeGrid expanded = grid;
  expanded.width += 10;
  expanded.height += 10;
  expanded.x = expanded.y = -.5;
  expanded.open.assign(expanded.Count(), 0);
  expanded.clear.assign(expanded.Count(), 3);
  expanded.evidence.assign(expanded.Count(), 0);
  expanded.visits.assign(expanded.Count(), 0);
  for (int i = 0; i < grid.Count(); ++i) {
    const int j = expanded.At(grid.Point(i));
    expanded.open[j] = grid.open[i];
    expanded.evidence[j] = grid.evidence[i];
  }
  const int expandedTarget = ChooseCoverageTarget(expanded, expanded.At({1, 1.8}), memory);
  Check(expandedTarget >= 0 && expanded.Key(expandedTarget) == grid.Key(target),
    "Grid growth preserves the chosen physical search location");
}

void CheckRetryPolicy() {
  MazeGrid grid;
  grid.width = grid.height = 11;
  grid.cell = .1;
  grid.evidence.assign(121, 0);
  MazeMemory memory;
  const auto key = grid.Key(60);

  for (int i = 0; i < 3; ++i) {
    RememberFrontier(grid, memory, key);
    Check(!FrontierAvailable(grid, 60, memory), "Cooldown prevents immediate repeated turn");
    memory.time += 61;
  }
  Check(!FrontierAvailable(grid, 60, memory), "Three unchanged attempts exhaust this target");
  grid.evidence[60] = -1;
  Check(FrontierAvailable(grid, 60, memory), "New local evidence reopens a rejected target");
  RememberFrontier(grid, memory, key);
  memory.time += 61;
  RememberFrontier(grid, memory, key);
  memory.time += 61;
  RememberFrontier(grid, memory, key);
  memory.time += 61;
  grid.evidence[60] = -12;
  Check(!FrontierAvailable(grid, 60, memory), "Repeated evidence strength is not new geometry");
}

}  // namespace

Widget* FindWidget(Widget& root, const std::string& key) {
  if (root.GetKey() == key) return &root;
  for (std::size_t i = 0; i < root.GetChildCount(); ++i)
    if (auto* found = FindWidget(*root.GetChild(i), key)) return found;
  return nullptr;
}

const ui::View* FindView(const ui::View& root, const std::string& key) {
  if (root.key == key) return &root;
  for (const auto& child : root.children)
    if (const auto* found = FindView(child, key)) return found;
  return nullptr;
}

void CheckMissionPanelSetup() {
  BehaviourScene scene;
  ProjectWorld world(scene);
  ui::ProjectUIHost host;
  RoverMissionPanel panel(world, host);
  auto view = panel.DescribeView();
  Check(FindView(view, "solver-missing"), "Missing solver explains scene setup");

  auto rover = scene.CreateObject();
  auto& config = rover.AddComponent<RoverSolverComponent>();
  const auto key = "mission-" + std::to_string(rover.GetEntity());
  config.enabled = false;
  view = panel.DescribeView();
  Check(FindView(view, key + "-disabled") && !FindView(view, "solver-missing"),
    "Disabled solver explains how to enable it");

  config.enabled = true;
  rover.SetActive(false);
  view = panel.DescribeView();
  Check(FindView(view, key + "-inactive"), "Inactive rover explains how to activate it");

  rover.SetActive(true);
  view = panel.DescribeView();
  const auto* five = FindView(view, "five");
  const auto* ten = FindView(view, "ten");
  const auto* select = FindView(view, key + "-select");
  const auto* unlimited = FindView(view, key + "-until-complete");
  const auto* finish = FindView(view, key + "-finish");
  Check(five && ten && select && unlimited && finish && !unlimited->enabled && !finish->enabled &&
          !five->enabled && !ten->enabled && !select->enabled,
    "Configured rover displays controls before Play and gates them on runtime readiness");
}

void CheckTimedWorkflow(bool laser, double duration) {
  RoverDestinationTool::Frame offsetFrame{{}, {12, -7}, float(std::numbers::pi / 2)};
  const auto scenePoint = offsetFrame.ToScene({2, 3});
  Check(std::hypot(scenePoint.x - 9, scenePoint.y + 5) < .0001,
    "Rotated starting frame displays the correct scene destination");
  const auto mapPoint = offsetFrame.ToMap(scenePoint);
  Check(std::hypot(mapPoint.x - 2, mapPoint.y - 3) < .0001,
    "Scene clicks invert translation and initial heading");

  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects =
    Fixture(runtime, laser, {0, 0});  // Marker at spawn must not end timed exploration.
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      config.explorationDuration = duration == 600 ? "10 minutes" : "5 minutes";
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  runtime.Step(60);
  const auto rover = runtime.ResolveSceneObject(2);
  auto& solver = runtime.world.simulation.solver;
  Check(runtime.Mission() && runtime.Mission()->timed, "Timed mission initialized");
  Check(!solver.SelectDestination(rover.GetEntity(), {0, 0}), "Cannot select while exploring");
  runtime.Step(int((duration - 2) * 60));
  Check(runtime.Mission()->phase == MissionPhase::Explore ||
          runtime.Mission()->phase == MissionPhase::AwaitDestination,
    "Exploration either continues within budget or stops with an explicit outcome");
  runtime.Step(125);
  auto* mission = runtime.Mission();
  std::cerr << (laser ? "LiDAR" : "Sonar") << " timed " << duration << " "
            << MissionPhaseText(mission->phase) << "\n";
  Check(mission->phase == MissionPhase::AwaitDestination && !mission->learnedGoal,
    "Budget ends in destination selection, not marker success or timeout");
  const auto before = rover.GetComponent<Transform2DComponent>()->position;
  runtime.Step(60);
  const auto after = rover.GetComponent<Transform2DComponent>()->position;
  Check(std::hypot(after.x - before.x, after.y - before.y) < .002, "Stopped while awaiting click");
  Check(!solver.SelectDestination(rover.GetEntity(), {1000, 1000}), "Reject unknown/outside map");
  Check(
    runtime.Mission()->phase == MissionPhase::AwaitDestination, "Invalid click keeps ready state");
  const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
  const auto generation = map->generation;
  const auto count = map->accepted;
  std::cerr << "Ready pose " << map->pose.x << "," << map->pose.y << " h=" << map->pose.heading
            << "\n";
  const MazePoint destination{0, 0};
  ui::ProjectUIHost host;
  RoverMapPanel panel(runtime.world, host);
  UIManager manager;
  Check(manager.LoadDefaultFont(
          std::string(ROVER_PROJECT_ROOT) + "/../../assets/fonts/roboto_regular.ttf"),
    "Minimap font loads");
  auto view = panel.DescribeView();
  auto mounted = manager.MountView(view);
  mounted.SetBounds(0, 0, 420, 780);
  manager.Update(0);
  const auto key = "map-" + std::to_string(rover.GetEntity());
  const auto* description = FindView(view, key);
  const auto* viewport = FindWidget(*manager.GetRoot(0), key);
  Check(description && viewport, "Actual minimap viewport mounted");
  const auto size = viewport->GetSize();
  const float scale = std::min(size.x, size.y) / description->camera.span;
  const auto screen = viewport->GetScreenPosition() + size * .5f +
                      sf::Vector2f{float(destination.x) - description->camera.center.x,
                        float(destination.y) - description->camera.center.y} *
                        scale;
  const sf::Vector2i pixel{int(std::round(screen.x)), int(std::round(screen.y))};
  manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, pixel});
  manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, pixel});
  Check(runtime.Mission()->phase == MissionPhase::AwaitDestination,
    "Minimap click only inspects the map, never selects a destination");

  sf::RenderTexture sceneImage({800, 800});
  auto context = pipeframe::backend::sfml::MakeRenderContext(sceneImage);
  context.SetCameraCenter({0, 0});
  context.SetCameraSize({10, 10});
  auto& tool = runtime.world.destinationTool;
  RoverMissionPanel controls(runtime.world, host);
  auto controlsView = controls.DescribeView();
  const auto* select =
    FindView(controlsView, "mission-" + std::to_string(rover.GetEntity()) + "-select");
  Check(select && select->enabled && select->onPressed,
    "Scene cursor button available after exploration");
  select->onPressed();
  Check(tool.IsArmed(rover.GetEntity()), "Rover UI arms scene destination cursor");
  const auto scenePixel = context.MapWorldToPixel({0, 0});
  const InputEvent press{
    InputEventType::PointerPressed, PointerInput{PointerButton::Left, scenePixel}};
  const InputEvent release{
    InputEventType::PointerReleased, PointerInput{PointerButton::Left, scenePixel}};
  context.SetCameraCenter({1000, 1000});
  tool.Handle(press, context, solver);
  tool.Handle(release, context, solver);
  Check(runtime.Mission()->CanSelectDestination() && tool.IsArmed(rover.GetEntity()),
    "Unknown scene destination is refused while keeping selection armed");
  context.SetCameraCenter({0, 0});
  Check(
    tool.Handle(press, context, solver) && tool.HasCapture(), "Scene click owns pointer capture");
  tool.Handle({InputEventType::PointerMoved, PointerMoveInput{{scenePixel.x + 10, scenePixel.y}}},
    context,
    solver);
  tool.Handle(release, context, solver);
  Check(runtime.Mission()->CanSelectDestination(), "Dragging cannot place a destination");
  tool.Handle({InputEventType::KeyPressed, KeyInput{InputKey::Escape}}, context, solver);
  Check(!tool.IsArmed(rover.GetEntity()) && !tool.HasCapture(), "Escape restores normal cursor");
  select->onPressed();
  tool.Handle(press, context, solver);
  tool.Handle({InputEventType::FocusChanged, FocusInput{false}}, context, solver);
  tool.Handle(release, context, solver);
  Check(runtime.Mission()->CanSelectDestination(), "Focus loss cancels a pending click");
  tool.Handle(press, context, solver);
  tool.Handle(release, context, solver);
  Check(runtime.Mission()->phase == MissionPhase::Navigate && !tool.IsArmed(rover.GetEntity()),
    "Scene click selects destination and returns to normal cursor");
  runtime.Step(1);
  std::cerr << "Planned: " << solver.Status(rover.GetEntity()) << " | "
            << runtime.Mission()->selectionMessage << "\n";
  Check(runtime.Mission()->phase == MissionPhase::Navigate, "Known route starts");
  Check(!solver.PlannedPath(rover.GetEntity()).empty(), "Navigation exposes visible route");
  const auto sceneGeometry = tool.Geometry(solver, .1f);
  Check(!sceneGeometry.Vertices().empty(), "Scene route and destination overlay built");
  sceneImage.clear();
  context.BeginWorld();
  sceneGeometry.Draw(context.GetCanvas());
  sceneImage.display();
  Check(sceneImage.getTexture().copyToImage().saveToFile(
          laser ? "/tmp/rl8-lidar-scene-route.png" : "/tmp/rl8-ultrasonic-scene-route.png"),
    "Save rendered scene route");
  panel.OnUpdate(.2f);
  view = panel.DescribeView();
  description = FindView(view, key);
  Check(description && description->mesh.size() == 2 && !description->mesh[1].vertices.empty(),
    "Route and destination geometry are present in minimap");
  mounted.SetView(view);
  manager.Update(0);
  sf::RenderTexture image({420, 780});
  image.clear(sf::Color{20, 26, 34});
  manager.Render(image);
  image.display();
  const auto pixels = image.getTexture().copyToImage();
  int cyan = 0, yellow = 0;
  for (unsigned y = 0; y < 780; ++y)
    for (unsigned x = 0; x < 420; ++x) {
      cyan += pixels.getPixel({x, y}) == sf::Color(40, 240, 225);
      yellow += pixels.getPixel({x, y}) == sf::Color(255, 215, 45);
    }
  Check(cyan > 0 && yellow > 0, "Native minimap actually renders cyan path and yellow destination");
  Check(pixels.saveToFile(laser ? "/tmp/rl8-lidar-route.png" : "/tmp/rl8-ultrasonic-route.png"),
    "Save minimap route render");
  for (const auto point : solver.PlannedPath(rover.GetEntity()))
    std::cerr << "Path " << point.x << "," << point.y << "\n";
  std::string last;
  for (int i = 0; i < 300 * 60 && runtime.Mission()->phase == MissionPhase::Navigate; ++i) {
    runtime.Step();
    const auto status = solver.Status(rover.GetEntity());
    if (status != last && (status.find("refused") != std::string::npos ||
                            status.find("changed") != std::string::npos ||
                            status.find("progress") != std::string::npos)) {
      const auto* value = runtime.world.simulation.mapping.Find(rover.GetEntity());
      std::cerr << "t=" << i / 60.0 << " " << status << " at " << value->pose.x << ","
                << value->pose.y << " h=" << value->pose.heading << "\n";
    }
    last = status;
  }
  std::cerr << "Arrival: " << MissionPhaseText(runtime.Mission()->phase) << " | "
            << runtime.Mission()->selectionMessage << "\n";
  Check(runtime.Mission()->phase == MissionPhase::Arrived, "Physically reach clicked destination");
  map = runtime.world.simulation.mapping.Find(rover.GetEntity());
  Check(std::hypot(map->pose.x, map->pose.y) < .08, "Arrival verified by estimated pose");
  Check(map->generation == generation && map->accepted >= count, "Navigation retains learned map");
  Check(solver.SelectDestination(rover.GetEntity(), {.8, 0}), "Can select another destination");
  runtime.Step(1);
  Check(solver.CancelDestination(rover.GetEntity()), "Can cancel navigation");
  runtime.Step(1);
  Check(runtime.Mission()->CanSelectDestination() && solver.PlannedPath(rover.GetEntity()).empty(),
    "Cancel clears route and permits reselection");
  Check(solver.BeginTimedExploration(rover.GetEntity(), 300), "Can explore more with retained map");
  runtime.Step();
  Check(runtime.world.simulation.mapping.Find(rover.GetEntity())->generation == generation,
    "Additional exploration retains map");
  const auto roverId = rover.GetEntity();
  runtime.Reset();
  Check(!runtime.Mission() && !tool.IsArmed(roverId) && !tool.HasCapture(),
    "Reset clears mission and scene cursor selection");
}

void CheckLidarStartExit(const char* algorithm, bool liveSwitch = false) {
  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects = Fixture(runtime, true);
  std::erase_if(objects, [](const auto& o) { return o.id >= 20; });
  // Start facing the side of a narrow entry corridor. The route must continue
  // past the wall end before turning into the room, not repeatedly turn early.
  objects.push_back(Wall(runtime, 20, {-.45f, 1.75f}, {.1f, 4.5f}));
  objects.push_back(Wall(runtime, 21, {.45f, .25f}, {.1f, 1.5f}));
  objects.push_back(Wall(runtime, 22, {1.5f, -.5f}, {4.f, .1f}));
  objects.push_back(Wall(runtime, 23, {3.5f, 1.75f}, {.1f, 4.5f}));
  objects.push_back(Wall(runtime, 24, {1.5f, 4.f}, {4.f, .1f}));
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      config.missionMode = "TimedExplore";
      config.explorationDuration = "Until complete";
      config.algorithm = liveSwitch ? "AStar" : algorithm;
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  bool exited = false;
  int exitTick = 0;
  for (; exitTick < 90 * 60; ++exitTick) {
    if (liveSwitch && exitTick == 120)
      runtime.ResolveSceneObject(2).GetComponent<RoverSolverComponent>()->algorithm = algorithm;
    runtime.Step();
    const auto& pose = *runtime.ResolveSceneObject(2).GetComponent<Transform2DComponent>();
    if (pose.position.x > .85 && pose.position.y > 1.2) {
      exited = true;
      break;
    }
  }
  std::cerr << algorithm << (liveSwitch ? " live switch" : " fresh start")
            << " corridor exit: " << exited << " after " << exitTick / 60.0 << " seconds; "
            << runtime.world.simulation.solver.Status(runtime.ResolveSceneObject(2).GetEntity())
            << '\n';
  Check(exited, "LiDAR exploration leaves its starting corridor within 90 simulated seconds");
}

void CheckLidarGapSafety(bool navigate) {
  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects = Fixture(runtime, true);
  std::erase_if(objects, [](const auto& object) { return object.id >= 20; });
  // Staggered wall ends leave a horizontal passage through y=0. The rear
  // wall tip sits at about 150 degrees, outside the .13 m half-width envelope.
  objects.push_back(Wall(runtime, 20, {-.4f, 1.1f}, {.1f, 1.8f}));
  objects.push_back(Wall(runtime, 21, {.65f, -1.1f}, {.1f, 1.8f}));
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      config.enabled = false;
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  runtime.Step(120);
  RoverMotionCommandEvent command;
  command.roverId = 2;
  command.forwardSpeed = -.1;
  const auto rover = runtime.ResolveSceneObject(2);
  if (navigate) {
    rover.GetComponent<RoverSolverComponent>()->enabled = true;
    runtime.Step();
    auto& solver = runtime.world.simulation.solver;
    Check(solver.FinishExploration(rover.GetEntity()), "Use observed map for gap destination test");
    Check(solver.SelectDestination(rover.GetEntity(), {-.85, -.35}),
      "Select destination across staggered wall gap");
  }
  int falseStops = 0;
  for (int i = 0; i < (navigate ? 120 : 12) * 60; ++i) {
    if (navigate && runtime.Mission()->phase != MissionPhase::Navigate) break;
    if (!navigate) runtime.world.simulation.events.Publish(command);
    runtime.Step();
    falseStops +=
      runtime.world.simulation.drive.SafetyStatus(rover.GetEntity()) == RoverSafetyStatus::Obstacle;
  }
  const auto& pose = *rover.GetComponent<Transform2DComponent>();
  std::cerr << "LiDAR gap reverse: " << pose.position.x << "," << pose.position.y
            << " obstacle stops=" << falseStops << "\n";
  if (navigate) {
    std::cerr << "Gap destination: " << MissionPhaseText(runtime.Mission()->phase) << " "
              << runtime.Mission()->selectionMessage << "\n";
    Check(runtime.Mission()->phase == MissionPhase::Arrived && falseStops == 0,
      "Planned destination route crosses the gap without repeated rear-sensor recovery");
    Check(std::hypot(pose.position.x + .85, pose.position.y + .35) <= .075,
      "Physical rover reaches the gap destination");
  } else
    Check(pose.position.x < -1 && std::abs(pose.position.y) < .02 && falseStops == 0,
      "Physical reverse motion crosses staggered wall gap without a fictitious rear obstacle");
}

void CheckCornerDeparture() {
  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects = Fixture(runtime, true);
  std::erase_if(objects, [](const auto& o) { return o.id >= 20 && o.id <= 23; });
  objects.push_back(Wall(runtime, 20, {-.6f, -1.1f}, {2.0f, .1f}));
  objects.push_back(Wall(runtime, 21, {-.6f, 1.1f}, {2.0f, .1f}));
  objects.push_back(Wall(runtime, 22, {-1.5f, 0}, {.1f, 2.3f}));
  objects.push_back(Wall(runtime, 23, {.29f, 0}, {.1f, 2.3f}));
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      config.enabled = false;
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  runtime.Step(120);
  // Reach a non-cardinal pose through real Drive/physics/odometry, not by
  // injecting true coordinates into the learned map.
  RoverMotionCommandEvent command;
  command.roverId = 2;
  command.turnRate = .08;
  for (int i = 0; i < 60; ++i) {
    runtime.world.simulation.events.Publish(command);
    runtime.Step();
  }
  command.turnRate = 0;
  runtime.world.simulation.events.Publish(command);
  runtime.Step(60);
  const auto rover = runtime.ResolveSceneObject(2);
  const auto entity = rover.GetEntity();
  rover.GetComponent<RoverSolverComponent>()->enabled = true;
  runtime.Step();
  auto& solver = runtime.world.simulation.solver;
  Check(solver.FinishExploration(entity), "Finish corner fixture mapping");
  const auto* map = runtime.world.simulation.mapping.Find(entity);
  MazeMemory memory;
  const auto grid = BuildMazeGrid(*map, memory, .16, .10, .03, 0, .15, .10, false, true);
  const int start = grid.At({map->pose.x, map->pose.y});
  Check(std::abs(map->pose.heading) > .03 && !grid.turnClear[start],
    "Corner fixture starts angled beside a wall without full turning clearance");
  auto cardinalOnly = grid;
  cardinalOnly.poseHeadings = 0;
  Check(FastestKnownRoute(cardinalOnly, start, grid.At({-.6, .4}), map->pose.heading, .1, 1)
          .cells.empty(),
    "Cardinal-only planner cannot depart the corner pose");
  const bool alignmentRoute =
    !FastestKnownRoute(grid, start, grid.At({-.6, .4}), map->pose.heading, .1, 1).cells.empty();
  Check(solver.SelectDestination(entity, {-.6, .4}), "Choose known free destination behind corner");
  Check(!solver.SelectDestination(entity, {-.4, .3}) && runtime.Mission()->destination->x == -.6,
    "Active destination cannot be replaced before arrival");
  bool departed = false;
  for (int i = 0; i < 120 * 60 && runtime.Mission()->phase == MissionPhase::Navigate; ++i) {
    runtime.Step();
    departed |= solver.Status(entity).starts_with("Moving to known turning space");
  }
  std::cerr << "Corner departure: " << MissionPhaseText(runtime.Mission()->phase) << " "
            << runtime.Mission()->selectionMessage << "\n";
  Check(departed || alignmentRoute,
    "Navigation uses verified alignment clearance or moves to known safe turning space");
  Check(runtime.Mission()->phase == MissionPhase::Arrived,
    "Real rover backs out safely and reaches the chosen corner destination");
  map = runtime.world.simulation.mapping.Find(entity);
  Check(std::hypot(map->pose.x + .6, map->pose.y - .4) <= .075,
    "Corner arrival requires actual estimated position at the destination");

  // A visible floor point can still be too close to a wall for the whole rover.
  Check(
    solver.SelectDestination(entity, {.18, 0}), "Accept known floor for route feasibility check");
  runtime.Step();
  Check(runtime.Mission()->phase == MissionPhase::NavigationBlocked &&
          !runtime.Mission()->CanSelectDestination(),
    "Unreachable goal is blocked, never ready or arrived");
  Check(!solver.SelectDestination(entity, {-.5, 0}) && !solver.BeginTimedExploration(entity, 300),
    "Blocked destination cannot be silently replaced");
  ui::ProjectUIHost host;
  RoverMissionPanel panel(runtime.world, host);
  const auto view = panel.DescribeView();
  const auto key = "mission-" + std::to_string(entity);
  Check(FindView(view, key + "-navigation-state") && !FindView(view, key + "-select")->enabled &&
          FindView(view, key + "-retry")->enabled && FindView(view, key + "-cancel")->enabled,
    "Blocked panel exposes status, retry and cancel while disabling destination selection");
  Check(solver.RetryDestination(entity), "Explicit retry retains the same destination");
  Check(runtime.Mission()->destination->x == .18, "Retry keeps the selected goal");
  runtime.Step();
  Check(solver.CancelDestination(entity), "Explicit cancel acknowledges failed navigation");
  Check(runtime.Mission()->CanSelectDestination() && !runtime.Mission()->destination,
    "Cancel clears the marker and unlocks another destination");
}

void CheckExplorationOutcomes(bool laser) {
  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects = Fixture(runtime, laser);
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  runtime.Step(120);
  const auto entity = runtime.ResolveSceneObject(2).GetEntity();
  auto& solver = runtime.world.simulation.solver;
  Check(runtime.Mission()->explorationLimit == 0, "Default exploration has no time limit");
  Check(runtime.Mission()->newArea > 0, "Newly observed area is measured");
  const auto* map = runtime.world.simulation.mapping.Find(entity);
  const auto generation = map->generation;
  Check(solver.FinishExploration(entity), "User can finish exploration early");
  Check(runtime.Mission()->explorationOutcome == "Stopped by user" &&
          runtime.Mission()->CanSelectDestination(),
    "Manual finish unlocks destination selection");
  Check(solver.BeginTimedExploration(entity, 0), "Unlimited exploration can resume");
  Check(runtime.world.simulation.mapping.Find(entity)->generation == generation,
    "Resume retains the learned map");
  for (int i = 0; i < 900 && runtime.Mission()->phase == MissionPhase::Explore; ++i)
    runtime.Step(60);
  Check(runtime.Mission()->phase == MissionPhase::AwaitDestination,
    "Enclosed fixture ends without an arbitrary time budget");
  const auto outcome = runtime.Mission()->explorationOutcome;
  std::cerr << (laser ? "LiDAR" : "Sonar") << " unlimited: " << outcome
            << " area=" << runtime.Mission()->newArea
            << " travel=" << runtime.Mission()->explorationDistance << "\n";
  Check(outcome.starts_with("Complete:") || outcome == "Blocked or uncertain",
    "Unlimited run reports completion or unresolved exploration honestly");
}

void CheckExplorationDeadline(double duration) {
  Runtime runtime;
  std::string error;
  Check(runtime.Load({}, error), error);
  auto objects = Fixture(runtime, true);
  std::erase_if(objects, [](const auto& object) { return object.id == 8; });
  for (auto& component : objects.front().components)
    if (component.typeId == "project.RoverSolverComponent") {
      RoverSolverComponent config;
      config.noNewCellTimeout = 600;
      config.explorationDuration = duration == 600 ? "10 minutes" : "5 minutes";
      component = Data(config);
    }
  runtime.SynchronizeScene(objects);
  runtime.UpdateAuthoringScene();
  runtime.Start();
  runtime.Step(int(duration * 60) - 1);
  Check(runtime.Mission()->phase == MissionPhase::Explore,
    "Budget does not end early without sensor data");
  runtime.Step(2);
  Check(runtime.Mission()->phase == MissionPhase::AwaitDestination &&
          runtime.Mission()->explorationOutcome == "Time limit reached",
    "Deadline remains effective without sensors and does not claim completion");
}

// Exercise the real controller and Drive, measuring motion rather than just
// inspecting the planned route. Sensor outages must still stop a known leg.
void CheckContinuousStraight(bool laser) {
  Runtime runtime;
  Load(runtime, laser);
  RunUntilFinished(runtime);
  Check(
    runtime.Mission()->phase == MissionPhase::GoalFound, "Learn a successful corridor route first");
  const auto rover = runtime.ResolveSceneObject(2);
  auto& solver = runtime.world.simulation.solver;
  rover.GetComponent<RoverSolverComponent>()->missionMode = "TimedExplore";
  runtime.Step(1);
  Check(solver.FinishExploration(rover.GetEntity()), "Finish corridor mapping");
  runtime.Step(1);
  Check(solver.SelectDestination(rover.GetEntity(), {0, 0}), "Accept known straight destination");
  int movingSamples = 0;
  bool outageChecked = false;
  for (int i = 0; i < 120 * 60 && runtime.Mission()->phase == MissionPhase::Navigate; ++i) {
    runtime.Step();
    const auto* pose = rover.GetComponent<Transform2DComponent>();
    const auto* body = rover.GetComponent<KinematicBody2DComponent>();
    if (pose->position.x > .2 && pose->position.x < .7) {
      Check(std::hypot(body->velocity.x, body->velocity.y) > .02,
        "Known straight route must not pulse to a stop between waypoints");
      ++movingSamples;
      if (!outageChecked && pose->position.x < .4) {
        auto sensor = runtime.ResolveSceneObject(8);
        if (laser)
          sensor.GetComponent<LidarSensorComponent>()->enabled = false;
        else
          sensor.GetComponent<UltrasonicSensorComponent>()->enabled = false;
        runtime.Step(60);
        Check(body->velocity == Vector2f{}, "Missing fresh readings stop continuous motion");
        const auto stopped = pose->position;
        runtime.Step(30);
        Check(pose->position == stopped, "Rover stays stopped during sensor outage");
        if (laser)
          sensor.GetComponent<LidarSensorComponent>()->enabled = true;
        else
          sensor.GetComponent<UltrasonicSensorComponent>()->enabled = true;
        runtime.Step(30);
        outageChecked = true;
      }
    }
  }
  Check(movingSamples > 100 && outageChecked, "Continuous corridor and outage were exercised");
  Check(runtime.Mission()->phase == MissionPhase::Arrived, "Straight route reaches destination");
  const auto stopped = rover.GetComponent<Transform2DComponent>()->position;
  runtime.Step(120);
  Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{} &&
          rover.GetComponent<Transform2DComponent>()->position == stopped,
    "Destination arrival stays stationary");
  std::cout << (laser ? "LiDAR" : "Ultrasonic") << " continuous straight samples: " << movingSamples
            << '\n';
}

void CheckMeasuredRouteSafety() {
  for (const bool laser : {false, true}) {
    Runtime runtime;
    Load(runtime, laser);
    const auto rover = runtime.ResolveSceneObject(2);
    auto& hardware = rover.AddComponent<MicromouseHardwareComponent>();
    hardware.length = .3;
    hardware.width = .2;
    hardware.trackWidth = .16;
    RunUntilFinished(runtime);
    Check(runtime.Mission()->phase == MissionPhase::GoalFound,
      "Hardware rover learns a real arrival before replay safety check");

    // Move an existing wall into the learned return corridor after discovery.
    // A recorded clear pose must never overrule a fresh rear-sensor obstacle.
    runtime.ResolveSceneObject(22).GetComponent<Transform2DComponent>()->position = {.45f, 0};
    Check(runtime.world.simulation.solver.ReturnAndRepeat(rover.GetEntity()),
      "Start measured return with changed environment");
    for (int i = 0; i < 600 && !runtime.Mission()->Finished(); ++i)
      runtime.Step();
    Check(runtime.Mission()->phase == MissionPhase::Failed &&
            runtime.Mission()->failure.find("sensor") != std::string::npos,
      "Fresh sensor obstruction refuses the measured return, for sonar and LiDAR");
    Check(rover.GetComponent<Transform2DComponent>()->position.x > .6f &&
            !runtime.Mission()->repeat.success,
      "Replay never drives through the new wall or claims a repeat arrival");
  }
}

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "continuous") {
    CheckContinuousStraight(false);
    CheckContinuousStraight(true);
    return 0;
  }
  // Explicit diagnostic mode: run a copy of an authored scene, never save it.
  if (argc > 1) {
    const std::filesystem::path root = ROVER_PROJECT_ROOT;
    const auto sandbox =
      std::filesystem::temp_directory_path() /
      ("rover-search-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(sandbox / ".pipeframe");
    struct Cleanup {
      std::filesystem::path path;
      ~Cleanup() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
      }
    } cleanup{sandbox};
    std::filesystem::copy(
      root / "Assets", sandbox / "Assets", std::filesystem::copy_options::recursive);
    std::filesystem::copy_file(root / ".pipeframe/assets.db", sandbox / ".pipeframe/assets.db");
    std::string error;
    assets::AssetDatabase database;
    Check(database.Open(sandbox, &error), error);
    ServiceRegistry services;
    services.Provide(database);
    auto document = editor::SceneSerializer::Load(root / "Scenes" / argv[1], &error);
    Check(document.has_value(), error);
    const bool mazeAcceptance = argc >= 4 && std::string(argv[3]) == "maze-acceptance";
    if (mazeAcceptance) {
      if (std::string(argv[1]) != "Microbot.pfscene") {
        const auto original =
          editor::SceneSerializer::Load(root / "Scenes/Microbot.pfscene", &error);
        Check(original.has_value(), error);
        auto expected = original->GetObjects();
        auto actual = document->GetObjects();
        Check(actual.size() == expected.size(), "Difficulty variants keep the same entities");
        for (std::size_t i = 0; i < actual.size(); ++i) {
          if (actual[i].id == 1 || actual[i].id == 8)
            continue;  // Map asset and goal location differ.
          const auto removeGeometry = [](auto& object) {
            std::erase_if(object.components, [](const auto& component) {
              return component.typeId == "project.MicromouseMazeComponent";
            });
          };
          removeGeometry(actual[i]);
          removeGeometry(expected[i]);
          Check(
            actual[i] == expected[i], "Difficulty preserves hardware, sensors, speed and safety");
        }
      }
      Check(editor::SceneSerializer::Save(*document, sandbox / "Maze.pfscene", &error), error);
      auto reopened = editor::SceneSerializer::Load(sandbox / "Maze.pfscene", &error);
      Check(reopened && reopened->GetObjects() == document->GetObjects(),
        "Maze save/reopen preserves hardware, mounts, marker and mission settings");
      document = std::move(reopened);
    }
    // Test against the original deadline, regardless of subsequent Inspector edits.
    if (argc < 3 || std::string(argv[2]) != "authored-limit")
      document->SetComponentProperty(2, "project.RoverSolverComponent", "runTimeout", 600.0);
    Runtime runtime;
    Check(runtime.Load({.projectDirectory = sandbox, .services = &services}, error), error);
    runtime.SynchronizeScene(document->GetObjects());
    runtime.UpdateAuthoringScene();
    runtime.Start();
    if (mazeAcceptance) {
      ui::ProjectUIHost host;
      RoverMissionPanel panel(runtime.world, host);
      const auto view = panel.DescribeView();
      const auto key = "mission-" + std::to_string(runtime.ResolveSceneObject(2).GetEntity());
      const auto* repeat = FindView(view, key + "-return");
      Check(repeat && !repeat->enabled, "Maze cannot return before discovering the marker");
      Check(FindView(view, key + "-solve") && !FindView(view, key + "-solve")->enabled,
        "Solve is disabled before discovery and physical return");
      Check(!runtime.world.simulation.solver.SolveMaze(runtime.ResolveSceneObject(2).GetEntity()),
        "Solver rejects premature Solve calls independently of UI");
      Check(
        !FindView(view, key + "-select"), "Maze exposes arrival controls, not destination picking");
    }
    if (argc >= 6 && std::string(argv[5]) == "corner") {
      bool found = false;
      for (int i = 0; i < 1000 * 60; ++i) {
        runtime.Step();
        const auto entity = runtime.ResolveSceneObject(2).GetEntity();
        const auto* map = runtime.world.simulation.mapping.Find(entity);
        if (map && std::hypot(map->pose.x - 2.25, map->pose.y - 5.39) < .08) {
          found = runtime.world.simulation.solver.FinishExploration(entity);
          break;
        }
      }
      Check(found, "Reached screenshot corner neighbourhood during exploration");
    } else
      RunUntilFinished(runtime);
    const auto* mission = runtime.Mission();
    std::cerr << "Saved scene: " << MissionPhaseText(mission->phase) << " " << mission->failure
              << " time " << mission->current.seconds << " distance " << mission->current.distance
              << "\nOutcome: " << mission->explorationOutcome
              << " | New observed area: " << mission->newArea
              << " | Survey seconds: " << mission->surveySeconds
              << " | Recovery seconds: " << mission->recoverySeconds << '\n';
    if (mission->phase == MissionPhase::Failed) {
      const auto rover = runtime.ResolveSceneObject(2);
      const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
      const auto* safety = rover.GetComponent<RoverSafetyComponent>();
      const auto* drive = rover.GetComponent<RoverDriveComponent>();
      MazeMemory diagnostic;
      const auto grid = BuildMazeGrid(*map,
        diagnostic,
        safety->halfLength,
        safety->halfWidth,
        safety->clearance,
        drive->stopDistance,
        safety->halfLength,
        safety->halfWidth);
      const int cell = grid.At({map->pose.x, map->pose.y});
      std::cerr << "POSE " << map->pose.x << "," << map->pose.y << " heading=" << map->pose.heading
                << " open=" << grid.Open(cell) << " clearance=" << int(grid.clear[cell])
                << " turn=" << int(grid.turnClear[cell]) << " starts=" << int(grid.poseHeadings)
                << '\n';
      for (int y = cell / grid.width - 10; y <= cell / grid.width + 10; ++y) {
        for (int x = cell % grid.width - 10; x <= cell % grid.width + 10; ++x) {
          int i = y * grid.width + x;
          std::cerr << (i == cell                    ? '@'
                        : i < 0 || i >= grid.Count() ? '?'
                        : map->cells[i] > 0          ? '#'
                        : map->cells[i] == 0         ? ' '
                        : grid.Open(i)               ? '.'
                                                     : 'x');
        }
        std::cerr << '\n';
      }
    }
    Check(
      mission->phase == (mission->timed ? MissionPhase::AwaitDestination : MissionPhase::GoalFound),
      "Saved scene completes configured exploration mode within its test budget");
    if (mazeAcceptance) {
      const auto rover = runtime.ResolveSceneObject(2);
      const auto* marker = runtime.ResolveSceneObject(8).GetComponent<Transform2DComponent>();
      const auto* goal = runtime.ResolveSceneObject(8).GetComponent<RoverGoalComponent>();
      const auto* hardware = rover.GetComponent<MicromouseHardwareComponent>();
      Check(hardware && hardware->Valid(), "Authored micromouse hardware loads");
      Check(
        mission->first.success && mission->first.distance > 2, "Maze exploration crosses rooms");
      auto reachedMarker = [&] {
        const auto p = rover.GetComponent<Transform2DComponent>()->position;
        return std::hypot(p.x - marker->position.x, p.y - marker->position.y) <=
               goal->radius + .005;
      };
      Check(reachedMarker(), "First arrival is at the real floor marker");
      const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
      const auto accepted = map->accepted;
      std::cout << "MAZE first: " << mission->first.seconds << " s / " << mission->first.distance
                << " m; arrival heading " << mission->goalHeading << " rad, approach "
                << mission->goalApproachSign << '\n';
      ui::ProjectUIHost host;
      RoverMissionPanel panel(runtime.world, host);
      const auto key = "mission-" + std::to_string(rover.GetEntity());
      auto controls = panel.DescribeView();
      const auto* repeat = FindView(controls, key + "-return");
      Check(repeat && repeat->enabled && repeat->onPressed,
        "Actual Maze panel enables Return to Start after arrival");
      repeat->onPressed();
      Check(runtime.Mission()->phase == MissionPhase::ReturnToStart,
        "Maze panel callback requests physical return");
      Check(!runtime.world.simulation.solver.PlannedPath(rover.GetEntity()).empty(),
        "Return path is available to scene and minimap renderers");
      controls = panel.DescribeView();
      Check(!FindView(controls, key + "-return")->enabled,
        "Return cannot be restarted while it is in progress");
      bool physicallyReturned = false;
      unsigned returnTicks = 0;
      const auto authored = document->GetObjects();
      const auto start = std::find_if(authored.begin(), authored.end(), [](const auto& object) {
        return object.id == 2;
      })->transform;
      RunUntilFinished(runtime, [&] {
        if (!physicallyReturned) ++returnTicks;
        if (!physicallyReturned && runtime.Mission()->phase == MissionPhase::ReadyToSolve) {
          const auto* pose = rover.GetComponent<Transform2DComponent>();
          Check(std::hypot(pose->position.x - start.position.x,
                  pose->position.y - start.position.y) <= .002 &&
                  std::abs(std::remainder(pose->rotation - start.rotation, 360.0)) < 1,
            "Repeat starts at the physical authored start and heading");
          physicallyReturned = true;
          std::cout << "MAZE return: " << returnTicks / 60.0 << " s; physical start "
                    << pose->position.x << "," << pose->position.y << '\n';
        }
      });
      Check(runtime.Mission()->phase == MissionPhase::ReadyToSolve, "Return stops at home");
      const auto home = rover.GetComponent<Transform2DComponent>()->position;
      runtime.Step(120);
      Check(runtime.Mission()->phase == MissionPhase::ReadyToSolve &&
              std::hypot(rover.GetComponent<Transform2DComponent>()->position.x - home.x,
                rover.GetComponent<Transform2DComponent>()->position.y - home.y) < .0001,
        "Returned rover stays stopped until Solve Maze is pressed");
      panel.RefreshView();
      controls = panel.DescribeView();
      const auto* solve = FindView(controls, key + "-solve");
      Check(solve && solve->enabled && solve->onPressed, "Solve unlocks only at home");
      solve->onPressed();
      Check(runtime.Mission()->phase == MissionPhase::Repeat, "Solve starts learned run");
      Check(!runtime.world.simulation.solver.SolveMaze(rover.GetEntity()),
        "Cannot restart an active solve");
      runtime.Step(30);
      const auto* cells = runtime.world.simulation.solver.LearnedCells(rover.GetEntity());
      Check(cells && std::count(cells->visited.begin(), cells->visited.end(), true) > 20,
        "Cell walls and visited centres survive return and solve");
      const auto plan = runtime.world.simulation.solver.PlannedPath(rover.GetEntity());
      Check(plan.size() > 2, "Fresh solve exposes its planned route to scene and minimap");
      for (std::size_t i = 2; i < plan.size(); ++i) {
        const int from = cells->At(plan[i - 1]), to = cells->At(plan[i]);
        bool open = false;
        for (int d = 0; d < 4; ++d)
          open |= cells->Neighbour(from, d) == to && cells->edges[from][d] == CellMaze::Open;
        Check(open, "Solve line crosses only measured open cell boundaries");
      }
      double straightDistance = 0, longestStraight = 0;
      auto previousPosition = rover.GetComponent<Transform2DComponent>()->position;
      RunUntilFinished(runtime, [&] {
        const auto* body = rover.GetComponent<KinematicBody2DComponent>();
        const auto current = rover.GetComponent<Transform2DComponent>()->position;
        if (std::hypot(body->velocity.x, body->velocity.y) > .01) {
          straightDistance +=
            std::hypot(current.x - previousPosition.x, current.y - previousPosition.y);
          longestStraight = std::max(longestStraight, straightDistance);
        } else
          straightDistance = 0;
        previousPosition = current;
      });
      Check(longestStraight > cells->settings.cellPitch * 2,
        "Microbot crosses multiple learned straight cells without stopping");
      std::cout << "MAZE uninterrupted straight: " << longestStraight << " m\n";
      mission = runtime.Mission();
      std::cerr << "MAZE repeat: " << MissionPhaseText(mission->phase) << " " << mission->failure
                << " " << runtime.world.simulation.solver.Status(rover.GetEntity()) << '\n';
      if (!mission->repeat.success) {
        const auto p = rover.GetComponent<Transform2DComponent>()->position;
        std::cerr << "Marker distance "
                  << std::hypot(p.x - marker->position.x, p.y - marker->position.y) << " / radius "
                  << goal->radius << '\n';
      }
      Check(mission->phase == MissionPhase::RepeatComplete && mission->repeat.success,
        "Saved Maze physically returns and completes a learned repeat");
      Check(physicallyReturned, "Maze returned before starting its repeat");
      Check(mission->repeatStartOffset <= .08, "Repeat begins at original start tolerance");
      Check(reachedMarker(), "Learned repeat physically reaches the marker");
      const auto solvedPosition = rover.GetComponent<Transform2DComponent>()->position;
      runtime.Step(120);
      Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{} &&
              rover.GetComponent<Transform2DComponent>()->position == solvedPosition,
        "Microbot remains stopped at the yellow destination after the continuous solve");
      Check(mission->repeat.distance < mission->first.distance * .8 &&
              mission->repeat.seconds < mission->first.seconds * .8,
        "New learned-map solve removes discovery detours rather than replaying the first run");
      Check(runtime.world.simulation.mapping.Find(rover.GetEntity())->accepted >= accepted,
        "Return/repeat keeps the learned map");
      std::cout << "MAZE repeat: " << mission->repeat.seconds << " s / " << mission->repeat.distance
                << " m; start offset " << mission->repeatStartOffset << " m\n";
      runtime.Reset();
      Check(!runtime.Mission(), "Maze reset clears learned mission");
      runtime.Start();
      runtime.Step(30);
      Check(runtime.Mission() && !runtime.Mission()->learnedGoal, "Maze reset starts unknown");
      return 0;
    }
    if (argc >= 5) {
      const auto rover = runtime.ResolveSceneObject(2);
      const MazePoint destination{std::stod(argv[3]), std::stod(argv[4])};
      auto& solver = runtime.world.simulation.solver;
      const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
      MazeMemory memory;
      const auto grid = BuildMazeGrid(*map, memory, .16, .10, .03, 0, .15, .10, false, true);
      const int here = grid.At({map->pose.x, map->pose.y});
      std::cerr << "Navigation start " << map->pose.x << "," << map->pose.y
                << " heading=" << map->pose.heading << " open=" << grid.Open(here)
                << " clear=" << int(grid.clear[here]) << " turn=" << int(grid.turnClear[here])
                << "\n";
      Check(
        solver.SelectDestination(rover.GetEntity(), destination), "Accept diagnostic destination");
      RunUntilFinished(runtime);
      std::cerr << "Navigation result: " << MissionPhaseText(runtime.Mission()->phase) << " "
                << runtime.Mission()->selectionMessage << " " << runtime.Mission()->failure << "\n";
      Check(
        runtime.Mission()->phase == MissionPhase::Arrived, "Reach diagnostic scene destination");
    }
    return 0;
  }

  CheckContinuousStraight(false);
  CheckContinuousStraight(true);
  CheckMissionPanelSetup();
  CheckMeasuredRouteSafety();
  for (const char* algorithm : {"AStar", "BFS", "DFS", "Dijkstra", "FloodFill", "RightHand"}) {
    CheckLidarStartExit(algorithm);
    if (std::string(algorithm) != "AStar") CheckLidarStartExit(algorithm, true);
  }
  CheckLidarGapSafety(false);
  CheckLidarGapSafety(true);
  CheckCornerDeparture();
  for (bool laser : {false, true})
    CheckExplorationOutcomes(laser);
  for (double duration : {300.0, 600.0})
    CheckExplorationDeadline(duration);
  CheckRetryPolicy();
  for (bool laser : {false, true})
    for (double duration : {300.0, 600.0})
      CheckTimedWorkflow(laser, duration);

  CheckFloorSearch();

  for (bool laser : {true, false}) {
    Runtime runtime;
    Load(runtime, laser);
    runtime.Step(30);
    Check(runtime.Mission() && !runtime.Mission()->learnedGoal,
      "Unseen goal coordinates must not enter solver state");

    RunUntilFinished(runtime);
    const auto* mission = runtime.Mission();
    std::cerr << (laser ? "LiDAR" : "Ultrasonic") << " first: " << MissionPhaseText(mission->phase)
              << " " << mission->failure << '\n';
    Check(mission->phase == MissionPhase::GoalFound, "Explore and detect floor marker");
    std::cout << (laser ? "LiDAR" : "Ultrasonic") << " first " << mission->first.seconds << " s, "
              << mission->first.distance << " m\n";
    Check(mission->first.success && mission->first.distance > .5, "Record first arrival metrics");
    const auto rover = runtime.ResolveSceneObject(2);
    Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{}, "Stop at goal");
    Check(runtime.world.simulation.solver.ReturnAndRepeat(rover.GetEntity()), "Request repeat");

    RunUntilFinished(runtime);
    mission = runtime.Mission();
    std::cerr << "repeat: " << MissionPhaseText(mission->phase) << " " << mission->failure << '\n';
    Check(mission->phase == MissionPhase::RepeatComplete, "Return and complete learned repeat");
    Check(mission->repeatCount == 1 && mission->repeat.success && mission->repeat.distance > .5,
      "Repeat result excludes the return trip");

    std::cout << (laser ? "LiDAR" : "Ultrasonic") << " repeat " << mission->repeat.seconds << " s, "
              << mission->repeat.distance << " m\n";

    runtime.Reset();
    Check(!runtime.Mission(), "Reset clears learned mission");
    runtime.Start();
    runtime.Step(30);
    Check(runtime.Mission() && !runtime.Mission()->learnedGoal, "Restart begins unknown");
  }

  // Seeing free room floor is not the same as searching it for an arrival marker.
  for (bool laser : {true, false}) {
    Runtime runtime;
    std::string error;
    Check(runtime.Load({}, error), error);
    auto objects = Fixture(runtime, laser, {1.1f, .8f});
    std::erase_if(objects, [](const auto& object) { return object.id >= 20 && object.id <= 23; });
    objects.push_back(Wall(runtime, 20, {.8f, -1.5f}, {4, .1f}));
    objects.push_back(Wall(runtime, 21, {.8f, 1.5f}, {4, .1f}));
    objects.push_back(Wall(runtime, 22, {-1.2f, 0}, {.1f, 3.1f}));
    objects.push_back(Wall(runtime, 23, {2.8f, 0}, {.1f, 3.1f}));
    runtime.SynchronizeScene(objects);
    runtime.UpdateAuthoringScene();
    runtime.Start();
    RunUntilFinished(runtime);
    const auto* mission = runtime.Mission();
    std::cerr << (laser ? "LiDAR" : "Ultrasonic") << " room " << MissionPhaseText(mission->phase)
              << " " << mission->failure << '\n';
    Check(mission->phase == MissionPhase::GoalFound, "Search room interior before mission timeout");
  }

  // Moving an unseen marker must not change commands, geometry or estimated motion.
  {
    Runtime first, second;
    Load(first, true, {2.0f, 0});
    Load(second, true, {2.0f, .4f});
    for (int i = 0; i < 600; ++i) {
      first.Step();
      second.Step();
      const auto* a = first.ResolveSceneObject(2).GetComponent<Transform2DComponent>();
      const auto* b = second.ResolveSceneObject(2).GetComponent<Transform2DComponent>();
      Check(a->position == b->position && a->rotation == b->rotation,
        "Unseen goal transforms cannot steer exploration");
    }
    Check(!first.Mission()->learnedGoal && !second.Mission()->learnedGoal,
      "Goal stays unknown until detected");
  }

  // Learned coordinates are not proof of arrival: the detector must confirm again.
  {
    Runtime runtime;
    Load(runtime, true);
    RunUntilFinished(runtime);
    Check(runtime.Mission()->phase == MissionPhase::GoalFound, "Discover confirmation fixture");

    runtime.ResolveSceneObject(30).GetComponent<RoverGoalComponent>()->enabled = false;
    const auto rover = runtime.ResolveSceneObject(2);
    Check(runtime.world.simulation.solver.ReturnAndRepeat(rover.GetEntity()),
      "Request confirmation run");
    RunUntilFinished(runtime);

    Check(runtime.Mission()->phase == MissionPhase::Failed &&
            runtime.Mission()->failure.starts_with("Learned goal could not be confirmed") &&
            runtime.Mission()->repeatCount == 0,
      "No repeat success without a fresh marker detection");
    Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{},
      "Failed confirmation stops motion");
  }

  // A disabled/wrong-tag marker at the start must not count as a destination.
  {
    Runtime runtime;
    Load(runtime, true, {0, 0});
    const auto marker = runtime.ResolveSceneObject(30);
    marker.GetComponent<RoverGoalComponent>()->tag = "Other";
    runtime.Step(30);
    Check(runtime.Mission()->phase == MissionPhase::Explore, "Ignore wrong goal tag");
    marker.GetComponent<RoverGoalComponent>()->tag = "Finish";
    marker.GetComponent<RoverGoalComponent>()->enabled = false;
    runtime.Step(30);
    Check(runtime.Mission()->phase == MissionPhase::Explore, "Ignore disabled marker");

    const auto rover = runtime.ResolveSceneObject(2);
    rover.GetComponent<RoverSolverComponent>()->runTimeout = 10;
    runtime.ResolveSceneObject(8).GetComponent<LidarSensorComponent>()->enabled = false;
    runtime.Step(60);
    Check(rover.GetComponent<KinematicBody2DComponent>()->velocity == Vector2f{},
      "Missing live sensing stops mission motion");
    runtime.Step(600);
    Check(runtime.Mission()->phase == MissionPhase::Failed &&
            runtime.Mission()->failure.starts_with("Mission time limit"),
      "Missing sensing terminates with timeout, not false goal success");
    Check(!runtime.world.simulation.solver.ReturnAndRepeat(rover.GetEntity()),
      "Cannot request learned repeat without a discovered goal");
  }

  // A branch off a corridor requires exploration to choose unseen side space.
  for (bool laser : {true, false}) {
    Runtime runtime;
    std::string error;
    Check(runtime.Load({}, error), error);
    auto objects = Fixture(runtime, laser, {1.5f, 1.1f});
    std::erase_if(objects, [](const auto& object) { return object.id == 21; });
    objects.push_back(Wall(runtime, 21, {0, .55f}, {2, .1f}));
    objects.push_back(Wall(runtime, 24, {2.5f, .55f}, {1, .1f}));
    objects.push_back(Wall(runtime, 25, {.95f, 1.1f}, {.1f, 1.2f}));
    objects.push_back(Wall(runtime, 26, {2.05f, 1.1f}, {.1f, 1.2f}));
    objects.push_back(Wall(runtime, 27, {1.5f, 1.65f}, {1.2f, .1f}));
    runtime.SynchronizeScene(objects);
    runtime.UpdateAuthoringScene();
    runtime.Start();
    RunUntilFinished(runtime);
    const auto* mission = runtime.Mission();
    std::cerr << (laser ? "LiDAR" : "Ultrasonic") << " branch " << MissionPhaseText(mission->phase)
              << " " << mission->failure << '\n';
    if (mission->phase != MissionPhase::GoalFound) {
      const auto rover = runtime.ResolveSceneObject(2);
      const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
      MazeMemory memory;
      const auto grid = BuildMazeGrid(*map, memory, .16, .10, .03, .35, .15, .10);
      const int here = grid.At({map->pose.x, map->pose.y});
      std::cerr << "Branch map at rover: evidence=" << int(map->cells[here])
                << " open=" << grid.Open(here) << " clearance=" << int(grid.clear[here])
                << " heading=" << map->pose.heading << "\n";
      for (double y = .2; y <= 1.3; y += .1) {
        for (double x = 1.15; x <= 1.85; x += .05) {
          const int cell = grid.At({x, y});
          std::cerr << (grid.evidence[cell] > 0  ? '#'
                        : !grid.Open(cell)       ? 'X'
                        : grid.Next(cell, 1) < 0 ? '-'
                                                 : '+');
        }
        std::cerr << " y=" << y << '\n';
      }
    }
    Check(mission->phase == MissionPhase::GoalFound, "Find marker in side branch");
    const auto rover = runtime.ResolveSceneObject(2);
    Check(runtime.world.simulation.solver.ReturnAndRepeat(rover.GetEntity()),
      "Return from discovered branch");
    RunUntilFinished(runtime);
    mission = runtime.Mission();
    std::cerr << (laser ? "LiDAR" : "Ultrasonic") << " branch repeat "
              << MissionPhaseText(mission->phase) << " " << mission->failure << '\n';
    if (mission->phase != MissionPhase::RepeatComplete) {
      const auto* pose = rover.GetComponent<Transform2DComponent>();
      std::cerr << "Failed pose " << pose->position.x << "," << pose->position.y << " heading "
                << pose->rotation << " repeat seconds " << mission->repeat.seconds << " trail size "
                << mission->trail.size() << '\n';
      const auto* map = runtime.world.simulation.mapping.Find(rover.GetEntity());
      MazeMemory memory;
      const auto grid = BuildMazeGrid(*map, memory, .16, .10, .03, 0, .15, .10, false);
      const int here = grid.At({map->pose.x, map->pose.y});
      std::cerr << "return cell " << int(map->cells[here]) << " open " << grid.Open(here)
                << " clearance " << int(grid.clear[here]) << '\n';
      for (const auto point : mission->trail)
        std::cerr << "Recorded point " << point.x << ',' << point.y << '\n';
      for (const auto point : runtime.world.simulation.solver.PlannedPath(rover.GetEntity()))
        std::cerr << "Remaining point " << point.x << ',' << point.y << '\n';
    }
    Check(mission->phase == MissionPhase::RepeatComplete, "Repeat learned side-branch route");
  }

  std::cout << "RL8 mission checks passed\n";
}

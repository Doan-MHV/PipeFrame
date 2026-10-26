#include "Benchmark.h"

#include <PipeFrame/Project/AssetDatabase.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <set>
#include <sstream>
#include <stdexcept>

#include "Integration/RoverLabAdapter.h"
#include "SceneSerializer.h"

namespace RoverLab::benchmark {
namespace {
using namespace pipeframe;
namespace fs = std::filesystem;
constexpr float Step = 1.f / 60;

void Require(bool condition, const std::string& reason) {
  if (!condition) throw std::runtime_error(reason);
}

bool Token(const std::string& text) {
  return !text.empty() && text.size() <= 80 &&
         text.find_first_not_of(
           "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") == std::string::npos;
}

bool IsMicrobot(const std::string& scene) {
  return scene == "Microbot" || scene == "MicrobotMedium" || scene == "MicrobotHard";
}

void Validate(const Case& c) {
  Require(Token(c.name), "Case name must be a short alphanumeric/hyphen/underscore token");
  Require(c.scene == "Ultrasonic" || c.scene == "LiDAR" || IsMicrobot(c.scene),
    "Scene must be Ultrasonic, LiDAR, Microbot, MicrobotMedium or MicrobotHard");
  const std::set<std::string> algorithms{
    "BFS", "DFS", "Dijkstra", "AStar", "FloodFill", "RightHand"};
  Require(IsMicrobot(c.scene) ? c.algorithm == "CellFloodFill" : algorithms.contains(c.algorithm),
    "Microbot uses CellFloodFill; room scenes use one of the six generic algorithms");
  Require(c.seed >= 0 && c.repeats >= 1 && c.repeats <= 100, "Invalid seed or repetition count");
  Require(
    std::isfinite(c.limit) && c.limit >= 1 && c.limit <= 3600, "Leg limit must be 1..3600 seconds");
  Require(std::isfinite(c.explore) && c.explore >= 0 && c.explore <= c.limit,
    "Exploration budget must be 0 (until complete) or no greater than the leg limit");
  Require(
    std::isfinite(c.destinationX) && std::isfinite(c.destinationY), "Destination must be finite");
  Require(c.workflow == "Explore" || c.workflow == "Destination" || c.workflow == "Maze",
    "Workflow must be Explore, Destination or Maze");
  Require((IsMicrobot(c.scene)) == (c.workflow == "Maze"), "Microbot requires the Maze workflow");
  Require(std::isfinite(c.noise) && (c.noise == -1 || (c.noise >= 0 && c.noise <= .05)),
    "Noise must be -1 (authored) or 0..0.05 metres");
  Require(std::isfinite(c.dropout) && (c.dropout == -1 || (c.dropout >= 0 && c.dropout <= 1)),
    "Dropout must be -1 (authored) or 0..1");
  Require(!IsMicrobot(c.scene) || c.dropout <= 0, "Infrared does not implement dropout");
}

struct Hash {
  std::uint64_t value{14695981039346656037ull};
  void Add(const std::string& text) {
    for (unsigned char c : text)
      value = (value ^ c) * 1099511628211ull;
  }
  std::string Text() const {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
  }
};

std::string Number(double value) {
  Require(std::isfinite(value), "Non-finite metric");
  std::ostringstream out;
  out << std::setprecision(17) << value;
  return out.str();
}

class Contacts final : public Behaviour {
public:
  unsigned entries{};
  void OnCollisionEnter(const CollisionEvent2D&) override { ++entries; }
};

bool Has(const SceneObjectData& object, const std::string& type) {
  return std::any_of(object.components.begin(),
    object.components.end(),
    [&](const auto& component) { return component.typeId == type; });
}

SceneObjectId Configure(editor::SceneDocument& doc, const Case& c) {
  SceneObjectId rover{};
  const auto objects = doc.GetObjects();
  for (const auto& object : objects) {
    if (!Has(object, "project.RoverSolverComponent")) continue;
    Require(rover == 0, "Benchmark requires exactly one Rover Solver");
    rover = object.id;
    doc.SetComponentProperty(rover,
      "project.RoverSolverComponent",
      "algorithm",
      IsMicrobot(c.scene) ? std::string("AStar") : c.algorithm);
    doc.SetComponentProperty(rover,
      "project.RoverSolverComponent",
      "missionMode",
      std::string(c.workflow == "Maze" ? "ArrivalMarker" : "TimedExplore"));
    doc.SetComponentProperty(
      rover, "project.RoverSolverComponent", "explorationDuration", std::string("Until complete"));
    doc.SetComponentProperty(rover, "project.RoverSolverComponent", "runTimeout", 3600.0);
    doc.SetComponentProperty(rover, "project.RoverSolverComponent", "enabled", true);
    // Independent runner tick limits apply even when the solver is waiting for sensors.
    if (Has(object, "project.RoverNavigationComponent"))
      doc.SetComponentProperty(rover, "project.RoverNavigationComponent", "enabled", false);
  }
  Require(rover != 0, "Scene has no Rover Solver");

  for (const auto& object : objects) {
    for (const std::string type : {"project.UltrasonicScheduleComponent",
           "project.LidarSensorComponent",
           "project.InfraredSensorComponent"}) {
      if (!Has(object, type)) continue;
      doc.SetComponentProperty(object.id, type, "randomSeed", std::int64_t(c.seed));
      if (c.noise >= 0) doc.SetComponentProperty(object.id, type, "noiseAmplitude", c.noise);
      if (c.dropout >= 0 && type != "project.InfraredSensorComponent")
        doc.SetComponentProperty(object.id, type, "dropoutProbability", c.dropout);
    }
  }
  return rover;
}

void CopyProject(const fs::path& from, const fs::path& to) {
  Require(fs::create_directory(to), "Output workspace already exists: " + to.string());
  fs::copy(from / "Assets", to / "Assets", fs::copy_options::recursive);
  fs::create_directory(to / ".pipeframe");
  fs::copy_file(from / ".pipeframe/assets.db", to / ".pipeframe/assets.db");
  fs::copy(from / "Scenes", to / "Scenes", fs::copy_options::recursive);
}

void Write(const fs::path& path, const std::string& text) {
  std::ofstream file(path);
  file << text;
  file.close();
  Require(bool(file), "Cannot write " + path.string());
}

std::string Fingerprints(const fs::path& root) {
  std::vector<fs::path> files;
  for (const auto& entry : fs::recursive_directory_iterator(root))
    if (entry.is_regular_file()) files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  std::ostringstream result;
  for (const auto& path : files) {
    std::ifstream file(path, std::ios::binary);
    Require(bool(file), "Cannot fingerprint " + path.string());
    Hash hash;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
      hash.Add(std::string(buffer, std::size_t(file.gcount())));
    Require(!file.bad(), "Fingerprint read failed");
    result << hash.Text() << ' ' << fs::relative(path, root).generic_string() << '\n';
  }
  return result.str();
}
}  // namespace

std::string Quote(const std::string& value) {
  std::ostringstream out;
  out << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\')
      out << '\\' << c;
    else if (c < 32)
      out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
    else
      out << c;
  }
  out << '"';
  return out.str();
}

std::vector<Case> ReadPlan(std::istream& input) {
  std::vector<Case> plan;
  std::set<std::string> names;
  std::string line;
  unsigned number = 0;
  bool header = false;
  while (std::getline(input, line)) {
    ++number;
    if (const auto comment = line.find('#'); comment != std::string::npos) line.resize(comment);
    std::istringstream row(line);
    row >> std::ws;
    if (row.eof()) continue;
    if (!header) {
      std::string word, extra;
      int version{};
      Require(bool(row >> word >> version) && word == "ROVERLAB_BENCHMARK" && version == 1 &&
                !(row >> extra),
        "Expected ROVERLAB_BENCHMARK 1 header");
      header = true;
      continue;
    }
    Case c;
    std::string extra;
    Require(
      bool(row >> c.name >> c.scene >> c.algorithm >> c.seed >> c.repeats >> c.workflow >>
           c.limit >> c.explore >> c.destinationX >> c.destinationY >> c.noise >> c.dropout) &&
        !(row >> extra),
      "Invalid benchmark row at line " + std::to_string(number));
    Validate(c);
    Require(names.insert(c.name).second, "Duplicate case name: " + c.name);
    plan.push_back(c);
    Require(plan.size() <= 1000, "Plan exceeds 1000 cases");
  }
  Require(header && !plan.empty(), "Empty benchmark plan");
  return plan;
}

std::string Result::Json() const {
  const auto& c = scenario;
  std::ostringstream out;
  out << std::setprecision(17) << "{\"schema\":1,\"case\":" << Quote(c.name)
      << ",\"scene\":" << Quote(c.scene) << ",\"algorithm\":" << Quote(c.algorithm)
      << ",\"exploration_planner\":" << Quote(explorationPlanner)
      << ",\"route_planner\":" << Quote(routePlanner) << ",\"seed\":" << c.seed
      << ",\"repetition\":" << repetition << ",\"workflow\":" << Quote(c.workflow)
      << ",\"limit_seconds\":" << c.limit << ",\"exploration_budget\":" << c.explore
      << ",\"destination_world\":[" << c.destinationX << ',' << c.destinationY << ']'
      << ",\"noise_override\":" << c.noise << ",\"dropout_override\":" << c.dropout
      << ",\"outcome\":" << Quote(outcome) << ",\"reason\":" << Quote(reason)
      << ",\"final_phase\":" << Quote(finalPhase) << ",\"solver_status\":" << Quote(solverStatus)
      << ",\"safety_reason\":" << Quote(safetyReason)
      << ",\"exploration_outcome\":" << Quote(explorationOutcome) << ",\"start_world\":[" << startX
      << ',' << startY << ',' << startDegrees << ']' << ",\"fixed_step_seconds\":" << double(Step)
      << ",\"wall_seconds\":" << wallSeconds << ",\"observed_area_m2\":" << observedArea
      << ",\"trace_hash\":" << Quote(traceHash) << ",\"map_hash\":" << Quote(mapHash)
      << ",\"legs\":[";
  bool first = true;
  for (const auto& leg : legs) {
    if (!first) out << ',';
    first = false;
    out << "{\"name\":" << Quote(leg.name) << ",\"outcome\":" << Quote(leg.outcome)
        << ",\"reason\":" << Quote(leg.reason) << ",\"seconds\":" << leg.seconds
        << ",\"distance_m\":" << leg.distance << ",\"rotation_rad\":" << leg.rotation
        << ",\"turn_episodes\":" << leg.turns << ",\"direction_reversals\":" << leg.reversals
        << ",\"collision_entries\":" << leg.collisions << '}';
  }
  out << "]}";
  return out.str();
}

bool Result::SameSimulation(const Result& other) const {
  auto a = *this, b = other;
  a.wallSeconds = b.wallSeconds = 0;
  a.repetition = b.repetition = 0;
  return a.Json() == b.Json();
}

Result RunCase(const Case& c,
  unsigned repetition,
  const fs::path& snapshot,
  const fs::path& workspace,
  const fs::path& resolvedScene,
  const std::function<bool()>& cancelled) {
  Result result;
  result.scenario = c;
  result.repetition = repetition;
  result.explorationPlanner = IsMicrobot(c.scene) ? "CellFloodFill" : c.algorithm;
  result.routePlanner = "TimeCostAStar";
  const auto started = std::chrono::steady_clock::now();
  try {
    Validate(c);
    if (cancelled()) {
      result.outcome = "cancelled";
      result.reason = "Batch cancelled before run";
      return result;
    }
    Require(!fs::exists(workspace), "Run workspace already exists");
    struct Cleanup {
      fs::path path;
      ~Cleanup() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
      }
    } cleanup{workspace};
    CopyProject(snapshot, workspace);

    std::string error;
    auto document =
      editor::SceneSerializer::Load(workspace / "Scenes" / (c.scene + ".pfscene"), &error);
    Require(document.has_value(), error);
    const auto id = Configure(*document, c);
    Require(editor::SceneSerializer::Save(*document, resolvedScene, &error), error);
    const auto start = document->FindObject(id)->transform;
    result.startX = start.position.x;
    result.startY = start.position.y;
    result.startDegrees = start.rotation;

    assets::AssetDatabase database;
    Require(database.Open(workspace, &error), error);
    ServiceRegistry services;
    services.Provide(database);
    RoverLabAdapter runtime;
    Require(runtime.Load({.projectDirectory = workspace, .services = &services}, error), error);
    runtime.SynchronizeScene(document->GetObjects());
    runtime.UpdateAuthoringScene();
    auto rover = runtime.ResolveSceneObject(id);
    Require(rover.IsValid(), "Benchmark rover did not load");
    auto& world = runtime.GetProjectWorld();
    auto& solver = world.simulation.solver;
    const auto entity = rover.GetEntity();
    Require(!solver.Mission(entity) && !world.simulation.mapping.Find(entity) &&
              !world.simulation.odometry.Find(entity),
      "Fresh run unexpectedly retained learning");
    const auto* cell = rover.GetComponent<MicromouseMazeComponent>();
    Require(c.workflow != "Maze" || (cell && cell->enabled), "Microbot requires enabled cell maze");
    Require(
      c.workflow == "Maze" || !cell || !cell->enabled, "Room algorithm overridden by cell maze");
    auto& contacts = rover.Attach<Contacts>();
    runtime.Start();
    Hash trace;

    auto runLeg = [&](const std::string& name, MissionPhase success, double budget) {
      Leg leg;
      leg.name = name;
      const auto beforeContacts = contacts.entries;
      auto previous = *rover.GetComponent<Transform2DComponent>();
      bool turning = false;
      int lastDirection = 0;
      const auto ticks = std::uint64_t(std::ceil(budget * 60));
      for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        if (cancelled()) {
          leg.outcome = "cancelled";
          leg.reason = "Batch cancelled";
          break;
        }
        runtime.FixedUpdate(Step);
        const auto current = *rover.GetComponent<Transform2DComponent>();
        const double dx = current.position.x - previous.position.x;
        const double dy = current.position.y - previous.position.y;
        const double angle =
          std::remainder(double(current.rotation - previous.rotation), 2 * std::numbers::pi);
        leg.seconds = (tick + 1) / 60.0;
        leg.distance += std::hypot(dx, dy);
        leg.rotation += std::abs(angle);
        const bool nowTurning = std::abs(angle) > .01 / 60;
        if (nowTurning && !turning) ++leg.turns;
        turning = nowTurning;
        const double forward = dx * std::cos(previous.rotation) + dy * std::sin(previous.rotation);
        if (std::abs(forward) > .001 / 60) {
          const int direction = forward < 0 ? -1 : 1;
          if (lastDirection && lastDirection != direction) ++leg.reversals;
          lastDirection = direction;
        }
        previous = current;
        const auto* mission = solver.Mission(entity);
        trace.Add(Number(current.position.x) + "," + Number(current.position.y) + "," +
                  Number(current.rotation) + ":" +
                  std::to_string(mission ? int(mission->phase) : -1) + ";");
        if (!mission) continue;
        if (mission->phase == success) {
          leg.outcome = "success";
          leg.reason = MissionPhaseText(success);
          if (name == "explore") {
            leg.reason = mission->explorationOutcome;
            leg.outcome =
              mission->explorationOutcome.starts_with("Complete:") ? "complete" : "blocked";
          }
          break;
        }
        if (mission->phase == MissionPhase::Failed ||
            mission->phase == MissionPhase::NavigationBlocked) {
          leg.outcome = "failed";
          leg.reason = mission->failure.empty() ? solver.Status(entity) : mission->failure;
          break;
        }
        if (mission->Finished()) {
          leg.outcome = "failed";
          leg.reason =
            std::string("Unexpected terminal phase: ") + MissionPhaseText(mission->phase);
          break;
        }
      }
      if (leg.outcome.empty()) {
        if (name == "explore" && c.explore > 0) {
          Require(solver.FinishExploration(entity), "Could not finish exploration budget");
          leg.outcome = "budget_complete";
          leg.reason = "Exploration budget exhausted; map completeness is not implied";
        } else {
          leg.outcome = "timeout";
          leg.reason = "Fixed simulation tick limit reached; " + solver.Status(entity);
        }
      }
      leg.collisions = contacts.entries - beforeContacts;
      result.legs.push_back(leg);
      result.outcome = leg.outcome;
      result.reason = leg.reason;
      return leg.outcome == "success" || leg.outcome == "complete" ||
             leg.outcome == "budget_complete";
    };

    if (c.workflow == "Maze") {
      if (runLeg("discover", MissionPhase::GoalFound, c.limit)) {
        Require(solver.ReturnToStart(entity), "Return request refused");
        if (runLeg("return", MissionPhase::ReadyToSolve, c.limit)) {
          Require(solver.SolveMaze(entity), "Solve request refused");
          runLeg("solve", MissionPhase::RepeatComplete, c.limit);
        }
      }
    } else {
      const bool explored =
        runLeg("explore", MissionPhase::AwaitDestination, c.explore > 0 ? c.explore : c.limit);
      if (explored && c.workflow == "Destination") {
        // Plan destinations are world coordinates, matching a scene click. The
        // benchmark never supplies ground-truth walls or a goal during exploration.
        const double radians = start.rotation * std::numbers::pi / 180;
        const double dx = c.destinationX - start.position.x, dy = c.destinationY - start.position.y;
        const MazePoint point{std::cos(radians) * dx + std::sin(radians) * dy,
          -std::sin(radians) * dx + std::cos(radians) * dy};
        if (solver.SelectDestination(entity, point))
          runLeg("destination", MissionPhase::Arrived, c.limit);
        else {
          result.outcome = "destination_rejected";
          result.reason = solver.Mission(entity)->selectionMessage;
          result.legs.push_back({"destination", result.outcome, result.reason});
        }
      }
    }

    trace.Add(result.outcome);
    result.solverStatus = solver.Status(entity);
    result.safetyReason = world.simulation.drive.StopReason(entity);
    if (const auto* mission = solver.Mission(entity)) {
      result.finalPhase = MissionPhaseText(mission->phase);
      result.explorationOutcome = mission->explorationOutcome;
    }
    result.traceHash = trace.Text();
    Hash mapHash;
    if (const auto* map = world.simulation.mapping.Find(entity)) {
      mapHash.Add(std::to_string(map->width) + ":" + std::to_string(map->height) + ":" +
                  Number(map->originX) + ":" + Number(map->originY) + ":" +
                  Number(map->settings.cellSize));
      for (auto cellValue : map->cells) {
        mapHash.Add(std::string(1, char(cellValue)));
        if (cellValue) result.observedArea += map->settings.cellSize * map->settings.cellSize;
      }
    }
    result.mapHash = mapHash.Text();
    runtime.Reset();
    const auto resetRover = runtime.ResolveSceneObject(id);
    Require(resetRover.IsValid(), "Reset removed authored rover");
    const auto* resetPose = resetRover.GetComponent<Transform2DComponent>();
    Require(resetPose && resetPose->position == start.position &&
              !solver.Mission(resetRover.GetEntity()) &&
              !world.simulation.mapping.Find(resetRover.GetEntity()),
      "Reset did not restore the authored start and clear learning");
  } catch (const std::exception& error) {
    result.outcome = "error";
    result.reason = error.what();
  }
  result.wallSeconds =
    std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
  return result;
}

int RunBatch(const fs::path& project,
  const fs::path& planPath,
  const fs::path& output,
  const std::function<bool()>& cancelled) {
  std::ifstream input(planPath);
  Require(bool(input), "Cannot read benchmark plan");
  const auto plan = ReadPlan(input);
  Require(
    !fs::exists(output), "Choose a new output directory; existing results are never overwritten");
  fs::create_directories(output);
  fs::copy_file(planPath, output / "plan.pfbench");
  CopyProject(project, output / "snapshot");
  Write(output / "input-fingerprints.txt", Fingerprints(output / "snapshot"));
  // Keep project source alongside inputs for audit; engine/build identity is recorded separately.
  fs::copy(project / "Source", output / "source", fs::copy_options::recursive);
  Write(output / "source-fingerprints.txt", Fingerprints(output / "source"));
  Write(output / "build.txt",
    std::string("RoverLab benchmark schema 1\nBuilt ") + __DATE__ + " " + __TIME__ + "\nCompiler " +
      __VERSION__ + "\nFixed update: float(1/60) seconds\n");
  fs::create_directory(output / "inputs");
  std::ofstream journal(output / "results.jsonl");
  Require(bool(journal), "Cannot open results journal");
  auto emit = [&](const std::string& value) {
    journal << value << '\n';
    journal.flush();
    Require(bool(journal), "Cannot persist benchmark result");
  };
  unsigned failures = 0, runs = 0, mismatches = 0;
  for (const auto& c : plan) {
    std::optional<Result> first;
    for (unsigned repetition = 1; repetition <= unsigned(c.repeats); ++repetition) {
      ++runs;
      const std::string key = c.name + "-" + std::to_string(repetition);
      emit("{\"event\":\"started\",\"run\":" + Quote(key) + "}");
      std::cout << "[" << key << "] " << c.scene << ' ' << c.algorithm << " seed=" << c.seed
                << std::endl;
      const auto result = RunCase(c,
        repetition,
        output / "snapshot",
        output / "working",
        output / "inputs" / (key + ".pfscene"),
        cancelled);
      emit("{\"event\":\"finished\",\"run\":" + Quote(key) + ",\"result\":" + result.Json() + "}");
      const bool okay = result.outcome == "success" || result.outcome == "complete" ||
                        result.outcome == "budget_complete";
      if (!okay) ++failures;
      if (first && result.outcome != "cancelled" && first->outcome != "cancelled") {
        const bool same = first->SameSimulation(result);
        if (!same) ++mismatches;
        emit("{\"event\":\"repeatability\",\"run\":" + Quote(key) +
             ",\"matches_first\":" + (same ? "true}" : "false}"));
      }
      if (!first) first = result;
      std::cout << "  " << result.outcome << ": " << result.reason << std::endl;
    }
  }
  emit("{\"event\":\"summary\",\"runs\":" + std::to_string(runs) +
       ",\"unsuccessful\":" + std::to_string(failures) +
       ",\"repeatability_mismatches\":" + std::to_string(mismatches) + "}");
  return failures || mismatches ? 1 : 0;
}
}  // namespace RoverLab::benchmark

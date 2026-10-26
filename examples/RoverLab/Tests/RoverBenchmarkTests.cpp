#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Benchmark.h"

using namespace RoverLab::benchmark;
namespace fs = std::filesystem;
namespace {
void Check(bool value, const std::string& reason) {
  if (!value) throw std::runtime_error(reason);
}

std::string Read(const fs::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void Invalid(const std::string& row) {
  std::istringstream text("ROVERLAB_BENCHMARK 1\n" + row);
  bool rejected = false;
  try {
    ReadPlan(text);
  } catch (const std::exception&) {
    rejected = true;
  }
  Check(rejected, "Malformed or misleading plan must be rejected");
}
}  // namespace

int main() {
  for (const auto* level : {"MicrobotMedium", "MicrobotHard"}) {
    std::istringstream plan(std::string("ROVERLAB_BENCHMARK 1\nx ") + level +
                            " CellFloodFill 42 1 Maze 1800 0 0 0 0 0\n");
    Check(ReadPlan(plan).size() == 1, "Difficulty maze is accepted by the benchmark");
    Invalid(std::string("x ") + level + " AStar 42 1 Maze 1800 0 0 0 0 0");
    Invalid(std::string("x ") + level + " CellFloodFill 42 1 Maze 1800 0 0 0 0 .5");
  }
  Invalid("x MicrobotImpossible CellFloodFill 42 1 Maze 1800 0 0 0 0 0");
  Invalid("x Microbot AStar 42 2 Maze 1800 0 0 0 0 0");
  Invalid("x LiDAR AStar -1 2 Explore 30 30 0 0 0 0");
  Invalid("x LiDAR AStar 42 2 Explore 30 60 0 0 0 0");
  Invalid("x Microbot CellFloodFill 42 2 Maze 1800 0 0 0 0 .5");
  Invalid("x LiDAR AStar 42 2 Explore 30 30 0 0 0 0 extra");
  Check(Quote("line\n\"\\") == "\"line\\u000a\\\"\\\\\"", "Journal escaping");

  const fs::path root = ROVER_PROJECT_ROOT;
  const auto output =
    fs::temp_directory_path() /
    ("rover-benchmark-test-" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directory(output);
  struct Cleanup {
    fs::path path;
    ~Cleanup() {
      std::error_code ignored;
      fs::remove_all(path, ignored);
    }
  } cleanup{output};
  const auto original = Read(root / "Scenes/LiDAR.pfscene");
  unsigned index = 0;
  auto run = [&](
               const Case& c, const std::function<bool()>& stop = [] { return false; }) {
    ++index;
    return RunCase(
      c, index, root, output / "work", output / (std::to_string(index) + ".pfscene"), stop);
  };

  for (const std::string scene : {"Ultrasonic", "LiDAR"}) {
    Case c{scene, scene, "AStar", "Explore", 42, 2, 30, 30, 0, 0, .01, 0};
    const auto first = run(c), second = run(c);
    Check(first.outcome == "budget_complete", first.Json());
    Check((first.legs.front().distance > .1 || first.legs.front().rotation > 1) &&
            first.observedArea > 0,
      "Room run must physically survey and map: " + first.Json());
    Check(first.SameSimulation(second),
      "Same seed restores pose, observations, timers and RNG: " + scene);
    c.seed = 43;
    const auto changed = run(c);
    Check(first.mapHash != changed.mapHash || first.traceHash != changed.traceHash,
      "Nonzero noise must respond to seed changes: " + scene);
  }

  Case mouse{"microbot", "Microbot", "CellFloodFill", "Maze", 42, 2, 1800, 0, 0, 0, 0, 0};
  auto first = run(mouse), second = run(mouse);
  Check(first.outcome == "success" && first.legs.size() == 3, first.Json());
  Check(first.legs[0].name == "discover" && first.legs[1].name == "return" &&
          first.legs[2].name == "solve",
    "Maze stages are separate");
  Check(first.legs[2].distance < first.legs[0].distance * .8,
    "Learned solve is not exploration replay");
  Check(first.SameSimulation(second), "Fresh Microbot repeated run must match");
  mouse.limit = 10;
  mouse.noise = .002;
  first = run(mouse);
  second = run(mouse);
  Check(first.outcome == "timeout" && first.SameSimulation(second),
    "IR noise resets deterministically");
  mouse.seed = 43;
  const auto anotherSeed = run(mouse);
  Check(first.mapHash != anotherSeed.mapHash || first.traceHash != anotherSeed.traceHash,
    "Infrared seed changes actual observations");

  Case timeout{"timeout", "LiDAR", "AStar", "Explore", 42, 1, 1, 0, 0, 0, 0, 1};
  const auto failed = run(timeout);
  Check(failed.outcome == "timeout" && failed.legs[0].seconds == 1,
    "Missing sensors cannot bypass tick deadline");
  Case rejected{"unknown", "LiDAR", "AStar", "Destination", 42, 1, 5, 5, 100, 100, 0, 0};
  Check(run(rejected).outcome == "destination_rejected",
    "Unobserved destination is recorded, never revealed");
  int calls = 0;
  Check(run(timeout, [&] { return ++calls > 10; }).outcome == "cancelled",
    "Cancellation records active result");
  Check(Read(root / "Scenes/LiDAR.pfscene") == original, "Benchmark never rewrites authored scene");
  Check(!fs::exists(output / "work"), "Per-run working assets removed");

  const auto planPath = output / "plan.pfbench";
  {
    std::ofstream file(planPath);
    file << "ROVERLAB_BENCHMARK 1\n"
            "timeout LiDAR AStar 42 2 Explore 1 0 0 0 0 1\n"
            "after Ultrasonic AStar 42 1 Explore 1 1 0 0 0 0\n";
  }
  Check(RunBatch(root, planPath, output / "batch", [] { return false; }) == 1,
    "Failed runs produce unsuccessful batch status");
  const auto journal = Read(output / "batch/results.jsonl");
  Check(journal.find("\"case\":\"after\"") != std::string::npos &&
          journal.find("\"matches_first\":true") != std::string::npos &&
          journal.find("\"runs\":3,\"unsuccessful\":2") != std::string::npos,
    "Timeouts retained, next case runs, repeated failures compared");
  bool refused = false;
  try {
    RunBatch(root, planPath, output / "batch", [] { return false; });
  } catch (const std::exception&) {
    refused = true;
  }
  Check(refused && Read(output / "batch/results.jsonl") == journal,
    "Existing results cannot be overwritten");
  std::cout << "RL9 benchmark regressions passed\n";
}

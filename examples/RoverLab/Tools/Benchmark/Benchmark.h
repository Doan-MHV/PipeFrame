#pragma once

#include <filesystem>
#include <functional>
#include <istream>
#include <string>
#include <vector>

namespace RoverLab::benchmark {

// A plan row is one scenario/algorithm/seed, repeated from independent fresh runtimes.
struct Case {
  std::string name, scene, algorithm, workflow;
  int seed{42}, repeats{2};
  double limit{600}, explore{300}, destinationX{}, destinationY{};
  // -1 preserves the authored setting. IR supports noise but not dropout.
  double noise{-1}, dropout{-1};
};

struct Leg {
  std::string name, outcome, reason;
  double seconds{}, distance{}, rotation{};
  unsigned turns{}, reversals{}, collisions{};
};

struct Result {
  Case scenario;
  unsigned repetition{};
  std::string outcome, reason, explorationPlanner, routePlanner;
  std::string traceHash, mapHash;
  std::string finalPhase, solverStatus, safetyReason, explorationOutcome;
  double wallSeconds{}, observedArea{}, startX{}, startY{}, startDegrees{};
  std::vector<Leg> legs;
  std::string Json() const;
  // Excludes wall-clock speed and repetition number; compares every simulated result.
  bool SameSimulation(const Result& other) const;
};

std::vector<Case> ReadPlan(std::istream& input);
std::string Quote(const std::string& value);

// Uses the production RoverLabAdapter. The workspace and resolved scene are outputs,
// never the user's project directory. Limits count fixed simulation ticks.
Result RunCase(
  const Case& scenario,
  unsigned repetition,
  const std::filesystem::path& snapshot,
  const std::filesystem::path& workspace,
  const std::filesystem::path& resolvedScene,
  const std::function<bool()>& cancelled = [] { return false; });

// Returns nonzero for failures, timeouts, cancellation, or repeatability mismatches.
int RunBatch(const std::filesystem::path& project,
  const std::filesystem::path& plan,
  const std::filesystem::path& output,
  const std::function<bool()>& cancelled);

}  // namespace RoverLab::benchmark

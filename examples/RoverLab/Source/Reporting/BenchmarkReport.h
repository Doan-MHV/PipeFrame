#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <optional>
#include <set>
#include <sstream>
#include <system_error>

#include "ResultJson.h"

namespace RoverLab::reports {
inline void Require(bool okay, const std::string& message) {
  if (!okay) throw std::runtime_error(message);
}
inline std::string Number(double value) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17) << value;
  return out.str();
}
inline double Metric(const detail::Json& j, const std::string& key) {
  const double n = j.At(key).Number();
  Require(n >= 0, "Negative metric: " + key);
  return n;
}
inline double Integer(const detail::Json& j, const std::string& key) {
  double n = Metric(j, key);
  Require(n == std::floor(n), "Non-integer field: " + key);
  return n;
}
inline std::string Coordinates(const detail::Json& j, std::size_t size) {
  Require(j.Items().size() == size, "Invalid coordinate dimensions");
  std::string text;
  for (const auto& n : j.Items()) {
    if (!text.empty()) text += ';';
    text += Number(n.Number());
  }
  return text;
}
inline bool Successful(const std::string& outcome) {
  return outcome == "success" || outcome == "complete";
}
struct Leg {
  std::string name, outcome, reason;
  double seconds{}, distance{}, rotation{}, turns{}, reversals{}, collisions{};
};
struct Run {
  std::string id, caseName, scene, algorithm, workflow, outcome{"incomplete"},
    reason{"Started without a finished record"};
  std::string explorationPlanner, routePlanner, phase, safety, solverStatus, explorationOutcome;
  std::string start, destination, traceHash, mapHash, repeatability{"not checked"};
  double seed{}, repetition{}, limit{}, budget{}, noise{}, dropout{}, step{}, wall{}, area{};
  bool finished{};
  std::vector<Leg> legs;
};
inline std::vector<std::string> Stages(const Run& r) {
  if (!r.finished) return {};
  std::vector<std::string> stages;
  if (r.workflow == "Maze")
    stages = {"discover", "return", "solve"};
  else if (r.workflow == "Destination")
    stages = {"explore", "destination"};
  else
    stages = {"explore"};
  for (const auto& l : r.legs)
    if (std::find(stages.begin(), stages.end(), l.name) == stages.end()) stages.push_back(l.name);
  return stages;
}
struct Batch {
  std::filesystem::path path;
  std::vector<Run> runs;
  bool hasSummary{};
  unsigned mismatches{};
  std::string Warning() const {
    const auto incomplete =
      std::count_if(runs.begin(), runs.end(), [](const Run& r) { return !r.finished; });
    return (hasSummary ? "" : "No final summary: partial/live batch. ") +
           (incomplete ? std::to_string(incomplete) + " unfinished attempt(s). " : "") +
           (mismatches ? std::to_string(mismatches) + " repeatability mismatch(es)." : "");
  }
};

inline Run ParseRun(const detail::Json& j, const std::string& id) {
  Require(j.At("schema").Number() == 1, "Unsupported result schema (expected 1)");
  Run r;
  r.id = id;
  r.finished = true;
  r.caseName = j.At("case").Text();
  r.scene = j.At("scene").Text();
  r.algorithm = j.At("algorithm").Text();
  r.workflow = j.At("workflow").Text();
  r.outcome = j.At("outcome").Text();
  r.reason = j.At("reason").Text();
  r.explorationPlanner = j.At("exploration_planner").Text();
  r.routePlanner = j.At("route_planner").Text();
  r.phase = j.At("final_phase").Text();
  r.safety = j.At("safety_reason").Text();
  r.solverStatus = j.At("solver_status").Text();
  r.explorationOutcome = j.At("exploration_outcome").Text();
  r.traceHash = j.At("trace_hash").Text();
  r.mapHash = j.At("map_hash").Text();
  r.seed = Integer(j, "seed");
  r.repetition = Integer(j, "repetition");
  r.limit = Metric(j, "limit_seconds");
  r.budget = Metric(j, "exploration_budget");
  r.noise = j.At("noise_override").Number();
  r.dropout = j.At("dropout_override").Number();
  r.step = Metric(j, "fixed_step_seconds");
  r.wall = Metric(j, "wall_seconds");
  r.area = Metric(j, "observed_area_m2");
  Require(r.step > 0 && r.repetition >= 1, "Invalid step or repetition");
  r.start = Coordinates(j.At("start_world"), 3);
  r.destination = Coordinates(j.At("destination_world"), 2);
  Require(!r.caseName.empty() && !r.scene.empty() && !r.outcome.empty(), "Empty result identity");
  std::set<std::string> names;
  Require(j.At("legs").Items().size() <= 16, "Too many stages");
  for (const auto& leg : j.At("legs").Items()) {
    Leg l;
    l.name = leg.At("name").Text();
    l.outcome = leg.At("outcome").Text();
    l.reason = leg.At("reason").Text();
    Require(!l.name.empty() && names.insert(l.name).second, "Duplicate/empty stage");
    l.seconds = Metric(leg, "seconds");
    l.distance = Metric(leg, "distance_m");
    l.rotation = Metric(leg, "rotation_rad");
    l.turns = Integer(leg, "turn_episodes");
    l.reversals = Integer(leg, "direction_reversals");
    l.collisions = Integer(leg, "collision_entries");
    r.legs.push_back(std::move(l));
  }
  return r;
}

inline Batch Load(const std::filesystem::path& inputPath) {
  Batch b;
  b.path = std::filesystem::canonical(
    std::filesystem::is_directory(inputPath) ? inputPath / "results.jsonl" : inputPath);
  Require(
    std::filesystem::is_regular_file(b.path), "Choose a results.jsonl file or batch directory");
  Require(std::filesystem::file_size(b.path) <= 64 * 1024 * 1024, "Journal exceeds 64 MiB");
  std::ifstream file(b.path);
  Require(bool(file), "Cannot open journal");
  std::map<std::string, std::size_t> ids;
  std::set<std::string> repeatRecords;
  std::string line;
  unsigned lineNumber = 0;
  while (std::getline(file, line)) {
    ++lineNumber;
    if (line.find_first_not_of(" \r\t") == std::string::npos) continue;
    try {
      Require(line.size() <= 1024 * 1024, "Record exceeds 1 MiB");
      Require(!b.hasSummary, "Records after final summary");
      const auto j = detail::JsonReader(line).Read();
      const auto event = j.At("event").Text();
      if (event == "summary") {
        const auto finished =
          std::count_if(b.runs.begin(), b.runs.end(), [](const Run& r) { return r.finished; });
        const auto unsuccessful = std::count_if(b.runs.begin(), b.runs.end(), [](const Run& r) {
          return !Successful(r.outcome) && r.outcome != "budget_complete";
        });
        Require(Integer(j, "runs") == b.runs.size() && std::size_t(finished) == b.runs.size(),
          "Summary run count disagrees with records");
        Require(Integer(j, "unsuccessful") == unsuccessful &&
                  Integer(j, "repeatability_mismatches") == b.mismatches,
          "Summary outcome counts disagree with records");
        b.hasSummary = true;
        continue;
      }
      const auto id = j.At("run").Text();
      Require(!id.empty(), "Empty run ID");
      if (event == "started") {
        Require(ids.emplace(id, b.runs.size()).second, "Duplicate started run");
        Require(b.runs.size() < 10000, "Journal exceeds 10000 runs");
        Run r;
        r.id = id;
        b.runs.push_back(std::move(r));
      } else {
        Require(ids.contains(id), "Result without started record");
        auto& r = b.runs[ids.at(id)];
        if (event == "finished") {
          Require(!r.finished, "Duplicate finished run");
          r = ParseRun(j.At("result"), id);
        } else if (event == "repeatability") {
          Require(r.finished && repeatRecords.insert(id).second,
            "Invalid/duplicate repeatability record");
          bool same = j.At("matches_first").Boolean();
          r.repeatability = same ? "match" : "MISMATCH";
          if (!same) ++b.mismatches;
        } else
          throw std::runtime_error("Unknown journal event: " + event);
      }
    } catch (const std::exception& e) {
      throw std::runtime_error("Line " + std::to_string(lineNumber) + ": " + e.what());
    }
  }
  Require(!file.bad() && !b.runs.empty(), "Unreadable or empty journal");
  return b;
}

struct Filter {
  std::string scene{"All"}, stage{"All"};
  bool Includes(const Run& run) const { return scene == "All" || run.scene == scene; }
  bool Includes(const Leg& leg) const { return stage == "All" || leg.name == stage; }
};
struct Group {
  std::size_t batch{};
  std::string caseName, scene, algorithm, workflow, stage, settings;
  unsigned attempts{}, reached{}, success{}, budgetComplete{}, failed{};
  double seconds{}, distance{}, minSeconds{}, maxSeconds{}, budgetSeconds{}, budgetDistance{};
};
// Keep batches, seeds and scenario settings separate. No cross-maze global winner.
inline std::vector<Group> Summarize(const std::vector<Batch>& batches, const Filter& filter) {
  std::vector<Group> groups;
  std::map<std::string, std::size_t> keys;
  for (std::size_t bi = 0; bi < batches.size(); ++bi) {
    const auto& batch = batches[bi];
    for (const auto& r : batch.runs) {
      if (!r.finished || !filter.Includes(r)) continue;
      const std::string settings = "seed=" + Number(r.seed) + " limit=" + Number(r.limit) +
                                   " budget=" + Number(r.budget) + " noise=" + Number(r.noise) +
                                   " dropout=" + Number(r.dropout) + " start=" + r.start +
                                   " target=" + r.destination + " step=" + Number(r.step) +
                                   " planners=" + r.explorationPlanner + "/" + r.routePlanner;
      // Length-prefixed fields avoid grouping collisions with arbitrary case names.
      const auto field = [](const std::string& s) {
        return std::to_string(s.size()) + ":" + s;
      };
      const auto stages = Stages(r);
      for (const auto& stage : stages) {
        if (filter.stage != "All" && stage != filter.stage) continue;
        const auto key = std::to_string(bi) + ":" + field(r.caseName) + field(r.scene) +
                         field(r.algorithm) + field(r.workflow) + field(settings) + field(stage);
        auto [it, inserted] = keys.emplace(key, groups.size());
        if (inserted) {
          std::ostringstream caption;
          caption.imbue(std::locale::classic());
          caption << "Seed " << Number(r.seed) << " | Budget " << Number(r.budget) << " s | Limit "
                  << Number(r.limit) << " s\n"
                  << std::fixed << std::setprecision(4) << "Noise " << r.noise << " m | Dropout "
                  << r.dropout << " (-1 = authored)";
          groups.push_back(
            {bi, r.caseName, r.scene, r.algorithm, r.workflow, stage, caption.str()});
        }
        auto& g = groups[it->second];
        ++g.attempts;
        const auto leg =
          std::find_if(r.legs.begin(), r.legs.end(), [&](const Leg& l) { return l.name == stage; });
        if (leg == r.legs.end()) continue;
        ++g.reached;
        if (Successful(leg->outcome)) {
          ++g.success;
          g.seconds += leg->seconds;
          g.distance += leg->distance;
          g.minSeconds = g.success == 1 ? leg->seconds : std::min(g.minSeconds, leg->seconds);
          g.maxSeconds = std::max(g.maxSeconds, leg->seconds);
        } else if (leg->outcome == "budget_complete") {
          ++g.budgetComplete;
          g.budgetSeconds += leg->seconds;
          g.budgetDistance += leg->distance;
        } else
          ++g.failed;
      }
    }
  }
  return groups;
}

// Quote every text field and neutralize spreadsheet formulas, including whitespace prefixes.
inline std::string CsvText(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if ((!value.empty() &&
        (value.front() == '\t' || value.front() == '\r' || value.front() == '\n')) ||
      (first != std::string::npos && std::string("=+-@").find(value[first]) != std::string::npos))
    value.insert(0, "'");
  std::string out = "\"";
  for (char c : value) {
    if (c == '"') out += '"';
    out += c;
  }
  return out + '"';
}
inline std::string Csv(const std::vector<Batch>& batches, const Filter& filter) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17);
  out << "source,run,case,scene,algorithm,workflow,seed,repetition,run_outcome,run_reason,"
         "repeatability,leg,leg_outcome,leg_reason,seconds,distance_m,rotation_rad,turn_episodes,"
         "direction_reversals,collision_entries,observed_area_m2,wall_seconds,limit_seconds,"
         "exploration_budget,noise_override,dropout_override,start_x_y_heading,destination_x_y,"
         "fixed_step_seconds,exploration_planner,route_planner,final_phase,safety_reason,solver_"
         "status,exploration_outcome,trace_hash,map_hash,batch_has_summary\r\n";
  for (const auto& b : batches)
    for (const auto& r : b.runs) {
      if (!filter.Includes(r)) continue;
      auto emit = [&](const Leg* l, bool measured = true) {
        std::vector<std::string> cells{CsvText(b.path.string()),
          CsvText(r.id),
          CsvText(r.caseName),
          CsvText(r.scene),
          CsvText(r.algorithm),
          CsvText(r.workflow),
          r.finished ? Number(r.seed) : "",
          r.finished ? Number(r.repetition) : "",
          CsvText(r.outcome),
          CsvText(r.reason),
          CsvText(r.repeatability),
          CsvText(l ? l->name : ""),
          CsvText(l ? l->outcome : ""),
          CsvText(l ? l->reason : "")};
        for (double n : {l ? l->seconds : 0,
               l ? l->distance : 0,
               l ? l->rotation : 0,
               l ? l->turns : 0,
               l ? l->reversals : 0,
               l ? l->collisions : 0})
          cells.push_back(l && measured ? Number(n) : "");
        for (double n : {r.area, r.wall, r.limit, r.budget, r.noise, r.dropout})
          cells.push_back(r.finished ? Number(n) : "");
        cells.insert(cells.end(),
          {CsvText(r.start),
            CsvText(r.destination),
            r.finished ? Number(r.step) : "",
            CsvText(r.explorationPlanner),
            CsvText(r.routePlanner),
            CsvText(r.phase),
            CsvText(r.safety),
            CsvText(r.solverStatus),
            CsvText(r.explorationOutcome),
            CsvText(r.traceHash),
            CsvText(r.mapHash),
            b.hasSummary ? "true" : "false"});
        for (std::size_t i = 0; i < cells.size(); ++i) {
          if (i) out << ',';
          out << cells[i];
        }
        out << "\r\n";
      };
      for (const auto& l : r.legs)
        if (filter.Includes(l)) emit(&l);
      for (const auto& stage : Stages(r)) {
        if (filter.stage != "All" && filter.stage != stage) continue;
        if (std::any_of(
              r.legs.begin(), r.legs.end(), [&](const Leg& l) { return l.name == stage; }))
          continue;
        Leg missing{stage, "not_reached", "No measurements: stage was not reached"};
        emit(&missing, false);
      }
      if (!r.finished && filter.stage == "All") emit(nullptr);
    }
  return out.str();
}
inline void Export(
  const std::filesystem::path& path, const std::vector<Batch>& batches, const Filter& filter) {
  Require(!batches.empty(), "Load results before exporting");
  const auto data = Csv(batches, filter);
  // Exclusive create: a mistyped path can never overwrite a journal or an earlier export.
  FILE* file = std::fopen(path.string().c_str(), "wx");
  if (!file)
    throw std::runtime_error(
      "Cannot create CSV (choose a new filename): " + std::string(std::strerror(errno)));
  const bool wrote = std::fwrite(data.data(), 1, data.size(), file) == data.size();
  const bool closed = std::fclose(file) == 0;
  if (!wrote || !closed) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    throw std::runtime_error("CSV write failed");
  }
}
}  // namespace RoverLab::reports

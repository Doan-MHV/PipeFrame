#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <iostream>

#include "Benchmark.h"
#include "Editor/RoverResultsPanel.h"
#include "Integration/RoverLabAdapter.h"

using namespace RoverLab;
namespace fs = std::filesystem;
namespace {
void Check(bool okay, const std::string& message) {
  if (!okay) throw std::runtime_error(message);
}
void Write(const fs::path& p, const std::string& text) {
  std::ofstream f(p);
  f << text;
  Check(bool(f), "Write fixture");
}
std::string Read(const fs::path& p) {
  std::ifstream f(p);
  return {std::istreambuf_iterator<char>(f), {}};
}
template <class F>
void Reject(F f, const std::string& message) {
  bool threw = false;
  try {
    f();
  } catch (const std::exception&) {
    threw = true;
  }
  Check(threw, message);
}
benchmark::Result Result(std::string name, std::string outcome) {
  benchmark::Result r;
  r.scenario.name = name;
  r.scenario.scene = "Microbot";
  r.scenario.algorithm = "CellFloodFill";
  r.scenario.workflow = "Maze";
  r.scenario.explore = 0;
  r.repetition = 1;
  r.outcome = outcome;
  r.reason = "reason, with \"quotes\"\nand a new line";
  r.explorationPlanner = "CellFloodFill";
  r.routePlanner = "TimeCostAStar";
  r.legs.push_back({"discover", "success", "Arrived", 100, 20, 1, 2, 3, 0});
  return r;
}
std::string Finished(const std::string& id, const benchmark::Result& r) {
  return "{\"event\":\"started\",\"run\":" + benchmark::Quote(id) +
         "}\n{\"event\":\"finished\",\"run\":" + benchmark::Quote(id) + ",\"result\":" + r.Json() +
         "}\n";
}
pipeframe::ui::View* Find(pipeframe::ui::View& root, const std::string& key) {
  if (root.key == key) return &root;
  for (auto& child : root.children)
    if (auto* found = Find(child, key)) return found;
  return nullptr;
}
Widget* FindWidget(Widget& root, const std::string& key) {
  if (root.GetKey() == key) return &root;
  for (std::size_t i = 0; i < root.GetChildCount(); ++i)
    if (auto* w = FindWidget(*root.GetChild(i), key)) return w;
  return nullptr;
}
}  // namespace
int main(int argc, char** argv) {
  const auto temp = fs::temp_directory_path() /
                    ("rover-results-test-" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directory(temp);
  try {
    const auto baselinePath =
      fs::path(ROVER_REPOSITORY_ROOT) / "output/roverlab/rl9-baseline-20261022/results.jsonl";
    // Portable fixture uses the production writer; the preserved baseline is also checked when
    // present.
    auto success = Result("mouse", "success");
    success.legs.push_back({"return", "success", "At home", 60, 10});
    success.legs.push_back({"solve", "success", "At goal", 40, 8});
    auto failed = Result("mouse", "timeout");
    failed.repetition = 2;
    failed.legs.push_back({"return", "timeout", "Deadline", 1800, 12});
    const auto fixture = temp / "results.jsonl";
    Write(fixture,
      Finished("a", success) + Finished("b", failed) +
        "{\"event\":\"repeatability\",\"run\":\"b\",\"matches_first\":false}\n"
        "{\"event\":\"started\",\"run\":\"interrupted\"}\n");
    auto batch = reports::Load(fixture);
    Check(batch.runs.size() == 3 && batch.mismatches == 1 && !batch.hasSummary,
      "Failure and incomplete attempts retained");
    Check(batch.Warning().find("unfinished") != std::string::npos, "Partial batch warning");
    auto groups = reports::Summarize({batch}, {});
    auto solve =
      std::find_if(groups.begin(), groups.end(), [](const auto& g) { return g.stage == "solve"; });
    Check(solve != groups.end() && solve->attempts == 2 && solve->reached == 1 &&
            solve->success == 1 && solve->seconds == 40,
      "Unreached solve not averaged as zero");
    Check(
      reports::Summarize({batch, batch}, {}).size() == groups.size() * 2, "Batches never pooled");
    auto different = batch;
    different.runs[1].noise = .01;
    Check(reports::Summarize({different}, {}).size() == 6, "Different settings never pooled");
    const auto csv = reports::Csv({batch}, {});
    Check(csv.find("reason, with \"\"quotes\"\"\nand a new line") != std::string::npos,
      "CSV escapes quotes/newlines");
    Check(csv.find("incomplete") != std::string::npos && csv.find("timeout") != std::string::npos,
      "CSV preserves unsuccessful records");
    Check(
      reports::CsvText("  =SUM(A1)").front() == '"' && reports::CsvText("  =SUM(A1)")[1] == '\'',
      "Formula injection neutralized");
    Check(reports::CsvText("\t@evil")[1] == '\'', "Control-prefix formula neutralized");
    Check(reports::Csv({batch}, {"LiDAR", "All"}).find("\r\n") ==
            reports::Csv({batch}, {"LiDAR", "All"}).size() - 2,
      "Scene filter exports header only when empty");
    const auto solveCsv = reports::Csv({batch}, {"All", "solve"});
    Check(solveCsv.find("\"return\"") == std::string::npos &&
            solveCsv.find("\"solve\"") != std::string::npos,
      "Stage filter");
    Check(solveCsv.find("\"not_reached\"") != std::string::npos &&
            solveCsv.find("\"timeout\"") != std::string::npos &&
            solveCsv.find("No measurements: stage was not reached\",,,,,,,") != std::string::npos,
      "Filtered CSV retains failed attempts without invented zero measurements");
    reports::Export(temp / "export.csv", {batch}, {});
    Check(Read(temp / "export.csv") == csv, "Export data matches");
    Reject([&] { reports::Export(temp / "export.csv", {batch}, {}); }, "Refuse overwrite");
    Check(Read(temp / "export.csv") == csv, "Existing export preserved");
    Reject([&] { reports::Export(temp / "missing/export.csv", {batch}, {}); },
      "Invalid destination reported");
    auto bad = temp / "bad.jsonl";
    for (const std::string content : {"{}",
           "{\"event\":\"started\",\"run\":\"x\",\"run\":\"y\"}",
           "{\"event\":\"started\",\"run\":\"x\"}\n{",
           "{\"event\":\"finished\",\"run\":\"x\"}"}) {
      Write(bad, content);
      Reject([&] { reports::Load(bad); }, "Malformed journal rejected");
    }
    auto invalid = Finished("x", success);
    invalid.replace(invalid.find("\"schema\":1"), 10, "\"schema\":2");
    Write(bad, invalid);
    Reject([&] { reports::Load(bad); }, "Unknown schema rejected");
    Write(bad,
      Finished("x", success) +
        "{\"event\":\"summary\",\"runs\":9,\"unsuccessful\":0,\"repeatability_mismatches\":0}\n");
    Reject([&] { reports::Load(bad); }, "Bad summary rejected");
    Check(reports::detail::JsonReader("\"\\ud83d\\ude80\"").Read().Text() == "🚀",
      "Unicode surrogate pair");
    for (auto text : {"01", "1e", "-", "NaN", "[1,]", "{\"a\":1,}", "\"\\ud800\"", "1e999"})
      Reject([&] { reports::detail::JsonReader(text).Read(); }, "Invalid JSON syntax rejected");
    if (fs::exists(baselinePath)) {
      auto baseline = reports::Load(baselinePath);
      Check(baseline.runs.size() == 6 && baseline.hasSummary && !baseline.mismatches,
        "Real RL9 baseline compatibility");
      reports::Export(temp / "baseline.csv", {baseline}, {});
    }

    RoverLabAdapter runtime;
    Check(
      runtime.UI().PanelCount() == 5, "Results registered alongside four existing project panels");
    std::string uiError;
    Check(runtime.UI().Prepare(uiError), "Production project UI prepares: " + uiError);

    RoverResultsPanel panel;
    auto commit = [&](const std::string& key, const std::string& value) {
      auto view = panel.DescribeView();
      auto* v = Find(view, key);
      Check(v && bool(v->onCommitted), "Input callback exists");
      v->onCommitted(value);
    };
    auto press = [&](const std::string& key) {
      auto view = panel.DescribeView();
      auto* v = Find(view, key);
      Check(v && v->enabled && bool(v->onPressed), "Button enabled: " + key);
      v->onPressed();
    };
    auto text = [&](const std::string& key) {
      auto view = panel.DescribeView();
      auto* v = Find(view, key);
      Check(v, "Text exists: " + key);
      return v->text;
    };
    commit("results-input", fixture.string());
    press("results-load");
    Check(text("results-overview").find("3 attempts") != std::string::npos, "Panel loads results");
    press("results-load");
    Check(text("results-status").find("already loaded") != std::string::npos,
      "Duplicate batch does not double counts");
    commit("results-input", bad.string());
    press("results-load");
    Check(text("results-overview").find("3 attempts") != std::string::npos,
      "Invalid load keeps previous data");
    const auto savedJournal = Read(fixture);
    Write(fixture, savedJournal + "{");
    press("results-reload");
    Check(text("results-status").find("Error:") != std::string::npos &&
            text("results-overview").find("3 attempts") != std::string::npos,
      "Truncated live journal reload retains previous snapshot");
    Write(fixture, savedJournal);
    press("results-reload");
    Check(text("results-status").find("Reloaded") != std::string::npos, "Reload recovers");
    commit("results-output", (temp / "panel.csv").string());
    press("results-export");
    Check(Read(temp / "panel.csv") == csv, "Panel exports loaded results");
    press("results-export");
    Check(
      text("results-status").find("Error:") != std::string::npos, "Panel reports export failure");
    press("results-scene");
    Check(text("results-empty").find("No rows") != std::string::npos, "Scene filtering reflected");
    press("results-scene");
    press("results-scene");
    press("results-runs");
    Check(text("results-row-0").find("reason, with") != std::string::npos,
      "Individual failure reasons visible");
    press("results-scene");
    Check(text("results-scene").find("MicrobotMedium") != std::string::npos,
      "Medium maze has a comparison filter");
    press("results-scene");
    Check(text("results-scene").find("MicrobotHard") != std::string::npos,
      "Hard maze has a comparison filter");
    press("results-scene");
    press("results-summary");
    commit("results-output", (temp / "native.csv").string());

    UIManager manager;
    Check(manager.LoadDefaultFont(PIPEFRAME_TEST_FONT), "Font loads");
    auto mounted = manager.MountView(panel.DescribeView());
    mounted.SetBounds(0, 0, 620, 680);
    manager.Update(0);
    auto* exportButton = FindWidget(*manager.GetRoot(0), "results-export");
    Check(exportButton, "Native export exists");
    const auto point = exportButton->GetScreenPosition() + exportButton->GetSize() * .5f;
    Check(point.y < 680, "Export above fold");
    manager.HandleEvent(
      sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {int(point.x), int(point.y)}});
    manager.HandleEvent(
      sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {int(point.x), int(point.y)}});
    Check(fs::exists(temp / "native.csv"), "Mounted native click exports CSV");
    mounted.SetView(panel.DescribeView());
    manager.Update(0);
    sf::RenderTexture target({620, 680});
    target.clear(sf::Color{20, 26, 34});
    manager.Render(target);
    target.display();
    Check(target.getTexture().copyToImage().saveToFile("/tmp/rover-results-panel.png"),
      "Panel screenshot");
    press("results-clear");
    Check(text("results-overview").find("0 attempts") != std::string::npos,
      "Clear removes loaded data");
    if (argc == 2) {
      const fs::path evidence = argv[1];
      Check(!fs::exists(evidence) && fs::create_directories(evidence),
        "Evidence directory must be new");
      commit("results-input", baselinePath.string());
      press("results-load");
      Check(text("results-overview").find("6 attempts") != std::string::npos, "Baseline displayed");
      commit("results-output", (evidence / "baseline.csv").string());
      press("results-export");
      Check(fs::exists(evidence / "baseline.csv"), "Baseline exported through panel");
      mounted.SetView(panel.DescribeView());
      manager.Update(0);
      const auto screenshot = [&](const char* name) {
        target.clear(sf::Color{20, 26, 34});
        manager.Render(target);
        target.display();
        Check(target.getTexture().copyToImage().saveToFile(evidence / name), "Evidence screenshot");
      };
      screenshot("comparison-controls.png");
      manager.HandleEvent(
        sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -10, {580, 600}});
      manager.Update(0);
      screenshot("comparison-stages.png");
      std::cout << "Evidence: " << evidence << '\n';
    }
    fs::remove_all(temp);
    std::cout << "RL10 report parsing, comparison, failure accounting, CSV and native panel input "
                 "passed.\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\nFixtures: " << temp << '\n';
    return 1;
  }
}

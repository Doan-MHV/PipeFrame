#pragma once

#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/ViewPanel.h>

#include "Reporting/BenchmarkReport.h"

namespace RoverLab {
class ProjectWorld;

// A report viewer: loading, filtering and exporting never touches the simulation.
class RoverResultsPanel final : public pipeframe::ui::ViewPanel {
public:
  RoverResultsPanel() = default;
  RoverResultsPanel(ProjectWorld&, pipeframe::ui::ProjectUIHost&) {}

protected:
  pipeframe::ui::View BuildView() override {
    using namespace pipeframe::ui;
    std::vector<View> controls{
      views::Text("results-intro",
        "RL10 COMPARISON\nLoad RL9 results.jsonl files or batch folders. Press Enter to commit "
        "paths. No simulation needs to be running.")
        .FitHeight(),
      views::TextField("results-input",
        "Result file / batch folder",
        input,
        [this](const std::string& text) { input = text; }),
      views::Row("results-load-actions",
        {
          views::Button("results-load", "ADD BATCH", [this] { Action([&] { Add(); }); }),
          views::Button("results-reload", "RELOAD", [this] { Action([&] { Reload(); }); })
            .Enabled(!batches.empty()),
          views::Button("results-clear",
            "CLEAR",
            [this] {
              batches.clear();
              page = 0;
              status = "Results cleared";
              RefreshView();
            })
            .Enabled(!batches.empty()),
        }),
      views::Text("results-status", status).FitHeight(),
      views::Row("results-filters",
        {
          views::Button("results-scene",
            "SCENE: " + filter.scene,
            [this] {
              Cycle(filter.scene,
                {"All", "Ultrasonic", "LiDAR", "Microbot", "MicrobotMedium", "MicrobotHard"});
            }),
          views::Button("results-stage",
            "STAGE: " + filter.stage,
            [this] {
              Cycle(filter.stage, {"All", "explore", "destination", "discover", "return", "solve"});
            }),
        }),
      views::Row("results-views",
        {
          views::Button("results-summary",
            "SUMMARY",
            [this] {
              summary = true;
              page = 0;
              RefreshView();
            })
            .Selected(summary),
          views::Button("results-runs",
            "RUN DETAILS",
            [this] {
              summary = false;
              page = 0;
              RefreshView();
            })
            .Selected(!summary),
        }),
      views::TextField("results-output",
        "New CSV filename (no overwrite)",
        output,
        [this](const std::string& text) { output = text; }),
      views::Button("results-export",
        "EXPORT FILTERED CSV",
        [this] {
          Action([&] {
            reports::Require(!output.empty(), "Enter a new CSV filename first");
            reports::Export(output, batches, filter);
            status = "CSV saved: " + std::filesystem::absolute(output).string();
          });
        })
        .Enabled(!batches.empty()),
    };

    unsigned runs = 0, successes = 0, budgets = 0, failures = 0, incomplete = 0, mismatches = 0;
    for (const auto& b : batches) {
      mismatches += b.mismatches;
      for (const auto& r : b.runs) {
        ++runs;
        if (!r.finished)
          ++incomplete;
        else if (reports::Successful(r.outcome))
          ++successes;
        else if (r.outcome == "budget_complete")
          ++budgets;
        else
          ++failures;
      }
    }
    controls.push_back(views::Text("results-overview",
      std::to_string(batches.size()) + " batches | " + std::to_string(runs) + " attempts\n" +
        "Success " + std::to_string(successes) + " | Budget complete " + std::to_string(budgets) +
        " | Unsuccessful " + std::to_string(failures) + " | Unfinished " +
        std::to_string(incomplete) + "\nRepeatability mismatches: " + std::to_string(mismatches) +
        "\nTotals include all loaded attempts. CSV uses scene/stage filters, all pages; "
        "unclassified unfinished runs require All / All.")
        .FitHeight());

    std::vector<std::string> rows;
    if (summary) {
      for (const auto& g : reports::Summarize(batches, filter)) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(2);
        s << "Batch " << g.batch + 1 << " | " << g.scene << " | " << g.caseName << '\n'
          << g.algorithm << " | " << g.workflow << " / " << g.stage << '\n'
          << "Attempts " << g.attempts << " | Reached stage " << g.reached << " | Not reached "
          << g.attempts - g.reached << '\n'
          << "Success " << g.success << " | Budget " << g.budgetComplete << " | Failed "
          << g.failed;
        if (g.success)
          s << "\nSuccessful-stage mean: " << g.seconds / g.success << " s | "
            << g.distance / g.success << " m\nTime range: " << g.minSeconds << " .. "
            << g.maxSeconds << " s";
        if (g.budgetComplete)
          s << "\nBudget-stage mean: " << g.budgetSeconds / g.budgetComplete << " s | "
            << g.budgetDistance / g.budgetComplete << " m (coverage not implied)";
        if (!g.success && !g.budgetComplete) s << "\nNo successful/budget-complete metrics";
        s << '\n' << g.settings;
        rows.push_back(s.str());
      }
    } else {
      for (std::size_t bi = 0; bi < batches.size(); ++bi)
        for (const auto& r : batches[bi].runs) {
          if (!filter.Includes(r)) continue;
          const auto stages = reports::Stages(r);
          if (filter.stage != "All" &&
              std::find(stages.begin(), stages.end(), filter.stage) == stages.end())
            continue;
          std::ostringstream s;
          s << std::fixed << std::setprecision(2);
          s << "Batch " << bi + 1 << " | " << r.id << " | " << r.scene << '\n'
            << r.outcome << ": " << r.reason << "\nRepeatability: " << r.repeatability;
          if (r.finished) {
            s << "\n"
              << r.explorationPlanner << " / " << r.routePlanner << " | Seed " << r.seed
              << "\nObserved area " << r.area << " m2 | Run wall time " << r.wall
              << " s (not planner CPU)";
            for (const auto& l : r.legs)
              if (filter.Includes(l))
                s << "\n"
                  << l.name << " / " << l.outcome << ": " << l.seconds << " s | " << l.distance
                  << " m\nTurns " << l.turns << " | Reversals " << l.reversals << " | Contacts "
                  << l.collisions << "\n"
                  << l.reason;
            for (const auto& stage : stages) {
              if (filter.stage != "All" && stage != filter.stage) continue;
              if (std::none_of(r.legs.begin(), r.legs.end(), [&](const reports::Leg& l) {
                    return l.name == stage;
                  }))
                s << "\n" << stage << " / not reached (no measurements)";
            }
            s << "\nFinal: " << r.phase << "\nSafety: " << r.safety;
          }
          rows.push_back(s.str());
        }
    }
    constexpr std::size_t pageSize = 4;
    const std::size_t pages = std::max<std::size_t>(1, (rows.size() + pageSize - 1) / pageSize);
    page = std::min(page, pages - 1);
    controls.push_back(views::Row("results-pages",
      {
        views::Button("results-prev",
          "PREVIOUS",
          [this] {
            if (page) --page;
            RefreshView();
          })
          .Enabled(page > 0),
        views::Text("results-page", std::to_string(page + 1) + " / " + std::to_string(pages)),
        views::Button("results-next",
          "NEXT",
          [this] {
            ++page;
            RefreshView();
          })
          .Enabled(page + 1 < pages),
      }));
    if (rows.empty())
      controls.push_back(
        views::Text("results-empty", "No rows match. Load results or change the filters.")
          .FitHeight());
    for (std::size_t i = page * pageSize; i < std::min(rows.size(), (page + 1) * pageSize); ++i)
      controls.push_back(views::Column("results-card-" + std::to_string(i),
        {views::Text("results-row-" + std::to_string(i), rows[i]).FitHeight()})
          .Surface()
          .Padding(10)
          .FitHeight());

    controls.push_back(views::Text("results-caveats",
      "Compare matching scenarios and stages. Batches/seeds/settings stay separate. No global "
      "winner across room and maze scenes. Means exclude failed stages; counts retain them. Source "
      "snapshots define hardware/map settings. An uncompleted stage is not a zero-second solve.")
        .FitHeight());
    for (std::size_t i = 0; i < batches.size(); ++i)
      controls.push_back(views::Text("results-source-" + std::to_string(i),
        "Batch " + std::to_string(i + 1) + ": " + batches[i].path.string() + "\n" +
          batches[i].Warning())
          .FitHeight());
    controls.push_back(views::Button("results-hide", "HIDE", [this] { Hide(); }));
    return views::Scroll("results-scroll",
      views::Column("results-content", std::move(controls)).Padding(12).Spacing(10).FitHeight())
      .FillHeight();
  }

private:
  template <class F>
  void Action(F action) {
    try {
      action();
    } catch (const std::exception& e) {
      status = "Error: " + std::string(e.what()) + "\nPreviously loaded results retained.";
    }
    RefreshView();
  }
  void Add() {
    reports::Require(!input.empty(), "Enter a result file or batch folder first");
    reports::Require(batches.size() < 16, "At most 16 batches can be loaded");
    auto batch = reports::Load(input);
    std::size_t total = batch.runs.size();
    for (const auto& existing : batches)
      total += existing.runs.size();
    reports::Require(total <= 10000, "At most 10000 attempts can be loaded together");
    for (const auto& existing : batches)
      reports::Require(existing.path != batch.path, "Batch already loaded; use Reload");
    if (output.empty()) output = (batch.path.parent_path() / "comparison.csv").string();
    status = "Loaded " + std::to_string(batch.runs.size()) + " attempts. " + batch.Warning();
    batches.push_back(std::move(batch));
    page = 0;
  }
  void Reload() {
    std::vector<reports::Batch> fresh;
    for (const auto& b : batches)
      fresh.push_back(reports::Load(b.path));
    std::size_t total = 0;
    for (const auto& b : fresh)
      total += b.runs.size();
    reports::Require(total <= 10000, "At most 10000 attempts can be loaded together");
    batches = std::move(fresh);
    page = 0;
    status = "Reloaded " + std::to_string(batches.size()) + " batches.";
  }
  void Cycle(std::string& value, std::vector<std::string> choices) {
    auto it = std::find(choices.begin(), choices.end(), value);
    value = choices[(std::size_t(it - choices.begin()) + 1) % choices.size()];
    page = 0;
    RefreshView();
  }
  std::vector<reports::Batch> batches;
  reports::Filter filter;
  std::string input, output, status{"No results loaded"};
  bool summary{true};
  std::size_t page{};
};
}  // namespace RoverLab

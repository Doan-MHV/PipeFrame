#include <PipeFrame/Project/EditorWorkspace.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {
void Require(const bool condition, const char *message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using namespace pipeframe::editor_workspace;
    WorkspaceManager workspace;
    std::string error;
    Require(workspace.RegisterPanel({"hierarchy", DockSite::Left, "scene", 0, true, true, 280}),
            "Hierarchy panel should register.");
    Require(workspace.RegisterPanel({"inspector", DockSite::Right, "properties", 0, true, true, 360}),
            "Inspector panel should register.");
    Require(workspace.RegisterPanel({"console", DockSite::Bottom, "output", 0, true, true, 220}),
            "Console panel should register.");
    Require(workspace.RegisterPanel({"profiler", DockSite::Bottom, "output", 1, true, false, 220}),
            "Profiler tab should register.");
    Require(workspace.RegisterPanel({"scene-viewport", DockSite::Center, "viewport", 0, true, true}),
            "Scene viewport should register.");
    Require(workspace.RegisterPanel({"game-viewport", DockSite::Center, "viewport", 1, true, false}),
            "Game viewport should register.");
    Require(!workspace.RegisterPanel({"console"}, &error), "Duplicate panel IDs should be rejected.");
    Require(workspace.SelectTab("profiler") && workspace.Find("profiler")->selected &&
                !workspace.Find("console")->selected,
            "Selecting a tab should update its whole tab group.");
    Require(workspace.Resize("inspector", 470) && workspace.Find("inspector")->dockSize == 470,
            "Dock sizes should be editable.");
    Require(workspace.Dock("profiler",DockSite::Left,"diagnostics")&&
                workspace.Dock("profiler",DockSite::Right,"diagnostics")&&
                workspace.Dock("profiler",DockSite::Bottom,"output"),
            "A registered panel should move between every supported dock edge.");
    Require(workspace.SelectTab("game-viewport")&&workspace.Find("game-viewport")->selected&&
                !workspace.Find("scene-viewport")->selected,
            "Scene and game viewport hosts should switch as one center tab group.");
    Require(workspace.Float("profiler", {{1380, 120}, {700, 520}}, "secondary"),
            "A panel should float onto an available secondary display.");
    workspace.Reconcile({"hierarchy", "inspector", "console", "profiler", "scene-viewport", "game-viewport"},
                        {{"primary", {{0, 0}, {1280, 720}}, 2.0f},
                         {"secondary", {{1280, 0}, {1280, 1024}}, 1.0f}}, {{0, 0}, {1280, 720}});
    Require(workspace.Find("profiler")->displayId=="secondary"&&
                workspace.Find("profiler")->floatingBounds.position.x>=1280,
            "A floating panel should remain assigned to an available display and its DPI work area.");
    Require(workspace.Float("profiler", {{1900, 900}, {900, 700}}, "removed-display"),
            "A panel should float onto a display.");
    workspace.Reconcile({"hierarchy", "inspector", "console", "profiler", "scene-viewport", "game-viewport"},
                        {{"primary", {{0, 0}, {1280, 720}}, 2.0f}}, {{0, 0}, {1280, 720}});
    const auto *floating = workspace.Find("profiler");
    Require(floating && floating->displayId.empty() &&
                floating->floatingBounds.position.x + floating->floatingBounds.size.x <= 1280,
            "A layout should recover a floating panel from a missing monitor.");

    const auto path = std::filesystem::temp_directory_path() / "pipeframe-workspace-test.layout";
    Require(workspace.Save(path, &error), "Workspace layout should save.");
    WorkspaceManager loaded;
    Require(loaded.Load(path, &error) && loaded.Find("inspector") &&
                loaded.Find("inspector")->dockSize == 470 && loaded.Validate().empty(),
            "Workspace layout should round-trip.");
    loaded.Reconcile({"hierarchy", "inspector", "console", "scene-viewport", "game-viewport"}, {}, {{0, 0}, {1024, 768}});
    Require(!loaded.Find("profiler"), "Missing plugin panels should be removed during reconciliation.");
    loaded.Reset();
    Require(loaded.GetLayout().panels.empty(), "Reset should restore an empty default layout.");
    std::filesystem::remove(path);
    std::cout << "All editor workspace tests passed.\n";
}

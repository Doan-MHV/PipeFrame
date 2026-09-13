#include "Editor/ProjectManager.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main() {
    namespace fs = std::filesystem;
    using namespace pipeframe::editor;

    const fs::path temporaryRoot =
        fs::temp_directory_path() / "pipeframe_sailboat_project_tests";
    std::error_code cleanupError;
    fs::remove_all(temporaryRoot, cleanupError);

    ProjectManager projectManager(temporaryRoot / ".recent-projects");
    SceneDocument document;
    std::string errorMessage;

    bool passed = true;
    passed &= Check(
        projectManager.OpenProject(PIPEFRAME_SAILBOAT_PROJECT_DIR, document, &errorMessage),
        "The Workbench ProjectManager should open SailBoatSimulation. " + errorMessage);

    const ProjectManifest *manifest = projectManager.GetActiveManifest();
    passed &= Check(manifest != nullptr, "Opening the project should activate its manifest.");
    if (manifest != nullptr) {
        passed &= Check(manifest->name == "SailBoat Simulation",
                        "The project should expose its authored display name.");
        passed &= Check(manifest->startupScene == fs::path{"Scenes/Main.pfscene"},
                        "The project should open the authored main scene.");
        passed &= Check(manifest->runtimeLibrary == fs::path{"Build/SailBoatSimulationRuntime"},
                        "The project should point to its generated runtime plugin.");
    }

    passed &= Check(document.GetObjects().size() == 5,
                    "The main scene should contain all five foundation objects.");
    passed &= Check(!document.IsDirty(), "An opened project scene should begin clean.");

    if (document.GetObjects().size() == 5) {
        passed &= Check(document.GetObjects()[0].typeId == "sailboat.race-start",
                        "The first object should define the race start.");
        passed &= Check(document.GetObjects()[1].typeId == "sailboat.finish-line",
                        "The second object should define the finish line.");
        passed &= Check(document.GetObjects()[2].typeId == "sailboat.waypoint",
                        "The third object should define the ordered waypoint.");
        passed &= Check(document.GetObjects()[3].typeId == "sailboat.environment",
                        "The fourth object should define the sailing environment.");
        passed &= Check(document.GetObjects()[4].typeId == "sailboat.training-settings",
                        "The fifth object should define training settings.");
    }

    fs::remove_all(temporaryRoot, cleanupError);

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat project integration tests passed.\n";
    return 0;
}

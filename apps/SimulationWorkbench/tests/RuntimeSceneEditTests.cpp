#include "../Editor/ProjectSession.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace pipeframe;
    using namespace pipeframe::editor;

    ProjectSession session(std::filesystem::temp_directory_path() / "pipeframe_runtime_edit_test");
    session.GetDocument().CreateObject("OLD MARK", "sailboat.waypoint");
    session.GetDocument().CreateObject("ENVIRONMENT", "sailboat.environment");
    session.GetDocument().MarkClean();

    SceneObjectData replacement;
    replacement.name = "WAYPOINT 1";
    replacement.typeId = "sailboat.waypoint";
    replacement.transform.position = {50.0f, 75.0f};

    std::vector<ProjectRuntimeSceneEdit> edits;
    edits.push_back({ProjectRuntimeSceneEditKind::RemoveObjectType, "sailboat.waypoint", {}});
    edits.push_back({ProjectRuntimeSceneEditKind::CreateObject, {}, replacement});
    Require(session.ApplyRuntimeSceneEdits(std::move(edits)), "Runtime scene edit batch should apply.");
    Require(session.GetDocument().GetObjects().size() == 2,
            "Runtime race replacement must preserve non-race objects.");
    Require(session.CanUndo(), "Runtime scene edits should create one undo step.");
    Require(session.Undo(), "Runtime scene edit undo should succeed.");
    Require(session.GetDocument().GetObjects().size() == 2,
            "Undo should restore the pre-edit scene.");
    Require(session.GetDocument().GetObjects()[0].name == "OLD MARK",
            "Undo should restore the old race mark.");
    return 0;
}

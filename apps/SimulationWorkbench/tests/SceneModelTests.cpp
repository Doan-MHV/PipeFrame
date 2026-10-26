#include "../Editor/ProjectSession.h"
#include "../Editor/ProjectScaffolder.h"
#include "../Editor/SceneSerializer.h"
#include "../Editor/SceneWorkspace.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ranges>

namespace {
void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace pipeframe;
    using namespace pipeframe::editor;

    ProjectSession session(std::filesystem::temp_directory_path() / "pipeframe_scene_model_test");
    auto &document = session.GetDocument();
    const auto root = document.CreateObject("ROBOT", "test.robot", {{10, 20}, 90, {2, 2}});
    const auto sensor = document.CreateObject("LIDAR", "test.sensor", {{5, 0}, 0, {1, 1}});
    Require(document.SetParent(sensor, root), "A child should be parented.");
    Require(!document.SetParent(root, sensor), "Hierarchy cycles must be rejected.");
    Require(document.GetWorldTransform(sensor).position == Vector2f{10, 30},
            "World transform should inherit scale and rotation.");

    Require(document.SetLayer(sensor, "Sensors"), "Layer should update.");
    Require(document.SetTags(sensor, {"range", "robot", "range"}), "Tags should update and deduplicate.");
    Require(document.SetVisible(sensor, false), "Visibility should update.");
    Require(document.FindObjects("lid", "Sensors", "range", false).empty(),
            "Filters should honor hidden objects.");
    Require(document.FindObjects("lid", "Sensors", "range", true) == std::vector<SceneObjectId>{sensor},
            "Search should combine name, layer, and tag filters.");

    SceneComponentData lidar{"robotics.lidar", 3, {{"range", 12.0}}, true, false};
    Require(document.AddComponent(sensor, lidar), "A project component should attach.");
    Require(!document.AddComponent(sensor, lidar), "Duplicate component types should be rejected.");
    Require(document.SetComponentProperty(sensor, "robotics.lidar", "range", 20.0),
            "Component properties should update.");
    Require(document.SetComponentProperty(sensor, Transform2DComponentTypeId, "position", Vector2f{6, 0}) &&
                document.FindObject(sensor)->transform.position == Vector2f{6, 0},
            "Transform component edits should update the canonical transform.");
    Require(!document.RemoveComponent(sensor, Transform2DComponentTypeId),
            "The required Transform component must not be removable.");
    Require(session.AddSceneConnection({7, SceneConnectionKind::Signal,
                                        {root, "controller.signal"}, {sensor, "lidar.signal"}}),
            "Typed stable endpoint connections should be stored by the scene.");
    Require(!document.AddConnection({8, SceneConnectionKind::Signal,
                                     {sensor, "lidar.signal"}, {root, "controller.signal"}}),
            "Duplicate endpoint connections should be rejected.");
    Require(session.Undo() && document.GetConnections().empty(),
            "Connection authoring should participate in undo.");
    Require(session.Redo() && document.GetConnections().size() == 1,
            "Connection authoring should participate in redo.");
    Require(document.Validate().empty(), "The scene model should validate.");

    session.SetSelectedObjects({root, sensor});
    Require(session.SetSelectedObjectsLocked(true), "Multi-selection locking should be transactional.");
    Require(session.CanUndo() && session.Undo(), "Hierarchy metadata edits should be undoable.");
    Require(!document.FindObject(root)->locked && !document.FindObject(sensor)->locked,
            "Undo should restore metadata for every selected object.");
    Require(session.Redo(), "Hierarchy metadata edits should be redoable.");

    session.SetSelectedObject(root);
    Require(session.DuplicateSelectedObjects(), "A hierarchy should duplicate recursively.");
    const auto duplicateRoot = session.GetSelectedObjectId();
    Require(duplicateRoot && document.GetChildren(*duplicateRoot).size() == 1,
            "Recursive duplication should retain the child hierarchy.");

    session.SetSelectedObjects({root, *duplicateRoot});
    Require(session.GroupSelectedObjects("ROBOTS"), "Selected roots should group under a new parent.");
    const auto group = session.GetSelectedObjectId();
    Require(group && document.GetChildren(*group).size() == 2,
            "Grouping should parent all selected roots in one undo step.");
    Require(session.Undo(), "Grouping should undo as one transaction.");
    Require(document.FindObject(group.value()) == nullptr, "Undo should remove the generated group.");

    SceneWorkspace workspace;
    const auto mainScene = workspace.CreateScene("Main", document);
    const auto testScene = workspace.DuplicateScene(mainScene, "Test Environment");
    Require(mainScene != 0 && testScene != 0 && workspace.GetLoadedScenes().size() == 1,
            "Scene templates and duplication should create independent scenes.");
    Require(workspace.SetAdditivelyLoaded(testScene, true) && workspace.GetLoadedScenes().size() == 2,
            "A second scene should load additively.");
    Require(workspace.SetActiveScene(testScene) && !workspace.SetAdditivelyLoaded(testScene, false),
            "The active scene must remain loaded.");

    const auto generatedRoot = std::filesystem::temp_directory_path() / "pipeframe_data_object_type_test";
    std::error_code cleanupError;
    std::filesystem::remove_all(generatedRoot, cleanupError);
    ProjectSession generatedSession(generatedRoot / ".recent-projects");
    std::string createError;
    Require(generatedSession.CreateProjectAt(generatedRoot / "GeneratedProject", &createError),
            "A standard project should be generated through the public editor workflow.");
    Require(ProjectScaffolder::AddModule(generatedRoot / "GeneratedProject",
                GeneratedModuleKind::Component, "DistanceSensor", &createError) &&
            generatedSession.OpenProject(generatedRoot / "GeneratedProject", &createError),
            "A generated component should be discovered after the project refreshes.");
    Require(std::ranges::any_of(generatedSession.GetComponentTypes(), [](const auto &type) {
                return type.typeId == "project.DistanceSensor";
            }),
            "Generated component metadata should appear in the Inspector without compiling a runtime plugin.");
    Require(!generatedSession.GetObjectTypes().empty(),
            "A generated project should expose its data-only starter object type without a runtime plugin.");
    Require(generatedSession.CreateObject(Vector2f{12, 34}),
            "Create Object should instantiate a generated data-only type without C++ code.");
    Require(generatedSession.GetSelectedObject() != nullptr &&
                generatedSession.GetSelectedObject()->transform.position == Vector2f{12, 34},
            "The generated object should use common Transform authoring data.");

    // Backend-neutral robotics readiness fixture for Milestone 19: assembly,
    // typed connections, environment data, persistence, duplication, and undo.
    SceneDocument readiness;
    const auto chassis = readiness.CreateObject("CHASSIS", "fixture.chassis", {{0, 0}, 0});
    const auto leftWheel = readiness.CreateObject("LEFT WHEEL", "fixture.wheel", {{-1, 0}, 0});
    const auto rightWheel = readiness.CreateObject("RIGHT WHEEL", "fixture.wheel", {{1, 0}, 0});
    const auto distanceSensor = readiness.CreateObject("DISTANCE SENSOR", "fixture.distance-sensor", {{0, -1}, 0});
    const auto light = readiness.CreateObject("LIGHT", "fixture.light", {{0, 1}, 0});
    const auto obstacle = readiness.CreateObject("OBSTACLE", "fixture.obstacle", {{8, 0}, 0});
    for (const auto child : {leftWheel, rightWheel, distanceSensor, light})
        Require(readiness.SetParent(child, chassis), "Robot parts should attach to the chassis hierarchy.");
    Require(readiness.AddComponent(chassis, {"pipeframe.physics-body2d", 1, {{"mass", 2.0}}, true, false}),
            "The chassis should use the common physics component.");
    Require(readiness.AddComponent(distanceSensor, {"fixture.distance-sensor", 1,
                {{"range", 4.0}, {"distance", 0.0}}, true, false}),
            "The distance sensor should expose authored and telemetry-ready values.");
    Require(readiness.AddComponent(light, {"fixture.light", 1, {{"enabled", false}}, true, false}),
            "The light should expose a reusable component property.");
    Require(readiness.AddConnection({1, SceneConnectionKind::Mechanical,
                {chassis, "left-axle"}, {leftWheel, "hub"}}) &&
            readiness.AddConnection({2, SceneConnectionKind::Mechanical,
                {chassis, "right-axle"}, {rightWheel, "hub"}}) &&
            readiness.AddConnection({3, SceneConnectionKind::Signal,
                {distanceSensor, "distance"}, {light, "control"}}),
            "The readiness fixture should preserve mechanical and signal wiring.");
    const auto fixturePath = generatedRoot / "GeneratedProject/Scenes/RoboticsReadiness.pfscene";
    Require(SceneSerializer::Save(readiness, fixturePath, &createError),
            "The robotics readiness fixture should save through the standard scene serializer.");
    const auto reloadedReadiness = SceneSerializer::Load(fixturePath, &createError);
    Require(reloadedReadiness.has_value() &&
                reloadedReadiness->GetObjects().size() == 6 && reloadedReadiness->GetConnections().size() == 3 &&
                reloadedReadiness->FindObject(obstacle) != nullptr,
            "The robotics readiness fixture should reload with its environment and wiring intact.");
    std::filesystem::remove_all(generatedRoot, cleanupError);

    std::cout << "All component scene model tests passed.\n";
    return 0;
}

#include "../Editor/ProjectManager.h"
#include "../Editor/ProjectScaffolder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool Check(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

pipeframe::editor::SceneDocument CreateTestDocument() {
    pipeframe::editor::SceneDocument document;

    document.CreateObject("TEST AGENT", "basic.demo-agent", {{25.0f, 50.0f}, 15.0f});

    document.MarkClean();

    return document;
}

} // namespace

int main(int argc,char **argv) {
    namespace fs = std::filesystem;
    using namespace pipeframe::editor;

    if(argc==3 && std::string(argv[1])=="--emit-r6-fixture") {
        const fs::path root=argv[2]; std::string error;
        ProjectManager manager(root.parent_path()/"r6-recent-projects"); SceneDocument document;
        if(!manager.CreateProject(root,"Soldier Fixture",document,&error) ||
           !ProjectScaffolder::AddModule(root,GeneratedModuleKind::Component,"SoldierSettings",&error) ||
           !ProjectScaffolder::AddModule(root,GeneratedModuleKind::Behaviour,"SoldierBehaviour",&error) ||
           !ProjectScaffolder::AddModule(root,GeneratedModuleKind::Entity,"ProbeEntity",&error) ||
           !ProjectScaffolder::CreateObjectType(root,"SoldierAnt",{pipeframe::Transform2DComponentTypeId,"project.SoldierSettings","project.SoldierBehaviour"},&error)) {
            std::cerr<<error<<'\n';return 1;
        }
        return 0;
    }
    bool passed = true;

    const fs::path testRoot = fs::temp_directory_path() / "pipeframe_project_manager_tests";

    std::error_code cleanupError;
    fs::remove_all(testRoot, cleanupError);

    const fs::path recentFile = testRoot / ".recent-projects";

    const fs::path projectDirectory = testRoot / "Test Project";

    ProjectManager projectManager(recentFile);

    SceneDocument originalDocument = CreateTestDocument();

    std::string errorMessage;

    passed &= Check(projectManager.CreateProject(projectDirectory, "Test Project", originalDocument, &errorMessage),
                    "Project should be created. " + errorMessage);

    passed &= Check(fs::exists(projectDirectory / "project.pipeframe"), "Project manifest should exist.");

    passed &= Check(fs::exists(projectDirectory / "Scenes/Main.pfscene"), "Startup scene should exist.");
    passed &= Check(fs::exists(projectDirectory / "Assets/Textures") &&
                    fs::exists(projectDirectory / "Assets/Audio") &&
                    fs::exists(projectDirectory / "Assets/Materials") &&
                    fs::exists(projectDirectory / "Assets/Models") &&
                    fs::exists(projectDirectory / "Assets/Parts") &&
                    fs::exists(projectDirectory / "Assets/Prefabs") &&
                    fs::exists(projectDirectory / "Assets/Shaders") &&
                    fs::exists(projectDirectory / ".pipeframe/cache"),
                    "New projects should contain the standard source and imported-cache folders.");
    for (const char *path : {"CMakeLists.txt", "Config/ProjectSettings.pipeframe",
                             "Config/Input.pipeframe", "Config/Physics.pipeframe",
                             "Config/Modules.pfconfig", "Source/Components",
                             "Source/Systems", "Source/Integration", "Source/Editor",
                             "Source/Plugin.cpp", "Tests", "Source/Editor/ProjectPanel.h",
                             "Source/Integration/Test_ProjectAdapter.h",
                             "Source/World/Simulation/ProjectSimulationWorld.h",
                             "Source/Events/ProjectEvents.h"}) {
        passed &= Check(fs::exists(projectDirectory / path),
                        std::string("Generated project entry should exist: ") + path);
    }
    passed &= Check(!fs::exists(projectDirectory / "Source/Runtime"),
                    "New projects must not create the ambiguous outer Runtime folder.");
    {
        std::ifstream setup(projectDirectory / "Source/Integration/Test_ProjectProject.h");
        const std::string source((std::istreambuf_iterator<char>(setup)), {});
        passed &= Check(source.find("ProjectDefinition<ProjectWorld>") != std::string::npos &&
                        source.find("ConfigureWorld") != std::string::npos,
                        "New projects should expose one type and system setup entry point.");
    }
    for(const char *retired : {"Source/Components/ComponentContract.h", "Source/Systems/SystemContract.h", "Source/Editor/ExtensionContract.h", "Tests/ProjectTests.cpp"})
        passed &= Check(!fs::exists(projectDirectory / retired), "New projects must not contain placeholder-only contracts/tests");
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::Entity,
                                                  "RobotEntity", &errorMessage), "Generate entity composition recipe.");
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::Behaviour,
                                                  "RobotDriver", &errorMessage), "Generate attached lifecycle script.");
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::Component,
                                                  "DriveMotor", &errorMessage),
                    "Add Component should generate a public PipeFrame component. " + errorMessage);
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::System,
                                                  "DriveSystem", &errorMessage),
                    "Add System should generate a PipeFrame system. " + errorMessage);
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::EditorExtension,
                                                  "PathTool", &errorMessage),
                    "Add Editor Extension should generate a PipeFrame tool. " + errorMessage);
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::Runtime,
                                                  "TelemetryModule", &errorMessage),
                    "Add Runtime Module should generate a PipeFrame runtime module. " + errorMessage);
    {
        std::ifstream generated(projectDirectory / "Source/Components/DriveMotor.h");
        const std::string source((std::istreambuf_iterator<char>(generated)), {});
        passed &= Check(source.find("static auto BuildSchema()")!=std::string::npos &&
                        source.find("pipeframe::Component<DriveMotor>")!=std::string::npos &&
                        source.find("Define(\"project.DriveMotor\"")!=std::string::npos &&
                        source.find("PF_COMPONENT")==std::string::npos,
                        "Generated components must own their typed schema");
    }
    {
        std::ifstream modules(projectDirectory / "Config/Modules.pfconfig");
        const std::string registrations((std::istreambuf_iterator<char>(modules)), {});
        passed &= Check(registrations.find("Component DriveMotor") != std::string::npos &&
                        registrations.find("System DriveSystem") != std::string::npos &&
                        registrations.find("Runtime TelemetryModule") != std::string::npos &&
                        registrations.find("EditorExtension PathTool") != std::string::npos,
                        "Generators should update module registration metadata automatically.");
    }
    {
        std::ifstream generated(projectDirectory / "Source/Integration/GeneratedRegistration.h");
        const std::string source((std::istreambuf_iterator<char>(generated)), {});
        passed &= Check(source.find("RegisterComponent<DriveMotor>")!=std::string::npos &&
                        source.find("RegisterBehaviour<")!=std::string::npos &&
                        source.find("RegisterEntity<RobotEntity>")!=std::string::npos,
                        "Generated registration must connect typed components and behaviours to runtime");
    }
    passed &= Check(ProjectScaffolder::CreateObjectType(projectDirectory, "Robot",
                                                        {"pipeframe.transform2d", "project.DriveMotor"},
                                                        &errorMessage),
                    "Create Object Type should generate a data-only archetype. " + errorMessage);
    passed &= Check(ProjectScaffolder::Validate(projectDirectory).empty(),
                    "A newly generated project should pass structural and backend validation.");

    // Older generated projects must refresh registration in their existing folder.
    fs::rename(projectDirectory / "Source/Integration", projectDirectory / "Source/Runtime");
    passed &= Check(ProjectScaffolder::AddModule(projectDirectory, GeneratedModuleKind::Runtime,
                                                 "LegacyTelemetry", &errorMessage),
                    "Legacy projects should still generate integration modules.");
    passed &= Check(fs::exists(projectDirectory / "Source/Runtime/LegacyTelemetry.h") &&
                    fs::exists(projectDirectory / "Source/Runtime/GeneratedRegistration.h") &&
                    !fs::exists(projectDirectory / "Source/Integration"),
                    "Legacy generation must not split registration between two folders.");
    passed &= Check(ProjectScaffolder::Validate(projectDirectory).empty(),
                    "Legacy integration layout should remain valid.");
    fs::rename(projectDirectory / "Source/Runtime", projectDirectory / "Source/Integration");

    passed &= Check(projectManager.HasActiveProject(), "Created project should become active.");

    SceneDocument loadedDocument;

    errorMessage.clear();

    passed &= Check(projectManager.OpenProject(projectDirectory, loadedDocument, &errorMessage),
                    "Project directory should open. " + errorMessage);

    passed &= Check(loadedDocument.GetObjects().size() == 1, "Loaded scene should contain one object.");

    passed &= Check(!loadedDocument.IsDirty(), "Loaded scene should be clean.");

    loadedDocument.CreateObject("SECOND AGENT", "basic.demo-agent", {{100.0f, 200.0f}, 0.0f});

    errorMessage.clear();

    passed &= Check(projectManager.SaveActiveScene(loadedDocument, &errorMessage),
                    "Active scene should save. " + errorMessage);

    SceneDocument reloadedDocument;

    errorMessage.clear();

    passed &= Check(projectManager.OpenProject(projectDirectory / "project.pipeframe", reloadedDocument, &errorMessage),
                    "Manifest path should open. " + errorMessage);

    passed &= Check(reloadedDocument.GetObjects().size() == 2, "Saved scene should contain two objects.");

    const std::vector<fs::path> recentProjects = projectManager.GetRecentProjects();

    passed &= Check(!recentProjects.empty(), "Recent-project history should not be empty.");

    const fs::path availableDirectory = ProjectManager::FindAvailableProjectDirectory(testRoot, "Test Project");

    passed &= Check(availableDirectory.filename() == "Test Project 2",
                    "Available project directory should use a numeric suffix.");

    passed &= Check(projectManager.CreateScene("Maze", loadedDocument, &errorMessage), "Duplicate scene should be created");
    passed &= Check(projectManager.GetActiveScenePath().filename()=="Maze.pfscene", "Active scene changes independently");
    passed &= Check(projectManager.GetActiveManifest()->startupScene=="Scenes/Main.pfscene", "Startup scene remains unchanged");
    passed &= Check(!projectManager.CreateScene("Maze", loadedDocument, &errorMessage), "Existing scene is never overwritten");
    passed &= Check(!projectManager.CreateScene("../Escape", loadedDocument, &errorMessage), "Scene name cannot escape project");
    SceneDocument blank;
    passed &= Check(projectManager.CreateScene("Road", blank, &errorMessage), "Empty scene can be created");
    passed &= Check(projectManager.GetScenes().size()==3, "All project scenes listed");
    passed &= Check(projectManager.OpenScene("Scenes/Maze.pfscene", blank, &errorMessage) && blank.GetObjects().size()==2, "Duplicate round trips objects");
    blank.CreateObject("MAZE ONLY", "basic.demo-agent");
    passed &= Check(projectManager.SaveActiveScene(blank, &errorMessage), "Save targets active scene");
    passed &= Check(projectManager.OpenScene("Scenes/Main.pfscene", blank, &errorMessage) && blank.GetObjects().size()==2, "Saving another scene does not overwrite Main");
    passed &= Check(!projectManager.OpenScene("Scenes/Missing.pfscene", blank, &errorMessage) && blank.GetObjects().size()==2 && projectManager.GetActiveScenePath().filename()=="Main.pfscene", "Failed switch preserves current scene");
    {std::ofstream corrupt(projectDirectory/"Scenes/Corrupt.pfscene");corrupt<<"broken";}
    passed &= Check(!projectManager.OpenScene("Scenes/Corrupt.pfscene", blank, &errorMessage) && projectManager.GetActiveScenePath().filename()=="Main.pfscene", "Corrupt scene does not replace current scene");

    fs::remove_all(testRoot, cleanupError);

    if (!passed) {
        return 1;
    }

    std::cout << "All project manager tests passed.\n";
    return 0;
}

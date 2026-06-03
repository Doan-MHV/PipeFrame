#include "PipeFrameApplication.h"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>

#include "App/EditorSettings.h"
#include "App/LevelGenerator.h"
#include "App/ProjectGenerator.h"
#include "imgui.h"
#include "Logger/Logger.h"
#include "Project/ProjectConfig.h"
#include "Simulation/ProjectModule.h"

#ifndef PIPEFRAME_PROJECTS_ROOT
#define PIPEFRAME_PROJECTS_ROOT "projects"
#endif

#ifndef PIPEFRAME_TEMPLATE_PROJECT_ROOT
#define PIPEFRAME_TEMPLATE_PROJECT_ROOT "apps/PipeFrameEditor/templates"
#endif

#ifndef PIPEFRAME_SAMPLE_ASSET_ROOT
#define PIPEFRAME_SAMPLE_ASSET_ROOT "apps/PipeFrameEditor/templates/SampleAssets"
#endif

#ifndef PIPEFRAME_BUILD_DIR
#define PIPEFRAME_BUILD_DIR "cmake-build-debug"
#endif

namespace
{
constexpr int DefaultWindowWidth = 1600;
constexpr int DefaultWindowHeight = 900;

std::filesystem::path ResolveProjectFilePath(const std::filesystem::path& projectFilePath)
{
    if (projectFilePath.is_absolute() && std::filesystem::exists(projectFilePath))
    {
        return projectFilePath;
    }

    if (std::filesystem::exists(projectFilePath))
    {
        return std::filesystem::absolute(projectFilePath);
    }

    const std::filesystem::path projectsRoot = PIPEFRAME_PROJECTS_ROOT;
    const std::filesystem::path sourceRoot = projectsRoot.parent_path();
    const std::filesystem::path fromSourceRoot = sourceRoot / projectFilePath;
    if (std::filesystem::exists(fromSourceRoot))
    {
        return fromSourceRoot;
    }

    const std::filesystem::path fromProjectsRoot = projectsRoot / projectFilePath;
    if (std::filesystem::exists(fromProjectsRoot))
    {
        return fromProjectsRoot;
    }

    if (projectFilePath.filename() != "PipeFrameProject.json")
    {
        const std::filesystem::path projectConfigFromProjectsRoot =
            projectsRoot / projectFilePath / "PipeFrameProject.json";
        if (std::filesystem::exists(projectConfigFromProjectsRoot))
        {
            return projectConfigFromProjectsRoot;
        }
    }

    return sourceRoot / projectFilePath;
}

std::string QuoteShellPath(const std::filesystem::path& path)
{
    std::string value = path.string();
    std::string quoted = "'";

    for (char ch : value)
    {
        if (ch == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += ch;
        }
    }

    quoted += "'";
    return quoted;
}

int RunCommandWithOutput(const std::string& command, std::string& output)
{
    std::array<char, 256> buffer{};
    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        output = "Failed to start command";
        return -1;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

    return pclose(pipe);
}

void LogCommandOutput(const std::string& output)
{
    if (output.empty())
    {
        return;
    }

    std::istringstream stream(output);
    std::string line;
    int loggedLines = 0;
    while (std::getline(stream, line) && loggedLines < 16)
    {
        if (!line.empty())
        {
            Logger::Log(line);
            loggedLines++;
        }
    }

    if (stream.good())
    {
        Logger::Log("Build output truncated. Check terminal for the full CMake output.");
    }
}

std::string ToCppIdentifier(const std::string& value)
{
    std::string result;

    for (char ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
        {
            result += ch;
        }
    }

    if (result.empty())
    {
        return "";
    }

    if (std::isdigit(static_cast<unsigned char>(result.front())))
    {
        result.insert(result.begin(), '_');
    }

    return result;
}

bool WriteGeneratedFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());

    if (std::filesystem::exists(path))
    {
        Logger::Err("Cannot create C++ class: file already exists: " + path.string());
        return false;
    }

    std::ofstream output(path);
    if (!output)
    {
        Logger::Err("Cannot create C++ class: failed to write " + path.string());
        return false;
    }

    output << content;
    return true;
}

bool AppendTextToFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::app);
    if (!output)
    {
        Logger::Err("Cannot update module file: " + path.string());
        return false;
    }

    output << content;
    return true;
}

bool ReplaceTextInFile(
    const std::filesystem::path& path,
    const std::string& search,
    const std::string& replacement
)
{
    std::ifstream input(path);
    if (!input)
    {
        Logger::Err("Cannot read module file: " + path.string());
        return false;
    }

    std::string text{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    const std::size_t position = text.find(search);
    if (position == std::string::npos)
    {
        Logger::Warn("Module update marker not found in: " + path.string());
        return false;
    }

    text.replace(position, search.size(), replacement);

    std::ofstream output(path);
    if (!output)
    {
        Logger::Err("Cannot write module file: " + path.string());
        return false;
    }

    output << text;
    return true;
}

bool InsertTextBefore(
    const std::filesystem::path& path,
    const std::string& search,
    const std::string& insertion
)
{
    return ReplaceTextInFile(path, search, insertion + search);
}

bool InsertIncludeIfMissing(const std::filesystem::path& path, const std::string& includeLine)
{
    std::ifstream input(path);
    if (!input)
    {
        Logger::Err("Cannot read module file: " + path.string());
        return false;
    }

    std::string text{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    if (text.find(includeLine) != std::string::npos)
    {
        return true;
    }

    const std::size_t firstBlankAfterIncludes = text.find("\n\n");
    if (firstBlankAfterIncludes == std::string::npos)
    {
        return AppendTextToFile(path, "\n" + includeLine + "\n");
    }

    text.insert(firstBlankAfterIncludes, "\n" + includeLine);

    std::ofstream output(path);
    if (!output)
    {
        Logger::Err("Cannot write module file: " + path.string());
        return false;
    }

    output << text;
    return true;
}

std::string GetProjectModuleClassName(const ProjectConfig& projectConfig)
{
    if (projectConfig.name == "AntSimulationDemo")
    {
        return "AntSimulationModule";
    }

    return ToCppIdentifier(projectConfig.projectRoot.filename().string()) + "Module";
}

std::filesystem::path GetProjectModuleHeaderPath(const ProjectConfig& projectConfig)
{
    return projectConfig.projectRoot / "Source" / (GetProjectModuleClassName(projectConfig) + ".h");
}

std::filesystem::path GetProjectModuleSourcePath(const ProjectConfig& projectConfig)
{
    return projectConfig.projectRoot / "Source" / (GetProjectModuleClassName(projectConfig) + ".cpp");
}

void UpdateProjectModuleForGeneratedClass(
    const ProjectConfig& projectConfig,
    CppClassKind kind,
    const std::string& className
)
{
    const std::filesystem::path moduleHeaderPath = GetProjectModuleHeaderPath(projectConfig);
    const std::filesystem::path moduleSourcePath = GetProjectModuleSourcePath(projectConfig);
    const std::string moduleClassName = GetProjectModuleClassName(projectConfig);

    switch (kind)
    {
    case CppClassKind::Component:
        (void)moduleClassName;
        Logger::Log("Component metadata will be generated by PipeFrameHeaderTool.");
        break;
    case CppClassKind::ProjectSystem:
        InsertIncludeIfMissing(moduleHeaderPath, "#include \"Systems/" + className + ".h\"");
        ReplaceTextInFile(
            moduleHeaderPath,
            "private:\n",
            "private:\n    " + className + " " + className + "Instance;\n"
        );
        InsertTextBefore(
            moduleSourcePath,
            "\n}\n\nvoid " + moduleClassName + "::Stop",
            "    " + className + "Instance.Update(context);\n"
        );
        break;
    case CppClassKind::EntitySystem:
        InsertIncludeIfMissing(moduleSourcePath, "#include \"Systems/" + className + ".h\"");
        InsertTextBefore(
            moduleSourcePath,
            "\n}\n\nvoid " + moduleClassName + "::Loaded",
            "    registry.AddSystem<" + className + ">();\n"
        );
        break;
    case CppClassKind::Event:
        (void)moduleClassName;
        Logger::Log("Event class created. Include it from systems that emit or listen to it.");
        break;
    case CppClassKind::EntityClass:
        InsertIncludeIfMissing(moduleSourcePath, "#include \"Entity/" + className + ".h\"");
        InsertTextBefore(
            moduleSourcePath,
            "\n}\n\nvoid " + moduleClassName + "::RegisterEntitySystems",
            "    registry.RegisterEntityClass({\n"
            "        .typeName = \"" + className + "\",\n"
            "        .displayName = \"" + className + "\",\n"
            "        .category = \"Project\",\n"
            "        .create = " + className + "::Create\n"
            "    });\n"
        );
        break;
    case CppClassKind::DenseAgentSimulation:
        InsertIncludeIfMissing(moduleHeaderPath, "#include \"Simulations/" + className + ".h\"");
        ReplaceTextInFile(
            moduleHeaderPath,
            "private:\n",
            "private:\n    " + className + " " + className + "Instance;\n"
        );
        InsertTextBefore(
            moduleSourcePath,
            "\n}\n\nvoid " + moduleClassName + "::Update",
            "    " + className + "Instance.Start(context);\n"
        );
        InsertTextBefore(
            moduleSourcePath,
            "\n}\n\nvoid " + moduleClassName + "::Stop",
            "    " + className + "Instance.Update(context);\n"
        );
        if (!InsertTextBefore(
                moduleSourcePath,
                "\n}\n\nconst std::unordered_map",
                "    " + className + "Instance.Render(renderer, assetRegistry, camera);\n"
            ))
        {
            InsertTextBefore(
                moduleSourcePath,
                "\n}\n\nextern \"C\" ProjectModule* CreateProjectModule",
                "    " + className + "Instance.Render(renderer, assetRegistry, camera);\n"
            );
        }
        break;
    case CppClassKind::PhysicsScenario:
        InsertIncludeIfMissing(moduleSourcePath, "#include \"PhysicsScenarios/" + className + ".h\"");
        break;
    }
}

std::string BuildComponentTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include \"Reflection/ComponentAnnotations.h\"\n\n"
           "PF_COMPONENT()\n"
           "struct " + className + "\n"
           "{\n"
           "    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)\n"
           "    int value = 0;\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildProjectSystemTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include \"Simulation/ProjectRuntime.h\"\n\n"
           "class " + className + " : public ProjectSystem\n"
           "{\n"
           "public:\n"
           "    void Start(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n\n"
           "    void Update(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildEntitySystemTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include \"Components/TransformComponent.h\"\n"
           "#include \"ECS/ECS.h\"\n\n"
           "class " + className + " : public EntitySystem\n"
           "{\n"
           "public:\n"
           "    void Loaded() override\n"
           "    {\n"
           "        RequireComponent<TransformComponent>();\n"
           "    }\n\n"
           "    void Start(EntitySystemContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n\n"
           "    void SubscribeToEvents(EntitySystemContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "        // Listen<MyEvent>(context, &" + className + "::OnMyEvent);\n"
           "    }\n\n"
           "    void Update(EntitySystemContext& context) override\n"
           "    {\n"
           "        for (Entity entity : GetSystemEntities())\n"
           "        {\n"
           "            auto& transform = entity.GetComponent<TransformComponent>();\n"
           "            transform.position.x += 0.0f * static_cast<float>(context.deltaTime);\n"
           "        }\n"
           "    }\n\n"
           "    void Stop(EntitySystemContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildEventTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include \"ECS/Entity.h\"\n"
           "#include \"EventBus/Event.h\"\n\n"
           "struct " + className + " : public Event\n"
           "{\n"
           "    Entity entity{-1};\n\n"
           "    " + className + "() = default;\n\n"
           "    explicit " + className + "(Entity entity)\n"
           "        : entity(entity)\n"
           "    {\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildEntityClassTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include <glm/glm.hpp>\n\n"
           "#include \"Components/TransformComponent.h\"\n"
           "#include \"Components/EditorEntityComponent.h\"\n"
           "#include \"Components/PersistentIdComponent.h\"\n"
           "#include \"ECS/ECS.h\"\n"
           "#include \"Project/EntityIdGenerator.h\"\n\n"
           "struct " + className + "\n"
           "{\n"
           "    static Entity Create(Registry& registry, glm::vec2 position)\n"
           "    {\n"
           "        Entity entity = registry.CreateEntity();\n"
           "        entity.AddComponent<EditorEntityComponent>();\n"
           "        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, \"" + className + "\"));\n"
           "        entity.AddComponent<TransformComponent>(position);\n"
           "        return entity;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildDenseSimulationTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include <SDL3/SDL.h>\n\n"
           "#include \"Assets/AssetRegistry.h\"\n"
           "#include \"Simulation/DenseAgentSimulation.h\"\n"
           "#include \"Simulation/ProjectRuntime.h\"\n\n"
           "struct " + className + "Agent\n"
           "{\n"
           "    float x = 0.0f;\n"
           "    float y = 0.0f;\n"
           "    float vx = 0.0f;\n"
           "    float vy = 0.0f;\n"
           "};\n\n"
           "class " + className + " : public DenseAgentSimulation<" + className + "Agent>, public ProjectSimulation\n"
           "{\n"
           "public:\n"
           "    void Start(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "        Clear();\n"
           "    }\n\n"
           "    void Update(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        const float deltaTime = static_cast<float>(context.deltaTime);\n"
           "        for (auto& agent : Agents())\n"
           "        {\n"
           "            agent.x += agent.vx * deltaTime;\n"
           "            agent.y += agent.vy * deltaTime;\n"
           "        }\n"
           "    }\n\n"
           "    void Render(SDL_Renderer* renderer, AssetRegistry& assetRegistry, const SDL_FRect& camera) override\n"
           "    {\n"
           "        (void)renderer;\n"
           "        (void)assetRegistry;\n"
           "        (void)camera;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}

std::string BuildPhysicsScenarioTemplate(const std::string& className)
{
    return "#ifndef " + className + "_H\n"
           "#define " + className + "_H\n\n"
           "#include \"Simulation/ProjectRuntime.h\"\n\n"
           "class " + className + " : public ProjectSystem\n"
           "{\n"
           "public:\n"
           "    void Update(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + className + "_H\n";
}
}

PipeFrameApplication::PipeFrameApplication(const std::filesystem::path& projectFilePath)
    : game(LoadProjectConfig(projectFilePath))
{
    InstallProjectModule(game.GetProjectConfig());
    SaveLastProjectFile(game.GetProjectConfig().projectFilePath);
}

bool PipeFrameApplication::Initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Logger::Err("Error initializing SDL.");
        return false;
    }

    if (!TTF_Init())
    {
        Logger::Err("Error initializing SDL_ttf.");
        return false;
    }

    window = SDL_CreateWindow(
        "PipeFrame",
        DefaultWindowWidth,
        DefaultWindowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        Logger::Err("Error creating SDL window.");
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer)
    {
        Logger::Err("Error creating SDL renderer.");
        return false;
    }

    if (!imguiLayer.Initialize(window, renderer))
    {
        Logger::Err("Error initializing ImGui.");
        return false;
    }

    game.Initialize(renderer);
    game.Setup();

    isRunning = true;
    return true;
}

void PipeFrameApplication::ProcessInput()
{
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        imguiLayer.ProcessEvent(sdlEvent);
        game.HandleEvent(sdlEvent, isRunning);
    }
}

void PipeFrameApplication::Render()
{
    const ImVec2 viewportSize = editor.GetViewportSize();

    game.RenderSceneToViewport(
        static_cast<int>(viewportSize.x),
        static_cast<int>(viewportSize.y)
    );

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
    SDL_RenderClear(renderer);

    imguiLayer.BeginFrame();

    const EditorToolbarResult editorResult = editor.Update(
        game.GetMode(),
        game.GetPlaySpeed(),
        game.GetViewportTexture(),
        game.GetRegistry(),
        game.GetProjectConfig(),
        game.GetComponentRegistry(),
        game.GetCurrentLevelFilePaths(),
        game.GetAssetRegistry(),
        game.GetFieldGrids(),
        game.GetPrefabRegistry(),
        game.GetClassRegistry(),
        game.GetProjectModule(),
        game.GetTileMap(),
        game.GetTilePaletteTexture(),
        game.GetCamera()
    );

    if (editorResult.requestedModeToggle)
    {
        game.ToggleMode();
    }

    if (editorResult.requestedPlaySpeedChange)
    {
        game.SetPlaySpeed(editorResult.playSpeed);
    }

    if (editorResult.requestedProjectCreate)
    {
        CreateNewProject(
            editorResult.projectName,
            editorResult.projectParentDirectory,
            editorResult.copySampleAntAssets
        );
    }

    if (editorResult.requestedProjectOpen)
    {
        OpenProject(editorResult.projectFilePath);
    }

    if (editorResult.requestedCppCompile)
    {
        CompileCppProject();
    }

    if (editorResult.requestedCppClassCreate)
    {
        CreateCppClass(editorResult.cppClassKind, editorResult.cppClassName);
    }

    if (editorResult.requestedLevelCreate)
    {
        CreateNewLevel(
            editorResult.levelName,
            editorResult.levelRows,
            editorResult.levelCols,
            editorResult.levelTileSize,
            editorResult.levelScale
        );
    }

    if (editorResult.requestedPrefabSave)
    {
        SaveEntityAsPrefab(editorResult.prefabSourceEntityId, editorResult.prefabName);
    }

    if (editorResult.requestedTextureImport)
    {
        ImportTextureAsset(editorResult);
    }

    imguiLayer.EndFrame(renderer);

    SDL_RenderPresent(renderer);
}

void PipeFrameApplication::InstallProjectModule(
    const ProjectConfig& projectConfig,
    bool restoreCurrentWorld
)
{
    UnloadProjectModule();

    const std::filesystem::path libraryPath = GetProjectModuleLibraryPath(projectConfig);
    if (libraryPath.empty())
    {
        return;
    }

    if (!std::filesystem::exists(libraryPath))
    {
        Logger::Warn("Project module dylib not found: " + libraryPath.string());
        game.SetProjectModule(nullptr);
        return;
    }

    const std::filesystem::path loadPath = CreateProjectModuleLiveLibraryCopy(projectConfig, libraryPath);
    if (loadPath.empty())
    {
        game.SetProjectModule(nullptr);
        return;
    }

    loadedProjectModuleLibraryPath = loadPath;
    loadedProjectModuleIsLiveCopy = loadPath != libraryPath;

    projectModuleLibraryHandle = dlopen(loadPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!projectModuleLibraryHandle)
    {
        Logger::Err("Failed to load project module: " + std::string(dlerror()));
        CleanupLoadedProjectModuleCopy();
        game.SetProjectModule(nullptr);
        return;
    }

    auto createProjectModule = reinterpret_cast<CreateProjectModuleFn>(
        dlsym(projectModuleLibraryHandle, "CreateProjectModule")
    );
    auto destroyProjectModule = reinterpret_cast<DestroyProjectModuleFn>(
        dlsym(projectModuleLibraryHandle, "DestroyProjectModule")
    );

    if (!createProjectModule || !destroyProjectModule)
    {
        Logger::Err("Project module is missing CreateProjectModule/DestroyProjectModule exports");
        dlclose(projectModuleLibraryHandle);
        projectModuleLibraryHandle = nullptr;
        CleanupLoadedProjectModuleCopy();
        game.SetProjectModule(nullptr);
        return;
    }

    ProjectModule* module = createProjectModule();
    if (!module)
    {
        Logger::Err("Project module factory returned null");
        dlclose(projectModuleLibraryHandle);
        projectModuleLibraryHandle = nullptr;
        CleanupLoadedProjectModuleCopy();
        game.SetProjectModule(nullptr);
        return;
    }

    game.SetProjectModule(std::shared_ptr<ProjectModule>(
        module,
        [destroyProjectModule](ProjectModule* projectModule)
        {
            destroyProjectModule(projectModule);
        }
    ));
    if (restoreCurrentWorld)
    {
        game.RestoreWorldAfterProjectModuleReload();
    }
    Logger::Log("Loaded project module: " + loadPath.string());
}

void PipeFrameApplication::UnloadProjectModule(bool preserveCurrentWorld)
{
    if (projectModuleLibraryHandle)
    {
        game.PrepareForProjectModuleUnload(preserveCurrentWorld);
    }

    game.SetProjectModule(nullptr);

    if (projectModuleLibraryHandle)
    {
        dlclose(projectModuleLibraryHandle);
        projectModuleLibraryHandle = nullptr;
    }

    CleanupLoadedProjectModuleCopy();
}

std::filesystem::path PipeFrameApplication::GetProjectModuleLibraryPath(
    const ProjectConfig& projectConfig
) const
{
    if (projectConfig.name.empty())
    {
        return {};
    }

    const std::string targetName = GetProjectModuleTargetName(projectConfig);
    if (targetName.empty())
    {
        return {};
    }

    return std::filesystem::path(PIPEFRAME_BUILD_DIR) /
        "projects" /
        projectConfig.projectRoot.filename() /
        "Source" /
        ("lib" + targetName + ".dylib");
}

std::filesystem::path PipeFrameApplication::CreateProjectModuleLiveLibraryCopy(
    const ProjectConfig& projectConfig,
    const std::filesystem::path& libraryPath
)
{
    if (libraryPath.empty())
    {
        return {};
    }

    std::error_code error;
    const std::filesystem::path liveDirectory = libraryPath.parent_path() / "LiveModules";
    std::filesystem::create_directories(liveDirectory, error);
    if (error)
    {
        Logger::Err("Cannot create live module directory: " + liveDirectory.string());
        return {};
    }

    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string targetName = GetProjectModuleTargetName(projectConfig);
    const std::string liveFileName =
        "lib" + targetName +
        ".live." + std::to_string(++projectModuleLiveReloadCounter) +
        "." + std::to_string(now) +
        libraryPath.extension().string();
    const std::filesystem::path livePath = liveDirectory / liveFileName;

    std::filesystem::copy_file(
        libraryPath,
        livePath,
        std::filesystem::copy_options::overwrite_existing,
        error
    );

    if (error)
    {
        Logger::Err(
            "Cannot create live module copy: " + libraryPath.string() +
            " -> " + livePath.string() +
            " (" + error.message() + ")"
        );
        return {};
    }

    return livePath;
}

void PipeFrameApplication::CleanupLoadedProjectModuleCopy()
{
    if (!loadedProjectModuleIsLiveCopy || loadedProjectModuleLibraryPath.empty())
    {
        loadedProjectModuleLibraryPath.clear();
        loadedProjectModuleIsLiveCopy = false;
        return;
    }

    std::error_code error;
    std::filesystem::remove(loadedProjectModuleLibraryPath, error);
    if (error)
    {
        Logger::Warn("Could not remove old live module copy: " + loadedProjectModuleLibraryPath.string());
    }

    loadedProjectModuleLibraryPath.clear();
    loadedProjectModuleIsLiveCopy = false;
}

std::string PipeFrameApplication::GetProjectModuleTargetName(const ProjectConfig& projectConfig) const
{
    if (projectConfig.name == "AntSimulationDemo")
    {
        return "PipeFrameAntSimulation";
    }

    const std::string identifier = ToCppIdentifier(projectConfig.projectRoot.filename().string());
    return identifier.empty() ? "" : "PipeFrame" + identifier;
}

void PipeFrameApplication::Run()
{
    while (isRunning)
    {
        ProcessInput();
        game.Update();
        Render();
    }
}

void PipeFrameApplication::CreateNewProject(
    const std::string& projectName,
    const std::string& parentDirectory,
    bool copySampleAntAssets
)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot create a new project while Play mode is running");
        return;
    }

    ProjectGeneratorOptions options;
    options.projectName = projectName;
    options.parentDirectory = parentDirectory;
    options.defaultProjectsRoot = PIPEFRAME_PROJECTS_ROOT;
    options.templateProjectRoot = PIPEFRAME_TEMPLATE_PROJECT_ROOT;
    options.sampleAssetRoot = PIPEFRAME_SAMPLE_ASSET_ROOT;
    options.copySampleAntAssets = copySampleAntAssets;

    std::filesystem::path projectFilePath;
    std::string error;

    if (!GeneratePipeFrameProject(options, projectFilePath, error))
    {
        Logger::Err("Failed to create project: " + error);
        return;
    }

    Logger::Log("Created project: " + projectFilePath.string());

    ProjectConfig newProjectConfig = LoadProjectConfig(projectFilePath);
    InstallProjectModule(newProjectConfig);

    if (!game.LoadProject(std::move(newProjectConfig)))
    {
        Logger::Err("Project was created, but loading it failed: " + projectFilePath.string());
        return;
    }

    SaveLastProjectFile(projectFilePath);
}

void PipeFrameApplication::OpenProject(const std::filesystem::path& projectFilePath)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot open project while Play mode is running");
        return;
    }

    if (projectFilePath.empty())
    {
        Logger::Err("Cannot open project: path is empty");
        return;
    }

    const std::filesystem::path resolvedProjectFilePath = ResolveProjectFilePath(projectFilePath);
    if (!std::filesystem::exists(resolvedProjectFilePath))
    {
        Logger::Err("Cannot open project: file does not exist: " + resolvedProjectFilePath.string());
        return;
    }

    ProjectConfig newProjectConfig = LoadProjectConfig(resolvedProjectFilePath);
    InstallProjectModule(newProjectConfig);

    if (!game.LoadProject(std::move(newProjectConfig)))
    {
        Logger::Err("Failed to open project: " + resolvedProjectFilePath.string());
        return;
    }

    SaveLastProjectFile(resolvedProjectFilePath);
    Logger::Log("Opened project: " + resolvedProjectFilePath.string());
}

void PipeFrameApplication::CompileCppProject()
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot compile C++ while Play mode is running");
        return;
    }

    const std::filesystem::path buildDirectory = PIPEFRAME_BUILD_DIR;
    if (!std::filesystem::exists(buildDirectory))
    {
        Logger::Err("Cannot compile C++: build directory does not exist: " + buildDirectory.string());
        return;
    }

    const std::filesystem::path sourceDirectory = std::filesystem::path(PIPEFRAME_PROJECTS_ROOT).parent_path();
    const std::string configureCommand =
        "cmake -S " + QuoteShellPath(sourceDirectory) +
        " -B " + QuoteShellPath(buildDirectory) + " 2>&1";
    const std::string buildCommand =
        "cmake --build " + QuoteShellPath(buildDirectory) +
        " --target " + GetProjectModuleTargetName(game.GetProjectConfig()) + " -j 10 2>&1";

    Logger::Log("Configuring C++ project module...");
    std::string output;
    const int configureExitCode = RunCommandWithOutput(configureCommand, output);
    LogCommandOutput(output);

    if (configureExitCode != 0)
    {
        Logger::Err("Configure failed. Keeping the current project module loaded.");
        return;
    }

    Logger::Log("Compiling C++ project module...");
    output.clear();
    const int buildExitCode = RunCommandWithOutput(buildCommand, output);
    LogCommandOutput(output);

    if (buildExitCode == 0)
    {
        InstallProjectModule(game.GetProjectConfig(), true);
        Logger::Log("Compile succeeded. Project module reloaded.");
    }
    else
    {
        Logger::Err("Compile failed. Keeping the current project module loaded.");
    }
}

void PipeFrameApplication::CreateCppClass(CppClassKind kind, const std::string& className)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot create C++ class while Play mode is running");
        return;
    }

    const std::string identifier = ToCppIdentifier(className);
    if (identifier.empty())
    {
        Logger::Err("Cannot create C++ class: class name is empty");
        return;
    }

    const std::filesystem::path sourceRoot = game.GetProjectConfig().projectRoot / "Source";
    std::filesystem::path outputPath;
    std::string content;

    switch (kind)
    {
        case CppClassKind::Component:
            outputPath = sourceRoot / "Components" / (identifier + ".h");
            content = BuildComponentTemplate(identifier);
            break;
        case CppClassKind::ProjectSystem:
            outputPath = sourceRoot / "Systems" / (identifier + ".h");
            content = BuildProjectSystemTemplate(identifier);
            break;
        case CppClassKind::EntitySystem:
            outputPath = sourceRoot / "Systems" / (identifier + ".h");
            content = BuildEntitySystemTemplate(identifier);
            break;
        case CppClassKind::Event:
            outputPath = sourceRoot / "Events" / (identifier + ".h");
            content = BuildEventTemplate(identifier);
            break;
        case CppClassKind::EntityClass:
            outputPath = sourceRoot / "Entity" / (identifier + ".h");
            content = BuildEntityClassTemplate(identifier);
            break;
        case CppClassKind::DenseAgentSimulation:
            outputPath = sourceRoot / "Simulations" / (identifier + ".h");
            content = BuildDenseSimulationTemplate(identifier);
            break;
        case CppClassKind::PhysicsScenario:
            outputPath = sourceRoot / "PhysicsScenarios" / (identifier + ".h");
            content = BuildPhysicsScenarioTemplate(identifier);
            break;
    }

    if (!WriteGeneratedFile(outputPath, content))
    {
        return;
    }

    UpdateProjectModuleForGeneratedClass(game.GetProjectConfig(), kind, identifier);
    Logger::Log("Created C++ class: " + outputPath.string());
    Logger::Log("Updated project module registration. Press Compile C++ to build and hot reload.");
}

void PipeFrameApplication::CreateNewLevel(
    const std::string& levelName,
    int rows,
    int cols,
    int tileSize,
    float scale
)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot create a new level while Play mode is running");
        return;
    }

    LevelGeneratorOptions options;
    options.projectConfig = game.GetProjectConfig();
    options.levelName = levelName;
    options.rows = rows;
    options.cols = cols;
    options.tileSize = tileSize;
    options.scale = scale;

    if (const TileMap* tileMap = game.GetTileMap())
    {
        options.tilemapTextureAssetId = tileMap->GetTextureAssetId();
    }

    std::filesystem::path levelFilePath;
    std::string error;

    if (!GeneratePipeFrameLevel(options, levelFilePath, error))
    {
        Logger::Err("Failed to create level: " + error);
        return;
    }

    Logger::Log("Created level: " + levelFilePath.string());

    if (!game.LoadLevel(levelFilePath))
    {
        Logger::Err("Level was created, but loading it failed: " + levelFilePath.string());
    }
}

void PipeFrameApplication::SaveEntityAsPrefab(int entityId, const std::string& prefabName)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot save prefab while Play mode is running");
        return;
    }

    game.SaveEntityAsPrefab(entityId, prefabName);
}

void PipeFrameApplication::ImportTextureAsset(const EditorToolbarResult& editorResult)
{
    if (game.GetMode() != EngineMode::Edit)
    {
        Logger::Err("Cannot import texture while Play mode is running");
        return;
    }

    TextureAssetImportOptions options;
    options.assetId = editorResult.textureAssetId;
    options.sourceFilePath = editorResult.textureSourceFilePath;
    options.sprite.mode = editorResult.textureImportMode == TextureImportMode::SpriteSheet
        ? TextureSpriteMode::SpriteSheet
        : TextureSpriteMode::SingleImage;
    options.sprite.defaultDisplayWidth = editorResult.textureDisplayWidth;
    options.sprite.defaultDisplayHeight = editorResult.textureDisplayHeight;
    options.sprite.frameWidth = editorResult.textureFrameWidth;
    options.sprite.frameHeight = editorResult.textureFrameHeight;
    options.sprite.defaultFrame = editorResult.textureDefaultFrame;

    game.ImportTextureAsset(options);
}

void PipeFrameApplication::Shutdown()
{
    UnloadProjectModule();
    game.Shutdown();
    imguiLayer.Shutdown();

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    TTF_Quit();
    SDL_Quit();
}

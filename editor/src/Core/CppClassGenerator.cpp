#include "Core/CppClassGenerator.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
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

    if (!result.empty() && std::isdigit(static_cast<unsigned char>(result.front())))
    {
        result.insert(result.begin(), '_');
    }

    return result;
}

std::string ToIncludeGuard(const std::string& className)
{
    std::string result = "PIPEFRAME_PROJECT_" + className + "_H";
    for (char& ch : result)
    {
        if (!std::isalnum(static_cast<unsigned char>(ch)))
        {
            ch = '_';
        }
        else
        {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
    }
    return result;
}

bool WriteNewFile(const std::filesystem::path& path, const std::string& content, std::string& outMessage)
{
    std::filesystem::create_directories(path.parent_path());
    if (std::filesystem::exists(path))
    {
        outMessage = "File already exists: " + path.string();
        return false;
    }

    std::ofstream output(path);
    if (!output)
    {
        outMessage = "Failed to write file: " + path.string();
        return false;
    }

    output << content;
    return true;
}

bool ReadFile(const std::filesystem::path& path, std::string& text, std::string& outMessage)
{
    std::ifstream input(path);
    if (!input)
    {
        outMessage = "Cannot read module file: " + path.string();
        return false;
    }

    text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return true;
}

bool WriteFile(const std::filesystem::path& path, const std::string& text, std::string& outMessage)
{
    std::ofstream output(path);
    if (!output)
    {
        outMessage = "Cannot write module file: " + path.string();
        return false;
    }

    output << text;
    return true;
}

bool InsertIncludeIfMissing(const std::filesystem::path& path, const std::string& includeLine, std::string& outMessage)
{
    std::string text;
    if (!ReadFile(path, text, outMessage))
    {
        return false;
    }

    if (text.find(includeLine) != std::string::npos)
    {
        return true;
    }

    const std::size_t position = text.find("\n\n");
    if (position == std::string::npos)
    {
        text += "\n" + includeLine + "\n";
    }
    else
    {
        text.insert(position, "\n" + includeLine);
    }

    return WriteFile(path, text, outMessage);
}

bool InsertBefore(
    const std::filesystem::path& path,
    const std::string& marker,
    const std::string& insertion,
    std::string& outMessage
)
{
    std::string text;
    if (!ReadFile(path, text, outMessage))
    {
        return false;
    }

    const std::size_t position = text.find(marker);
    if (position == std::string::npos)
    {
        outMessage = "Module update marker not found: " + marker;
        return false;
    }

    text.insert(position, insertion);
    return WriteFile(path, text, outMessage);
}

bool InsertAfter(
    const std::filesystem::path& path,
    const std::string& marker,
    const std::string& insertion,
    std::string& outMessage
)
{
    std::string text;
    if (!ReadFile(path, text, outMessage))
    {
        return false;
    }

    const std::size_t position = text.find(marker);
    if (position == std::string::npos)
    {
        outMessage = "Module update marker not found: " + marker;
        return false;
    }

    text.insert(position + marker.size(), insertion);
    return WriteFile(path, text, outMessage);
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

std::string BuildComponentTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
           "#include \"Reflection/ComponentAnnotations.h\"\n\n"
           "PF_COMPONENT()\n"
           "struct " + className + "\n"
           "{\n"
           "    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)\n"
           "    int value = 0;\n"
           "};\n\n"
           "#endif // " + guard + "\n";
}

std::string BuildProjectSystemTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
           "#include \"Simulation/ProjectRuntime.h\"\n\n"
           "class " + className + " : public ProjectSystem\n"
           "{\n"
           "public:\n"
           "    void Loaded(ProjectRuntimeContext& context) override { (void)context; }\n"
           "    void Start(ProjectRuntimeContext& context) override { (void)context; }\n"
           "    void Update(ProjectRuntimeContext& context) override { (void)context; }\n"
           "    void Stop(ProjectRuntimeContext& context) override { (void)context; }\n"
           "    void Unloaded(ProjectRuntimeContext& context) override { (void)context; }\n"
           "};\n\n"
           "#endif // " + guard + "\n";
}

std::string BuildEntitySystemTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
           "#include \"Components/TransformComponent.h\"\n"
           "#include \"ECS/ECS.h\"\n\n"
           "class " + className + " : public EntitySystem\n"
           "{\n"
           "public:\n"
           "    void Loaded() override\n"
           "    {\n"
           "        RequireComponent<TransformComponent>();\n"
           "    }\n\n"
           "    void Start(EntitySystemContext& context) override { (void)context; }\n"
           "    void SubscribeToEvents(EntitySystemContext& context) override { (void)context; }\n"
           "    void Update(EntitySystemContext& context) override { (void)context; }\n"
           "    void Stop(EntitySystemContext& context) override { (void)context; }\n"
           "};\n\n"
           "#endif // " + guard + "\n";
}

std::string BuildEventTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
           "#include \"ECS/Entity.h\"\n"
           "#include \"EventBus/Event.h\"\n\n"
           "struct " + className + " : public Event\n"
           "{\n"
           "    Entity entity{-1};\n"
           "};\n\n"
           "#endif // " + guard + "\n";
}

std::string BuildEntityClassTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
           "#include <glm/glm.hpp>\n\n"
           "#include \"Components/EditorEntityComponent.h\"\n"
           "#include \"Components/PersistentIdComponent.h\"\n"
           "#include \"Components/TransformComponent.h\"\n"
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
           "#endif // " + guard + "\n";
}

std::string BuildDenseSimulationTemplate(const std::string& className)
{
    const std::string guard = ToIncludeGuard(className);
    return "#ifndef " + guard + "\n"
           "#define " + guard + "\n\n"
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
           "    void Start(ProjectRuntimeContext& context) override { (void)context; Clear(); }\n"
           "    void Update(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        const float deltaTime = static_cast<float>(context.deltaTime);\n"
           "        for (auto& agent : Agents())\n"
           "        {\n"
           "            agent.x += agent.vx * deltaTime;\n"
           "            agent.y += agent.vy * deltaTime;\n"
           "        }\n"
           "    }\n"
           "    void Render(SDL_Renderer* renderer, AssetRegistry& assetRegistry, const SDL_FRect& camera) override\n"
           "    {\n"
           "        (void)renderer;\n"
           "        (void)assetRegistry;\n"
           "        (void)camera;\n"
           "    }\n"
           "};\n\n"
           "#endif // " + guard + "\n";
}

std::string BuildPhysicsScenarioTemplate(const std::string& className)
{
    return BuildProjectSystemTemplate(className);
}

bool UpdateProjectModuleForGeneratedClass(
    const ProjectConfig& projectConfig,
    CppClassKind kind,
    const std::string& className,
    std::string& outMessage
)
{
    const std::filesystem::path moduleHeaderPath = GetProjectModuleHeaderPath(projectConfig);
    const std::filesystem::path moduleSourcePath = GetProjectModuleSourcePath(projectConfig);
    const std::string moduleClassName = GetProjectModuleClassName(projectConfig);

    switch (kind)
    {
    case CppClassKind::Component:
    case CppClassKind::Event:
    case CppClassKind::PhysicsScenario:
        return true;
    case CppClassKind::ProjectSystem:
        return InsertIncludeIfMissing(moduleHeaderPath, "#include \"Systems/" + className + ".h\"", outMessage) &&
            InsertAfter(moduleHeaderPath, "private:\n", "    " + className + " " + className + "Instance;\n", outMessage) &&
            InsertBefore(moduleSourcePath, "\n}\n\nvoid " + moduleClassName + "::Stop", "    " + className + "Instance.Update(context);\n", outMessage);
    case CppClassKind::EntitySystem:
        return InsertIncludeIfMissing(moduleSourcePath, "#include \"Systems/" + className + ".h\"", outMessage) &&
            InsertBefore(moduleSourcePath, "\n}\n\nvoid " + moduleClassName + "::Loaded", "    registry.AddSystem<" + className + ">();\n", outMessage);
    case CppClassKind::EntityClass:
        return InsertIncludeIfMissing(moduleSourcePath, "#include \"Entity/" + className + ".h\"", outMessage) &&
            InsertBefore(
                moduleSourcePath,
                "\n}\n\nvoid " + moduleClassName + "::RegisterEntitySystems",
                "    registry.RegisterEntityClass({\n"
                "        .typeName = \"" + className + "\",\n"
                "        .displayName = \"" + className + "\",\n"
                "        .category = \"Project\",\n"
                "        .create = " + className + "::Create\n"
                "    });\n",
                outMessage
            );
    case CppClassKind::DenseAgentSimulation:
        return InsertIncludeIfMissing(moduleHeaderPath, "#include \"Simulations/" + className + ".h\"", outMessage) &&
            InsertAfter(moduleHeaderPath, "private:\n", "    " + className + " " + className + "Instance;\n", outMessage) &&
            InsertBefore(moduleSourcePath, "\n}\n\nvoid " + moduleClassName + "::Update", "    " + className + "Instance.Start(context);\n", outMessage) &&
            InsertBefore(moduleSourcePath, "\n}\n\nvoid " + moduleClassName + "::Stop", "    " + className + "Instance.Update(context);\n", outMessage);
    }

    return true;
}
}

CppClassGenerationResult GenerateProjectCppClass(
    const ProjectConfig& projectConfig,
    CppClassKind kind,
    const std::string& requestedClassName
)
{
    CppClassGenerationResult result;
    const std::string className = ToCppIdentifier(requestedClassName);
    if (className.empty())
    {
        result.message = "Class name is empty";
        return result;
    }

    const std::filesystem::path sourceRoot = projectConfig.projectRoot / "Source";
    std::string content;

    switch (kind)
    {
    case CppClassKind::Component:
        result.generatedFilePath = sourceRoot / "Components" / (className + ".h");
        content = BuildComponentTemplate(className);
        break;
    case CppClassKind::ProjectSystem:
        result.generatedFilePath = sourceRoot / "Systems" / (className + ".h");
        content = BuildProjectSystemTemplate(className);
        break;
    case CppClassKind::EntitySystem:
        result.generatedFilePath = sourceRoot / "Systems" / (className + ".h");
        content = BuildEntitySystemTemplate(className);
        break;
    case CppClassKind::Event:
        result.generatedFilePath = sourceRoot / "Events" / (className + ".h");
        content = BuildEventTemplate(className);
        break;
    case CppClassKind::EntityClass:
        result.generatedFilePath = sourceRoot / "Entity" / (className + ".h");
        content = BuildEntityClassTemplate(className);
        break;
    case CppClassKind::DenseAgentSimulation:
        result.generatedFilePath = sourceRoot / "Simulations" / (className + ".h");
        content = BuildDenseSimulationTemplate(className);
        break;
    case CppClassKind::PhysicsScenario:
        result.generatedFilePath = sourceRoot / "PhysicsScenarios" / (className + ".h");
        content = BuildPhysicsScenarioTemplate(className);
        break;
    }

    if (!WriteNewFile(result.generatedFilePath, content, result.message))
    {
        return result;
    }

    if (!UpdateProjectModuleForGeneratedClass(projectConfig, kind, className, result.message))
    {
        return result;
    }

    result.success = true;
    result.message = "Created C++ class: " + result.generatedFilePath.string();
    return result;
}

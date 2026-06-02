#include "ProjectGenerator.h"

#include <cctype>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace
{
constexpr int StarterRows = 16;
constexpr int StarterCols = 16;

std::string Trim(const std::string& value)
{
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
    {
        first++;
    }

    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
    {
        last--;
    }

    return value.substr(first, last - first);
}

std::string ToFolderName(const std::string& value)
{
    std::string result;

    for (char ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)))
        {
            result += ch;
        }
        else if (ch == '-' || ch == '_' || std::isspace(static_cast<unsigned char>(ch)))
        {
            result += '_';
        }
    }

    return result;
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

bool WriteTextFile(const std::filesystem::path& path, const std::string& text, std::string& outError)
{
    std::filesystem::create_directories(path.parent_path());

    std::ofstream output(path);
    if (!output)
    {
        outError = "Failed to write file: " + path.string();
        return false;
    }

    output << text;
    return true;
}

bool CopyFileIfExists(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& outError
)
{
    if (!std::filesystem::exists(source))
    {
        return false;
    }

    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::overwrite_existing
    );
    return true;
}

bool CopyDirectoryIfExists(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& outError
)
{
    if (!std::filesystem::exists(source))
    {
        return false;
    }

    std::filesystem::create_directories(destination);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(source))
    {
        const std::filesystem::path relativePath = std::filesystem::relative(entry.path(), source);
        const std::filesystem::path destinationPath = destination / relativePath;

        if (entry.is_directory())
        {
            std::filesystem::create_directories(destinationPath);
        }
        else if (entry.is_regular_file())
        {
            std::filesystem::create_directories(destinationPath.parent_path());
            std::filesystem::copy_file(
                entry.path(),
                destinationPath,
                std::filesystem::copy_options::overwrite_existing
            );
        }
    }

    return true;
}

std::string BuildCsvGrid(int rows, int cols, const std::string& value)
{
    std::ostringstream stream;

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            if (col > 0)
            {
                stream << ",";
            }

            stream << value;
        }

        if (row < rows - 1)
        {
            stream << "\n";
        }
    }

    return stream.str();
}

std::string BuildProjectJson(const std::string& projectName, bool includeSampleAntTaxonomy)
{
    nlohmann::json projectJson;
    projectJson["name"] = projectName;
    projectJson["assets_root"] = "assets";
    projectJson["asset_manifest"] = "assets/AssetManifest.json";
    projectJson["prefab_directory"] = "assets/prefabs";
    projectJson["startup_level"] = "assets/levels/Level1.json";
    projectJson["simulation"] = {
        {"field_cell_size", 16},
        {"field_decay_per_second", 0.35}
    };
    projectJson["tags"] = nlohmann::json::array();
    projectJson["groups"] = includeSampleAntTaxonomy
        ? nlohmann::json::array({"agents", "swarms", "colonies", "food", "obstacles"})
        : nlohmann::json::array();

    return projectJson.dump(2) + "\n";
}

nlohmann::json BuildLevelJson()
{
    nlohmann::json levelJson;
    levelJson["tilemap"] = {
        {"map_file", "starter.map"},
        {"texture_asset_id", "starter-tilemap-texture"},
        {"num_rows", StarterRows},
        {"num_cols", StarterCols},
        {"tile_size", 32},
        {"scale", 2.0},
    };
    levelJson["terrain_file"] = "starter.terrain";
    levelJson["entities_file"] = "starter.entities.json";

    return levelJson;
}

nlohmann::json BuildAssetManifestJson(bool copiedAnt, bool copiedMarker)
{
    nlohmann::json manifest;
    manifest["assets"] = nlohmann::json::array();
    manifest["assets"].push_back({
        {"type", "texture"},
        {"id", "starter-tilemap-texture"},
        {"file", "tilemaps/starter.png"},
        {"sprite", {
            {"mode", "image"},
            {"default_display_size", {
                {"w", 32},
                {"h", 32}
            }}
        }},
    });

    if (copiedAnt)
    {
        manifest["assets"].push_back({
            {"type", "texture"},
            {"id", "ant-texture"},
            {"file", "images/ant.png"},
            {"sprite", {
                {"mode", "image"},
                {"default_display_size", {
                    {"w", 32},
                    {"h", 48}
                }}
            }},
        });
    }

    if (copiedMarker)
    {
        manifest["assets"].push_back({
            {"type", "texture"},
            {"id", "marker-texture"},
            {"file", "images/marker.png"},
            {"sprite", {
                {"mode", "image"},
                {"default_display_size", {
                    {"w", 32},
                    {"h", 32}
                }}
            }},
        });
    }

    return manifest;
}

nlohmann::json BuildEntitiesJson(bool copiedAnt, bool copiedMarker)
{
    nlohmann::json entitiesJson;
    entitiesJson["entities"] = nlohmann::json::array();

    if (copiedMarker)
    {
        entitiesJson["entities"].push_back({
            {"id", "marker_01"},
            {"components", {
                {"transform", {
                    {"position", {{"x", 200.0}, {"y", 200.0}}},
                    {"scale", {{"x", 1.0}, {"y", 1.0}}},
                    {"rotation", 0.0},
                }},
                {"sprite", {
                    {"texture_asset_id", "marker-texture"},
                    {"width", 32},
                    {"height", 32},
                    {"z_index", 1},
                    {"fixed", false},
                    {"src_rect_x", 0.0},
                    {"src_rect_y", 0.0},
                    {"src_rect_w", 100.0},
                    {"src_rect_h", 100.0},
                }},
            }},
        });
    }

    if (copiedAnt)
    {
        entitiesJson["entities"].push_back({
            {"id", "ant_01"},
            {"group", "agents"},
            {"components", {
                {"transform", {
                    {"position", {{"x", 256.0}, {"y", 256.0}}},
                    {"scale", {{"x", 1.0}, {"y", 1.0}}},
                    {"rotation", 0.0},
                }},
                {"sprite", {
                    {"texture_asset_id", "ant-texture"},
                    {"width", 32},
                    {"height", 48},
                    {"z_index", 2},
                    {"fixed", false},
                    {"src_rect_x", 0.0},
                    {"src_rect_y", 0.0},
                    {"src_rect_w", 370.0},
                    {"src_rect_h", 552.0},
                }},
                {"boxcollider", {
                    {"width", 12},
                    {"height", 12},
                    {"offset", {{"x", 10.0}, {"y", 28.0}}},
                }},
                {"movement", {{"type", "land"}}},
            }},
        });
    }

    return entitiesJson;
}

nlohmann::json BuildAntPrefabJson()
{
    return {
        {"id", "ant"},
        {"group", "agents"},
        {"components", {
            {"transform", {
                {"position", {{"x", 256.0}, {"y", 256.0}}},
                {"scale", {{"x", 1.0}, {"y", 1.0}}},
                {"rotation", 0.0},
            }},
            {"sprite", {
                {"texture_asset_id", "ant-texture"},
                {"width", 32},
                {"height", 48},
                {"z_index", 2},
                {"fixed", false},
                {"src_rect_x", 0.0},
                {"src_rect_y", 0.0},
                {"src_rect_w", 370.0},
                {"src_rect_h", 552.0},
            }},
            {"boxcollider", {
                {"width", 12},
                {"height", 12},
                {"offset", {{"x", 10.0}, {"y", 28.0}}},
            }},
            {"movement", {{"type", "land"}}},
        }},
    };
}

nlohmann::json BuildFoodPrefabJson()
{
    return {
        {"id", "food"},
        {"group", "food"},
        {"components", {
            {"transform", {
                {"position", {{"x", 200.0}, {"y", 200.0}}},
                {"scale", {{"x", 1.0}, {"y", 1.0}}},
                {"rotation", 0.0},
            }},
            {"sprite", {
                {"texture_asset_id", "marker-texture"},
                {"width", 24},
                {"height", 24},
                {"z_index", 1},
                {"fixed", false},
                {"src_rect_x", 0.0},
                {"src_rect_y", 0.0},
                {"src_rect_w", 100.0},
                {"src_rect_h", 100.0},
            }},
            {"boxcollider", {
                {"width", 24},
                {"height", 24},
                {"offset", {{"x", 0.0}, {"y", 0.0}}},
            }},
        }},
    };
}

nlohmann::json BuildColonyPrefabJson()
{
    return {
        {"id", "colony"},
        {"group", "colonies"},
        {"components", {
            {"transform", {
                {"position", {{"x", 160.0}, {"y", 160.0}}},
                {"scale", {{"x", 1.0}, {"y", 1.0}}},
                {"rotation", 0.0},
            }},
            {"sprite", {
                {"texture_asset_id", "marker-texture"},
                {"width", 48},
                {"height", 48},
                {"z_index", 1},
                {"fixed", false},
                {"src_rect_x", 100.0},
                {"src_rect_y", 0.0},
                {"src_rect_w", 100.0},
                {"src_rect_h", 100.0},
            }},
            {"boxcollider", {
                {"width", 48},
                {"height", 48},
                {"offset", {{"x", 0.0}, {"y", 0.0}}},
            }},
        }},
    };
}

std::string BuildSourceCMake(const std::string& projectIdentifier)
{
    return "file(GLOB_RECURSE PIPEFRAME_" + projectIdentifier + "_SOURCES CONFIGURE_DEPENDS\n"
           "    \"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp\"\n"
           "    \"${CMAKE_CURRENT_SOURCE_DIR}/*.h\"\n"
           ")\n\n"
           "set(PIPEFRAME_" + projectIdentifier + "_GENERATED_DIR \"${CMAKE_CURRENT_BINARY_DIR}/Generated\")\n"
           "set(PIPEFRAME_" + projectIdentifier + "_GENERATED_HEADER\n"
           "    \"${PIPEFRAME_" + projectIdentifier + "_GENERATED_DIR}/ProjectComponents.generated.h\"\n"
           ")\n\n"
           "add_custom_command(\n"
           "    OUTPUT \"${PIPEFRAME_" + projectIdentifier + "_GENERATED_HEADER}\"\n"
           "    COMMAND PipeFrameHeaderTool\n"
           "        \"${CMAKE_CURRENT_SOURCE_DIR}\"\n"
           "        \"${PIPEFRAME_" + projectIdentifier + "_GENERATED_HEADER}\"\n"
           "    DEPENDS\n"
           "        PipeFrameHeaderTool\n"
           "        ${PIPEFRAME_" + projectIdentifier + "_SOURCES}\n"
           "    COMMENT \"Generating " + projectIdentifier + " component metadata\"\n"
           "    VERBATIM\n"
           ")\n\n"
           "add_library(PipeFrame" + projectIdentifier + " SHARED\n"
           "    ${PIPEFRAME_" + projectIdentifier + "_SOURCES}\n"
           "    \"${PIPEFRAME_" + projectIdentifier + "_GENERATED_HEADER}\"\n"
           ")\n\n"
           "target_include_directories(PipeFrame" + projectIdentifier + " PUBLIC\n"
           "    \"${CMAKE_CURRENT_SOURCE_DIR}\"\n"
           "    \"${CMAKE_CURRENT_BINARY_DIR}\"\n"
           ")\n\n"
           "target_link_libraries(PipeFrame" + projectIdentifier + " PRIVATE\n"
           "    PipeFrameEngine\n"
           ")\n";
}

std::string BuildProjectModuleHeader(const std::string& projectIdentifier)
{
    return "#ifndef " + projectIdentifier + "_MODULE_H\n"
           "#define " + projectIdentifier + "_MODULE_H\n\n"
           "#include \"Simulation/ProjectModule.h\"\n\n"
           "class " + projectIdentifier + "Module : public ProjectModule\n"
           "{\n"
           "public:\n"
           "    std::string GetName() const override;\n"
           "    void RegisterComponents(ComponentRegistry& registry) override;\n"
           "    void RegisterEntityClasses(ClassRegistry& registry) override;\n"
           "    void RegisterEntitySystems(Registry& registry) override;\n"
           "    void Loaded(ProjectRuntimeContext& context) override;\n"
           "    void Start(ProjectRuntimeContext& context) override;\n"
           "    void Update(ProjectRuntimeContext& context) override;\n"
           "    void Stop(ProjectRuntimeContext& context) override;\n"
           "    void Unloaded(ProjectRuntimeContext& context) override;\n"
           "    void RenderProjectSimulation(\n"
           "        SDL_Renderer* renderer,\n"
           "        AssetRegistry& assetRegistry,\n"
           "        const SDL_FRect& camera\n"
           "    ) override;\n"
           "};\n\n"
           "#endif // " + projectIdentifier + "_MODULE_H\n";
}

std::string BuildProjectModuleCpp(const std::string& projectName, const std::string& projectIdentifier)
{
    return "#include \"" + projectIdentifier + "Module.h\"\n\n"
           "#include \"Entity/ExampleEntity.h\"\n"
           "#include \"Generated/ProjectComponents.generated.h\"\n"
           "#include \"Systems/ExampleEntitySystem.h\"\n"
           "#include \"Reflection/EditorMetadata.h\"\n"
           "#include \"ECS/Registry.h\"\n\n"
           "std::string " + projectIdentifier + "Module::GetName() const\n"
           "{\n"
           "    return \"" + projectName + "\";\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::RegisterComponents(ComponentRegistry& registry)\n"
           "{\n"
           "    RegisterGeneratedProjectComponents(registry);\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::RegisterEntityClasses(ClassRegistry& registry)\n"
           "{\n"
           "    registry.RegisterEntityClass({\n"
           "        .typeName = \"ExampleEntity\",\n"
           "        .displayName = \"Example Entity\",\n"
           "        .category = \"Entities\",\n"
           "        .create = ExampleEntity::Create\n"
           "    });\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::RegisterEntitySystems(Registry& registry)\n"
           "{\n"
           "    registry.AddSystem<ExampleEntitySystem>();\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::Loaded(ProjectRuntimeContext& context)\n"
           "{\n"
           "    (void)context;\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::Start(ProjectRuntimeContext& context)\n"
           "{\n"
           "    (void)context;\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::Update(ProjectRuntimeContext& context)\n"
           "{\n"
           "    (void)context;\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::Stop(ProjectRuntimeContext& context)\n"
           "{\n"
           "    (void)context;\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::Unloaded(ProjectRuntimeContext& context)\n"
           "{\n"
           "    (void)context;\n"
           "}\n\n"
           "void " + projectIdentifier + "Module::RenderProjectSimulation(\n"
           "    SDL_Renderer* renderer,\n"
           "    AssetRegistry& assetRegistry,\n"
           "    const SDL_FRect& camera\n"
           ")\n"
           "{\n"
           "    (void)renderer;\n"
           "    (void)assetRegistry;\n"
           "    (void)camera;\n"
           "}\n\n"
           "extern \"C\" ProjectModule* CreateProjectModule()\n"
           "{\n"
           "    return new " + projectIdentifier + "Module();\n"
           "}\n\n"
           "extern \"C\" void DestroyProjectModule(ProjectModule* module)\n"
           "{\n"
           "    delete module;\n"
           "}\n";
}

std::string BuildExampleComponentHeader()
{
    return "#ifndef PIPEFRAME_EXAMPLECOMPONENT_H\n"
           "#define PIPEFRAME_EXAMPLECOMPONENT_H\n\n"
           "#include \"Reflection/ComponentAnnotations.h\"\n\n"
           "PF_COMPONENT()\n"
           "struct ExampleComponent\n"
           "{\n"
           "    PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)\n"
           "    int value = 10;\n"
           "};\n\n"
           "#endif // PIPEFRAME_EXAMPLECOMPONENT_H\n";
}

std::string BuildExampleEntitySystemHeader()
{
    return "#ifndef PIPEFRAME_EXAMPLEENTITYSYSTEM_H\n"
           "#define PIPEFRAME_EXAMPLEENTITYSYSTEM_H\n\n"
           "#include \"Components/ExampleComponent.h\"\n"
           "#include \"Components/TransformComponent.h\"\n"
           "#include \"ECS/ECS.h\"\n\n"
           "#include \"Events/ExampleEvent.h\"\n\n"
           "class ExampleEntitySystem : public EntitySystem\n"
           "{\n"
           "public:\n"
           "    void Loaded() override\n"
           "    {\n"
           "        RequireComponent<TransformComponent>();\n"
           "        RequireComponent<ExampleComponent>();\n"
           "    }\n\n"
           "    void SubscribeToEvents(EntitySystemContext& context) override\n"
           "    {\n"
           "        Listen<ExampleEvent>(context, &ExampleEntitySystem::OnExampleEvent);\n"
           "    }\n\n"
           "    void Update(EntitySystemContext& context) override\n"
           "    {\n"
           "        for (Entity entity : GetSystemEntities())\n"
           "        {\n"
           "            auto& transform = entity.GetComponent<TransformComponent>();\n"
           "            const auto& example = entity.GetComponent<ExampleComponent>();\n"
           "            transform.rotation += example.value * static_cast<float>(context.deltaTime);\n"
           "            // Emit<ExampleEvent>(context, entity, \"Example entity updated\");\n"
           "        }\n"
           "    }\n"
           "\n"
           "private:\n"
           "    void OnExampleEvent(ExampleEvent& event)\n"
           "    {\n"
           "        (void)event;\n"
           "    }\n"
           "};\n\n"
           "#endif // PIPEFRAME_EXAMPLEENTITYSYSTEM_H\n";
}

std::string BuildExampleEventHeader()
{
    return "#ifndef PIPEFRAME_EXAMPLEEVENT_H\n"
           "#define PIPEFRAME_EXAMPLEEVENT_H\n\n"
           "#include <string>\n"
           "#include <utility>\n\n"
           "#include \"ECS/Entity.h\"\n"
           "#include \"EventBus/Event.h\"\n\n"
           "class ExampleEvent : public Event\n"
           "{\n"
           "public:\n"
           "    Entity entity;\n"
           "    std::string message;\n\n"
           "    ExampleEvent(Entity entity, std::string message)\n"
           "        : entity(entity), message(std::move(message))\n"
           "    {\n"
           "    }\n"
           "};\n\n"
           "#endif // PIPEFRAME_EXAMPLEEVENT_H\n";
}

std::string BuildExampleEntityHeader()
{
    return "#ifndef PIPEFRAME_EXAMPLEENTITY_H\n"
           "#define PIPEFRAME_EXAMPLEENTITY_H\n\n"
           "#include <glm/glm.hpp>\n\n"
           "#include \"Components/EditorEntityComponent.h\"\n"
           "#include \"Components/PersistentIdComponent.h\"\n"
           "#include \"Components/TransformComponent.h\"\n"
           "#include \"ECS/ECS.h\"\n"
           "#include \"Project/EntityIdGenerator.h\"\n\n"
           "struct ExampleEntity\n"
           "{\n"
           "    static Entity Create(Registry& registry, glm::vec2 position)\n"
           "    {\n"
           "        Entity entity = registry.CreateEntity();\n"
           "        entity.AddComponent<EditorEntityComponent>();\n"
           "        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, \"ExampleEntity\"));\n"
           "        entity.AddComponent<TransformComponent>(position);\n"
           "        return entity;\n"
           "    }\n"
           "};\n\n"
           "#endif // PIPEFRAME_EXAMPLEENTITY_H\n";
}

std::string BuildExampleDenseSimulationHeader()
{
    return "#ifndef PIPEFRAME_EXAMPLEDENSESIMULATION_H\n"
           "#define PIPEFRAME_EXAMPLEDENSESIMULATION_H\n\n"
           "#include <SDL3/SDL.h>\n\n"
           "#include \"Assets/AssetRegistry.h\"\n"
           "#include \"Simulation/DenseAgentSimulation.h\"\n"
           "#include \"Simulation/ProjectRuntime.h\"\n\n"
           "struct ExampleAgent\n"
           "{\n"
           "    float x = 0.0f;\n"
           "    float y = 0.0f;\n"
           "};\n\n"
           "class ExampleDenseSimulation : public DenseAgentSimulation<ExampleAgent>, public ProjectSimulation\n"
           "{\n"
           "public:\n"
           "    void Start(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "        Clear();\n"
           "    }\n\n"
           "    void Update(ProjectRuntimeContext& context) override\n"
           "    {\n"
           "        (void)context;\n"
           "    }\n\n"
           "    void Render(SDL_Renderer* renderer, AssetRegistry& assetRegistry, const SDL_FRect& camera) override\n"
           "    {\n"
           "        (void)renderer;\n"
           "        (void)assetRegistry;\n"
           "        (void)camera;\n"
           "    }\n"
           "};\n\n"
           "#endif // PIPEFRAME_EXAMPLEDENSESIMULATION_H\n";
}
}

bool GeneratePipeFrameProject(
    const ProjectGeneratorOptions& options,
    std::filesystem::path& outProjectFilePath,
    std::string& outError
)
{
    try
    {
        const std::string projectName = Trim(options.projectName);
        const std::string folderName = ToFolderName(projectName);
        const std::string projectIdentifier = ToCppIdentifier(folderName);

        if (projectName.empty() || folderName.empty() || projectIdentifier.empty())
        {
            outError = "Project name is empty";
            return false;
        }

        std::filesystem::path parentDirectory = options.parentDirectory;
        if (parentDirectory.empty())
        {
            parentDirectory = options.defaultProjectsRoot;
        }
        else if (parentDirectory == "projects")
        {
            parentDirectory = options.defaultProjectsRoot;
        }

        if (parentDirectory.is_relative())
        {
            parentDirectory = options.defaultProjectsRoot / parentDirectory;
        }

        const std::filesystem::path projectRoot = parentDirectory / folderName;

        if (std::filesystem::exists(projectRoot) && !std::filesystem::is_empty(projectRoot))
        {
            outError = "Project folder already exists and is not empty: " + projectRoot.string();
            return false;
        }

        std::filesystem::create_directories(projectRoot / "assets" / "images");
        std::filesystem::create_directories(projectRoot / "assets" / "levels");
        std::filesystem::create_directories(projectRoot / "assets" / "prefabs");
        std::filesystem::create_directories(projectRoot / "assets" / "tilemaps");
        std::filesystem::create_directories(projectRoot / "Source" / "Components");
        std::filesystem::create_directories(projectRoot / "Source" / "Systems");
        std::filesystem::create_directories(projectRoot / "Source" / "Events");
        std::filesystem::create_directories(projectRoot / "Source" / "Entity");
        std::filesystem::create_directories(projectRoot / "Source" / "Simulations");

        const std::filesystem::path starterTileset =
            options.templateProjectRoot / "assets" / "tilemaps" / "jungle.png";
        if (!CopyFileIfExists(
                starterTileset,
                projectRoot / "assets" / "tilemaps" / "starter.png",
                outError
            ))
        {
            outError = "Missing starter tileset template: " + starterTileset.string();
            return false;
        }

        const bool copiedAnt = options.copySampleAntAssets &&
            CopyFileIfExists(
                options.sampleAssetRoot / "ant.png",
                projectRoot / "assets" / "images" / "ant.png",
                outError
            );
        const bool copiedMarker = options.copySampleAntAssets &&
            CopyFileIfExists(
                options.sampleAssetRoot / "marker.png",
                projectRoot / "assets" / "images" / "marker.png",
                outError
            );

        if (!WriteTextFile(
                projectRoot / "PipeFrameProject.json",
                BuildProjectJson(projectName, options.copySampleAntAssets),
                outError
            ) ||
            !WriteTextFile(projectRoot / "assets" / "AssetManifest.json", BuildAssetManifestJson(copiedAnt, copiedMarker).dump(2) + "\n", outError) ||
            !WriteTextFile(projectRoot / "assets" / "levels" / "Level1.json", BuildLevelJson().dump(2) + "\n", outError) ||
            !WriteTextFile(projectRoot / "assets" / "levels" / "starter.map", BuildCsvGrid(StarterRows, StarterCols, "00"), outError) ||
            !WriteTextFile(projectRoot / "assets" / "levels" / "starter.terrain", BuildCsvGrid(StarterRows, StarterCols, "0"), outError) ||
            !WriteTextFile(projectRoot / "assets" / "levels" / "starter.entities.json", BuildEntitiesJson(copiedAnt, copiedMarker).dump(2) + "\n", outError) ||
            !WriteTextFile(projectRoot / "Source" / "CMakeLists.txt", BuildSourceCMake(projectIdentifier), outError) ||
            !WriteTextFile(projectRoot / "Source" / (projectIdentifier + "Module.h"), BuildProjectModuleHeader(projectIdentifier), outError) ||
            !WriteTextFile(projectRoot / "Source" / (projectIdentifier + "Module.cpp"), BuildProjectModuleCpp(projectName, projectIdentifier), outError) ||
            !WriteTextFile(projectRoot / "Source" / "Components" / "ExampleComponent.h", BuildExampleComponentHeader(), outError) ||
            !WriteTextFile(projectRoot / "Source" / "Events" / "ExampleEvent.h", BuildExampleEventHeader(), outError) ||
            !WriteTextFile(projectRoot / "Source" / "Systems" / "ExampleEntitySystem.h", BuildExampleEntitySystemHeader(), outError) ||
            !WriteTextFile(projectRoot / "Source" / "Entity" / "ExampleEntity.h", BuildExampleEntityHeader(), outError) ||
            !WriteTextFile(projectRoot / "Source" / "Simulations" / "ExampleDenseSimulation.h", BuildExampleDenseSimulationHeader(), outError))
        {
            return false;
        }

        if (copiedAnt && copiedMarker)
        {
            if (!WriteTextFile(projectRoot / "assets" / "prefabs" / "ant.json", BuildAntPrefabJson().dump(2) + "\n", outError) ||
                !WriteTextFile(projectRoot / "assets" / "prefabs" / "food.json", BuildFoodPrefabJson().dump(2) + "\n", outError) ||
                !WriteTextFile(projectRoot / "assets" / "prefabs" / "colony.json", BuildColonyPrefabJson().dump(2) + "\n", outError))
            {
                return false;
            }
        }

        outProjectFilePath = projectRoot / "PipeFrameProject.json";
        return true;
    }
    catch (const std::exception& exception)
    {
        outError = exception.what();
        return false;
    }
}

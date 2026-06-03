#include "ProjectConfig.h"

#include <algorithm>
#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Logger/Logger.h"

namespace
{
std::filesystem::path ResolveProjectPath(
    const std::filesystem::path& projectRoot,
    const std::string& path
)
{
    std::filesystem::path resolvedPath(path);

    if (resolvedPath.is_relative())
    {
        resolvedPath = projectRoot / resolvedPath;
    }

    return resolvedPath;
}

std::vector<std::string> ReadStringArray(
    const nlohmann::json& json,
    const std::string& key,
    const std::vector<std::string>& fallback
)
{
    if (!json.contains(key) || !json[key].is_array())
    {
        return fallback;
    }

    std::vector<std::string> values;

    for (const auto& item : json[key])
    {
        if (!item.is_string())
        {
            continue;
        }

        const std::string value = item.get<std::string>();
        if (value.empty())
        {
            continue;
        }

        if (std::find(values.begin(), values.end(), value) == values.end())
        {
            values.push_back(value);
        }
    }

    return values;
}

std::vector<std::string> ReadDisabledEngineSystemsConfig(
    const nlohmann::json& json,
    const std::vector<std::string>& fallback
)
{
    std::vector<std::string> disabledSystems = ReadStringArray(
        json,
        "disabled_engine_systems",
        fallback
    );

    if (!json.contains("engine_systems") || !json["engine_systems"].is_object())
    {
        return disabledSystems;
    }

    for (const auto& [name, value] : json["engine_systems"].items())
    {
        if (!value.is_boolean())
        {
            continue;
        }

        const auto disabledSystem = std::find(disabledSystems.begin(), disabledSystems.end(), name);
        if (!value.get<bool>() && disabledSystem == disabledSystems.end())
        {
            disabledSystems.push_back(name);
        }
        else if (value.get<bool>() && disabledSystem != disabledSystems.end())
        {
            disabledSystems.erase(disabledSystem);
        }
    }

    return disabledSystems;
}

SimulationConfig ReadSimulationConfig(const nlohmann::json& json, const SimulationConfig& fallback)
{
    SimulationConfig simulation = fallback;

    if (!json.contains("simulation") || !json["simulation"].is_object())
    {
        return simulation;
    }

    const auto& simulationJson = json["simulation"];
    simulation.fieldCellSize = simulationJson.value("field_cell_size", simulation.fieldCellSize);
    simulation.fieldDecayPerSecond = simulationJson.value(
        "field_decay_per_second",
        simulation.fieldDecayPerSecond
    );

    if (simulation.fieldCellSize <= 0.0f)
    {
        simulation.fieldCellSize = fallback.fieldCellSize;
    }

    if (simulation.fieldDecayPerSecond < 0.0)
    {
        simulation.fieldDecayPerSecond = fallback.fieldDecayPerSecond;
    }

    return simulation;
}

ProjectConfig CreateFallbackProjectConfig(const std::filesystem::path& projectFilePath)
{
    ProjectConfig config;

    config.name = projectFilePath.parent_path().filename().string();
    config.projectFilePath = projectFilePath;
    config.projectRoot = projectFilePath.parent_path();
    config.assetsRoot = config.projectRoot / "assets";
    config.assetManifestPath = config.assetsRoot / "AssetManifest.json";
    config.prefabDirectory = config.assetsRoot / "prefabs";
    config.startupLevelPath = config.assetsRoot / "levels" / "Level1.json";
    config.simulation = {};
    config.disabledEngineSystems = {};
    config.tags = {"player"};
    config.groups = {"enemies", "projectiles", "obstacles"};

    return config;
}
}

ProjectConfig LoadProjectConfig(const std::filesystem::path& projectFilePath)
{
    ProjectConfig config = CreateFallbackProjectConfig(projectFilePath);

    std::ifstream input(projectFilePath);
    if (!input.is_open())
    {
        Logger::Err("Failed to open project config: " + projectFilePath.string());
        return config;
    }

    nlohmann::json projectJson;
    try
    {
        input >> projectJson;
    }
    catch (const std::exception& exception)
    {
        Logger::Err(
            "Failed to parse project config: " +
            projectFilePath.string() +
            " error: " +
            exception.what()
        );
        return config;
    }

    config.name = projectJson.value("name", config.name);
    config.assetsRoot = ResolveProjectPath(
        config.projectRoot,
        projectJson.value("assets_root", std::string("assets"))
    );
    config.assetManifestPath = ResolveProjectPath(
        config.projectRoot,
        projectJson.value("asset_manifest", std::string("assets/AssetManifest.json"))
    );
    config.prefabDirectory = ResolveProjectPath(
        config.projectRoot,
        projectJson.value("prefab_directory", std::string("assets/prefabs"))
    );
    config.startupLevelPath = ResolveProjectPath(
        config.projectRoot,
        projectJson.value("startup_level", std::string("assets/levels/Level1.json"))
    );
    config.simulation = ReadSimulationConfig(projectJson, config.simulation);
    config.disabledEngineSystems = ReadDisabledEngineSystemsConfig(
        projectJson,
        config.disabledEngineSystems
    );
    config.tags = ReadStringArray(projectJson, "tags", config.tags);
    config.groups = ReadStringArray(projectJson, "groups", config.groups);

    return config;
}

ProjectConfig CreateDefaultProjectConfig()
{
    return LoadProjectConfig("projects/JungleDemo/PipeFrameProject.json");
}

bool IsEngineSystemEnabled(const ProjectConfig& config, const std::string& systemName)
{
    return std::find(
        config.disabledEngineSystems.begin(),
        config.disabledEngineSystems.end(),
        systemName
    ) == config.disabledEngineSystems.end();
}

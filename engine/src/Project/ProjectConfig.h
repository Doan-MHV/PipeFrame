

#ifndef PIPEFRAME_PROJECTCONFIG_H
#define PIPEFRAME_PROJECTCONFIG_H

#include <filesystem>
#include <string>
#include <vector>

struct SimulationConfig
{
    float fieldCellSize = 8.0f;
    double fieldDecayPerSecond = 0.35;
};

struct ProjectConfig
{
    std::string name;
    std::filesystem::path projectFilePath;
    std::filesystem::path projectRoot;
    std::filesystem::path assetsRoot;
    std::filesystem::path assetManifestPath;
    std::filesystem::path prefabDirectory;
    std::filesystem::path startupLevelPath;
    SimulationConfig simulation;
    std::vector<std::string> disabledEngineSystems;
    std::vector<std::string> tags;
    std::vector<std::string> groups;
};

ProjectConfig LoadProjectConfig(const std::filesystem::path& projectFilePath);
ProjectConfig CreateDefaultProjectConfig();
bool IsEngineSystemEnabled(const ProjectConfig& config, const std::string& systemName);

#endif //PIPEFRAME_PROJECTCONFIG_H

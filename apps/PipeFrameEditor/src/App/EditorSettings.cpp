#include "EditorSettings.h"

#include <cstdlib>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Logger/Logger.h"

namespace
{
std::filesystem::path GetSettingsFilePath()
{
    const char* home = std::getenv("HOME");

    if (home && home[0] != '\0')
    {
        return std::filesystem::path(home) / ".pipeframe" / "editor_settings.json";
    }

    return std::filesystem::current_path() / ".pipeframe" / "editor_settings.json";
}

std::filesystem::path LoadLastProjectFile()
{
    const std::filesystem::path settingsFilePath = GetSettingsFilePath();
    std::ifstream input(settingsFilePath);

    if (!input)
    {
        return {};
    }

    nlohmann::json settingsJson;
    try
    {
        input >> settingsJson;
    }
    catch (const std::exception& exception)
    {
        Logger::Err("Failed to parse editor settings: " + std::string(exception.what()));
        return {};
    }

    std::filesystem::path projectFilePath = settingsJson.value("last_project", std::string(""));
    if (projectFilePath.empty() || !std::filesystem::exists(projectFilePath))
    {
        return {};
    }

    return projectFilePath;
}
}

std::filesystem::path ResolveStartupProjectFile(
    int argc,
    char* argv[],
    const std::filesystem::path& defaultProjectFilePath
)
{
    if (argc > 1)
    {
        return argv[1];
    }

    const std::filesystem::path lastProjectFilePath = LoadLastProjectFile();
    if (!lastProjectFilePath.empty())
    {
        return lastProjectFilePath;
    }

    return defaultProjectFilePath;
}

void SaveLastProjectFile(const std::filesystem::path& projectFilePath)
{
    try
    {
        const std::filesystem::path settingsFilePath = GetSettingsFilePath();
        std::filesystem::create_directories(settingsFilePath.parent_path());

        nlohmann::json settingsJson;
        settingsJson["last_project"] = std::filesystem::absolute(projectFilePath).string();

        std::ofstream output(settingsFilePath);
        if (!output)
        {
            Logger::Err("Failed to write editor settings: " + settingsFilePath.string());
            return;
        }

        output << settingsJson.dump(2) << "\n";
    }
    catch (const std::exception& exception)
    {
        Logger::Err("Failed to save editor settings: " + std::string(exception.what()));
    }
}

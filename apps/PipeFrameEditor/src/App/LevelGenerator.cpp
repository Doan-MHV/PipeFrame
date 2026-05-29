#include "LevelGenerator.h"

#include <cctype>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace
{
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

std::string ToFileStem(const std::string& value)
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

nlohmann::json BuildLevelJson(
    const std::string& mapFileName,
    const std::string& terrainFileName,
    const std::string& entitiesFileName,
    const std::string& textureAssetId,
    int rows,
    int cols,
    int tileSize,
    float scale
)
{
    nlohmann::json levelJson;
    levelJson["tilemap"] = {
        {"map_file", mapFileName},
        {"texture_asset_id", textureAssetId},
        {"num_rows", rows},
        {"num_cols", cols},
        {"tile_size", tileSize},
        {"scale", scale},
    };
    levelJson["terrain_file"] = terrainFileName;
    levelJson["entities_file"] = entitiesFileName;

    return levelJson;
}

bool UpdateProjectStartupLevel(
    const ProjectConfig& projectConfig,
    const std::filesystem::path& levelFilePath,
    std::string& outError
)
{
    std::ifstream input(projectConfig.projectFilePath);
    if (!input)
    {
        outError = "Failed to open project config: " + projectConfig.projectFilePath.string();
        return false;
    }

    nlohmann::json projectJson;
    try
    {
        input >> projectJson;
    }
    catch (const std::exception& exception)
    {
        outError = "Failed to parse project config: " + std::string(exception.what());
        return false;
    }

    const std::filesystem::path relativeLevelPath =
        std::filesystem::relative(levelFilePath, projectConfig.projectRoot);
    projectJson["startup_level"] = relativeLevelPath.generic_string();

    return WriteTextFile(projectConfig.projectFilePath, projectJson.dump(2) + "\n", outError);
}
}

bool GeneratePipeFrameLevel(
    const LevelGeneratorOptions& options,
    std::filesystem::path& outLevelFilePath,
    std::string& outError
)
{
    try
    {
        const std::string levelName = Trim(options.levelName);
        const std::string fileStem = ToFileStem(levelName);

        if (levelName.empty() || fileStem.empty())
        {
            outError = "Level name is empty";
            return false;
        }

        if (options.rows <= 0 || options.cols <= 0 || options.tileSize <= 0 || options.scale <= 0.0f)
        {
            outError = "Level dimensions must be positive";
            return false;
        }

        std::string textureAssetId = Trim(options.tilemapTextureAssetId);
        if (textureAssetId.empty())
        {
            textureAssetId = "starter-tilemap-texture";
        }

        const std::filesystem::path levelsDirectory = options.projectConfig.assetsRoot / "levels";
        const std::filesystem::path levelFilePath = levelsDirectory / (fileStem + ".json");
        const std::filesystem::path mapFilePath = levelsDirectory / (fileStem + ".map");
        const std::filesystem::path terrainFilePath = levelsDirectory / (fileStem + ".terrain");
        const std::filesystem::path entitiesFilePath = levelsDirectory / (fileStem + ".entities.json");

        if (std::filesystem::exists(levelFilePath) ||
            std::filesystem::exists(mapFilePath) ||
            std::filesystem::exists(terrainFilePath) ||
            std::filesystem::exists(entitiesFilePath))
        {
            outError = "Level files already exist for: " + fileStem;
            return false;
        }

        const nlohmann::json entitiesJson = {{"entities", nlohmann::json::array()}};

        if (!WriteTextFile(
                levelFilePath,
                BuildLevelJson(
                    mapFilePath.filename().string(),
                    terrainFilePath.filename().string(),
                    entitiesFilePath.filename().string(),
                    textureAssetId,
                    options.rows,
                    options.cols,
                    options.tileSize,
                    options.scale
                ).dump(2) + "\n",
                outError
            ) ||
            !WriteTextFile(mapFilePath, BuildCsvGrid(options.rows, options.cols, "00"), outError) ||
            !WriteTextFile(terrainFilePath, BuildCsvGrid(options.rows, options.cols, "0"), outError) ||
            !WriteTextFile(entitiesFilePath, entitiesJson.dump(2) + "\n", outError) ||
            !UpdateProjectStartupLevel(options.projectConfig, levelFilePath, outError))
        {
            return false;
        }

        outLevelFilePath = levelFilePath;
        return true;
    }
    catch (const std::exception& exception)
    {
        outError = exception.what();
        return false;
    }
}

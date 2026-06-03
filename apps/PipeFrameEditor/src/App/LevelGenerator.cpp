#include "LevelGenerator.h"

#include <cctype>
#include <fstream>

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

nlohmann::json BuildTileMapJson(
    int rows,
    int cols,
    const std::string& textureAssetId,
    int tileSize,
    float scale
)
{
    nlohmann::json tileMapJson;
    tileMapJson["version"] = 1;
    tileMapJson["texture_asset_id"] = textureAssetId;
    tileMapJson["rows"] = rows;
    tileMapJson["cols"] = cols;
    tileMapJson["tile_size"] = tileSize;
    tileMapJson["scale"] = scale;
    tileMapJson["tiles"] = nlohmann::json::array();

    for (int row = 0; row < rows; row++)
    {
        nlohmann::json tileRow = nlohmann::json::array();

        for (int col = 0; col < cols; col++)
        {
            tileRow.push_back({
                {"tile", {
                    {"row", 0},
                    {"col", 0}
                }},
                {"terrain", "Land"}
            });
        }

        tileMapJson["tiles"].push_back(tileRow);
    }

    return tileMapJson;
}

nlohmann::json BuildLevelJson(
    const std::string& tileMapFileName,
    const std::string& entitiesFileName
)
{
    nlohmann::json levelJson;
    levelJson["tilemap_file"] = tileMapFileName;
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
        const std::filesystem::path tileMapFilePath = levelsDirectory / (fileStem + ".tilemap.json");
        const std::filesystem::path entitiesFilePath = levelsDirectory / (fileStem + ".entities.json");

        if (std::filesystem::exists(levelFilePath) ||
            std::filesystem::exists(tileMapFilePath) ||
            std::filesystem::exists(entitiesFilePath))
        {
            outError = "Level files already exist for: " + fileStem;
            return false;
        }

        const nlohmann::json entitiesJson = {{"entities", nlohmann::json::array()}};

        if (!WriteTextFile(
                levelFilePath,
                BuildLevelJson(
                    tileMapFilePath.filename().string(),
                    entitiesFilePath.filename().string()
                ).dump(2) + "\n",
                outError
            ) ||
            !WriteTextFile(
                tileMapFilePath,
                BuildTileMapJson(
                    options.rows,
                    options.cols,
                    textureAssetId,
                    options.tileSize,
                    options.scale
                ).dump(2) + "\n",
                outError
            ) ||
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

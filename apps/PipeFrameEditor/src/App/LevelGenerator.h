#ifndef PIPEFRAME_LEVELGENERATOR_H
#define PIPEFRAME_LEVELGENERATOR_H

#include <filesystem>
#include <string>

#include "Project/ProjectConfig.h"

struct LevelGeneratorOptions
{
    ProjectConfig projectConfig;
    std::string levelName;
    std::string tilemapTextureAssetId;
    int rows = 16;
    int cols = 16;
    int tileSize = 32;
    float scale = 2.0f;
};

bool GeneratePipeFrameLevel(
    const LevelGeneratorOptions& options,
    std::filesystem::path& outLevelFilePath,
    std::string& outError
);

#endif // PIPEFRAME_LEVELGENERATOR_H

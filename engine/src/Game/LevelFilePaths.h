#ifndef PIPEFRAME_LEVELFILEPATHS_H
#define PIPEFRAME_LEVELFILEPATHS_H

#include <filesystem>

struct LevelFilePaths
{
    std::filesystem::path levelPath;
    std::filesystem::path mapPath;
    std::filesystem::path terrainPath;
    std::filesystem::path entitiesPath;
};

#endif // PIPEFRAME_LEVELFILEPATHS_H

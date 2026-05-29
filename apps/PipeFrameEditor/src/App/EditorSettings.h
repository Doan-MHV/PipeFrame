#ifndef PIPEFRAME_EDITORSETTINGS_H
#define PIPEFRAME_EDITORSETTINGS_H

#include <filesystem>

std::filesystem::path ResolveStartupProjectFile(
    int argc,
    char* argv[],
    const std::filesystem::path& defaultProjectFilePath
);

void SaveLastProjectFile(const std::filesystem::path& projectFilePath);

#endif // PIPEFRAME_EDITORSETTINGS_H

#ifndef PIPEFRAME_PROJECTGENERATOR_H
#define PIPEFRAME_PROJECTGENERATOR_H

#include <filesystem>
#include <string>

struct ProjectGeneratorOptions
{
    std::string projectName;
    std::filesystem::path parentDirectory;
    std::filesystem::path defaultProjectsRoot;
    std::filesystem::path templateProjectRoot;
    std::filesystem::path sampleAssetRoot;
    bool copySampleAntAssets = false;
};

bool GeneratePipeFrameProject(
    const ProjectGeneratorOptions& options,
    std::filesystem::path& outProjectFilePath,
    std::string& outError
);

#endif // PIPEFRAME_PROJECTGENERATOR_H

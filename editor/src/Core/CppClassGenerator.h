#ifndef PIPEFRAME_CPPCLASSGENERATOR_H
#define PIPEFRAME_CPPCLASSGENERATOR_H

#include <filesystem>
#include <string>

#include "Core/EditorCommands.h"
#include "Project/ProjectConfig.h"

struct CppClassGenerationResult
{
    bool success = false;
    std::filesystem::path generatedFilePath;
    std::string message;
};

CppClassGenerationResult GenerateProjectCppClass(
    const ProjectConfig& projectConfig,
    CppClassKind kind,
    const std::string& requestedClassName
);

#endif // PIPEFRAME_CPPCLASSGENERATOR_H

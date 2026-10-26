#ifndef PIPEFRAME_EDITOR_PROJECT_SCAFFOLDER_H
#define PIPEFRAME_EDITOR_PROJECT_SCAFFOLDER_H

#include <filesystem>
#include <string>
#include <vector>

namespace pipeframe::editor {

enum class GeneratedModuleKind { Component, System, Runtime, EditorExtension, Entity, Behaviour, Brush };

struct ProjectValidationIssue {
    std::filesystem::path path;
    std::string message;
};

class ProjectScaffolder final {
public:
    static bool CreateStandardStructure(const std::filesystem::path &root,
                                        const std::string &projectName,
                                        std::string *error = nullptr);
    static bool AddModule(const std::filesystem::path &root,
                          GeneratedModuleKind kind, const std::string &name,
                          std::string *error = nullptr);
    static bool CreateObjectType(const std::filesystem::path &root,
                                 const std::string &name,
                                 const std::vector<std::string> &componentIds,
                                 std::string *error = nullptr);
    static bool RefreshGeneratedRegistration(const std::filesystem::path &root, std::string *error = nullptr);
    static std::vector<ProjectValidationIssue> Validate(const std::filesystem::path &root);
};

} // namespace pipeframe::editor

#endif

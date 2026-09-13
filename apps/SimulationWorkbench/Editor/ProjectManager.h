#ifndef PIPEFRAME_PROJECT_MANAGER_H
#define PIPEFRAME_PROJECT_MANAGER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "SceneDocument.h"

namespace pipeframe::editor {

struct ProjectManifest {
    static constexpr unsigned int CurrentFormatVersion = 2;
    static constexpr unsigned int OldestSupportedFormatVersion = 1;

    unsigned int formatVersion = CurrentFormatVersion;

    std::string name;

    std::filesystem::path startupScene =
        "Scenes/Main.pfscene";

    // Empty means the project contains no native runtime plugin.
    std::filesystem::path runtimeLibrary;
};

class ProjectManager {
  public:
    explicit ProjectManager(std::filesystem::path recentProjectsFile);

    bool HasActiveProject() const;

    const ProjectManifest *GetActiveManifest() const;

    const std::filesystem::path &GetActiveProjectDirectory() const;

    std::filesystem::path GetActiveManifestPath() const;

    std::filesystem::path GetActiveScenePath() const;

    bool CreateProject(const std::filesystem::path &projectDirectory, std::string projectName,
                       const SceneDocument &initialDocument, std::string *errorMessage = nullptr);

    bool OpenProject(const std::filesystem::path &projectPath, SceneDocument &document,
                     std::string *errorMessage = nullptr);

    bool SetRuntimeLibrary(const std::filesystem::path &path,std::string *errorMessage=nullptr);

    bool SaveActiveScene(const SceneDocument &document, std::string *errorMessage = nullptr);

    std::vector<std::filesystem::path> GetRecentProjects() const;

    static std::filesystem::path FindAvailableProjectDirectory(const std::filesystem::path &projectsDirectory,
                                                               const std::string &preferredName);

  private:
    static constexpr const char *ManifestFileName = "project.pipeframe";

    static void SetError(std::string *errorMessage, std::string message);

    static bool IsSafeRelativePath(const std::filesystem::path &path);

    static bool SaveManifest(const ProjectManifest &manifest, const std::filesystem::path &path,
                             std::string *errorMessage);

    static std::optional<ProjectManifest> LoadManifest(const std::filesystem::path &path, std::string *errorMessage);

    void RecordRecentProject(const std::filesystem::path &manifestPath);

    std::filesystem::path recentProjectsFile;

    std::filesystem::path activeProjectDirectory;
    std::optional<ProjectManifest> activeManifest;
};

} // namespace pipeframe::editor

#endif

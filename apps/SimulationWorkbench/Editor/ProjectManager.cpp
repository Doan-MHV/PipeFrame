#include "ProjectManager.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <system_error>
#include <utility>

#include "SceneSerializer.h"
#include "ProjectScaffolder.h"

namespace pipeframe::editor {

namespace {

constexpr const char *ProjectFileHeader = "PIPEFRAME_PROJECT";
constexpr std::size_t MaximumRecentProjectCount = 10;

std::filesystem::path NormalizeAbsolutePath(const std::filesystem::path &path) {

    std::error_code error;

    const std::filesystem::path absolutePath = std::filesystem::absolute(path, error);

    if (error) {
        return path.lexically_normal();
    }

    return absolutePath.lexically_normal();
}

} // namespace

ProjectManager::ProjectManager(std::filesystem::path recentProjectsFile)
    : recentProjectsFile(std::move(recentProjectsFile)) {}

bool ProjectManager::HasActiveProject() const { return activeManifest.has_value(); }

const ProjectManifest *ProjectManager::GetActiveManifest() const {
    return activeManifest.has_value() ? &*activeManifest : nullptr;
}

const std::filesystem::path &ProjectManager::GetActiveProjectDirectory() const { return activeProjectDirectory; }

std::filesystem::path ProjectManager::GetActiveManifestPath() const {
    if (!HasActiveProject()) {
        return {};
    }

    return activeProjectDirectory / ManifestFileName;
}

std::filesystem::path ProjectManager::GetActiveScenePath() const {
    if (!HasActiveProject()) {
        return {};
    }

    return activeProjectDirectory / activeScene;
}

bool ProjectManager::CreateProject(const std::filesystem::path &projectDirectory, std::string projectName,
                                   const SceneDocument &initialDocument, std::string *errorMessage) {

    if (projectName.empty()) {
        SetError(errorMessage, "Project name cannot be empty.");
        return false;
    }

    if (projectDirectory.empty()) {
        SetError(errorMessage, "Project directory cannot be empty.");
        return false;
    }

    const std::filesystem::path normalizedDirectory = NormalizeAbsolutePath(projectDirectory);

    std::error_code filesystemError;

    if (std::filesystem::exists(normalizedDirectory, filesystemError)) {
        if (filesystemError) {
            SetError(errorMessage, "Could not inspect the project directory: " + filesystemError.message());

            return false;
        }

        if (!std::filesystem::is_directory(normalizedDirectory, filesystemError)) {

            SetError(errorMessage, "The project path exists but is not a directory.");

            return false;
        }

        if (!std::filesystem::is_empty(normalizedDirectory, filesystemError)) {

            SetError(errorMessage, "The project directory is not empty: " + normalizedDirectory.string());

            return false;
        }
    }

    if (!ProjectScaffolder::CreateStandardStructure(normalizedDirectory, projectName, errorMessage)) {
        return false;
    }

    ProjectManifest manifest;

    manifest.name = std::move(projectName);
    manifest.startupScene = "Scenes/Main.pfscene";

    const std::filesystem::path manifestPath = normalizedDirectory / ManifestFileName;

    if (!SaveManifest(manifest, manifestPath, errorMessage)) {
        return false;
    }

    const std::filesystem::path scenePath = normalizedDirectory / manifest.startupScene;

    if (!SceneSerializer::Save(initialDocument, scenePath, errorMessage)) {
        return false;
    }

    activeProjectDirectory = normalizedDirectory;
    activeManifest = std::move(manifest);
    activeScene = activeManifest->startupScene;

    RecordRecentProject(manifestPath);

    return true;
}

bool ProjectManager::OpenProject(const std::filesystem::path &projectPath, SceneDocument &document,
                                 std::string *errorMessage) {

    if (projectPath.empty()) {
        SetError(errorMessage, "Project path cannot be empty.");
        return false;
    }

    std::filesystem::path manifestPath = projectPath;

    std::error_code filesystemError;

    if (std::filesystem::is_directory(manifestPath, filesystemError)) {
        manifestPath /= ManifestFileName;
    }

    if (filesystemError) {
        SetError(errorMessage, "Could not inspect the project path: " + filesystemError.message());

        return false;
    }

    manifestPath = NormalizeAbsolutePath(manifestPath);

    std::optional<ProjectManifest> manifest = LoadManifest(manifestPath, errorMessage);

    if (!manifest.has_value()) {
        return false;
    }

    const std::filesystem::path projectDirectory = manifestPath.parent_path();

    const std::filesystem::path scenePath = projectDirectory / manifest->startupScene;

    std::optional<SceneDocument> loadedDocument = SceneSerializer::Load(scenePath, errorMessage);

    if (!loadedDocument.has_value()) {
        return false;
    }

    document = std::move(*loadedDocument);
    document.MarkClean();

    activeProjectDirectory = projectDirectory;
    activeManifest = std::move(*manifest);
    activeScene = activeManifest->startupScene;

    RecordRecentProject(manifestPath);

    return true;
}

std::vector<std::filesystem::path> ProjectManager::GetScenes() const {
    std::vector<std::filesystem::path> result;
    if (!HasActiveProject()) return result;
    std::error_code ec;
    const auto root = activeProjectDirectory / "Scenes";
    for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".pfscene")
            result.push_back(std::filesystem::path("Scenes") / it->path().filename());
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool ProjectManager::OpenScene(const std::filesystem::path& relative, SceneDocument& document, std::string* error) {
    if (!HasActiveProject() || !IsSafeRelativePath(relative) ||
        relative.parent_path() != "Scenes" || relative.extension() != ".pfscene") {
        SetError(error, "Choose a .pfscene file inside this project's Scenes folder."); return false;
    }
    auto loaded = SceneSerializer::Load(activeProjectDirectory / relative, error);
    if (!loaded) return false;
    document = std::move(*loaded);
    document.MarkClean();
    activeScene = relative;
    return true;
}

bool ProjectManager::CreateScene(const std::string& name, const SceneDocument& document, std::string* error) {
    if (!HasActiveProject() || name.empty() || name == "." || name == ".." ||
        name.find_first_of("/\\:\n\r") != std::string::npos || name.front() == ' ' || name.back() == ' ') {
        SetError(error, "Enter a scene name without slashes, colons or surrounding spaces."); return false;
    }
    auto relative = std::filesystem::path("Scenes") / (name + ".pfscene");
    std::error_code ec;
    if (std::filesystem::exists(activeProjectDirectory / relative, ec) || ec) {
        SetError(error, "A scene with that name already exists, or its path cannot be inspected."); return false;
    }
    if (!SceneSerializer::Save(document, activeProjectDirectory / relative, error)) return false;
    activeScene = relative;
    return true;
}

bool ProjectManager::SaveActiveScene(const SceneDocument &document, std::string *errorMessage) {

    if (!HasActiveProject()) {
        SetError(errorMessage, "There is no active project.");
        return false;
    }

    const std::filesystem::path scenePath = GetActiveScenePath();

    std::error_code directoryError;

    std::filesystem::create_directories(scenePath.parent_path(), directoryError);

    if (directoryError) {
        SetError(errorMessage, "Could not create the scene directory: " + directoryError.message());

        return false;
    }

    if (!SceneSerializer::Save(document, scenePath, errorMessage)) {
        return false;
    }

    RecordRecentProject(GetActiveManifestPath());

    return true;
}

std::vector<std::filesystem::path> ProjectManager::GetRecentProjects() const {

    std::vector<std::filesystem::path> result;

    std::ifstream input(recentProjectsFile);

    if (!input.is_open()) {
        return result;
    }

    std::string pathText;

    while (input >> std::quoted(pathText)) {
        const std::filesystem::path path(pathText);

        std::error_code existsError;

        if (std::filesystem::exists(path, existsError) && !existsError) {

            result.push_back(path);
        }
    }

    return result;
}

std::filesystem::path ProjectManager::FindAvailableProjectDirectory(const std::filesystem::path &projectsDirectory,
                                                                    const std::string &preferredName) {

    const std::string baseName = preferredName.empty() ? "Untitled" : preferredName;

    std::filesystem::path candidate = projectsDirectory / baseName;

    std::error_code existsError;

    if (!std::filesystem::exists(candidate, existsError)) {
        return candidate;
    }

    for (std::size_t number = 2;; ++number) {
        candidate = projectsDirectory / (baseName + " " + std::to_string(number));

        existsError.clear();

        if (!std::filesystem::exists(candidate, existsError)) {
            return candidate;
        }
    }
}

void ProjectManager::SetError(std::string *errorMessage, std::string message) {

    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

bool ProjectManager::IsSafeRelativePath(const std::filesystem::path &path) {

    if (path.empty() || path.is_absolute()) {
        return false;
    }

    const std::filesystem::path normalized = path.lexically_normal();

    for (const std::filesystem::path &component : normalized) {
        if (component == "..") {
            return false;
        }
    }

    return true;
}

bool ProjectManager::SaveManifest(const ProjectManifest &manifest, const std::filesystem::path &path,
                                  std::string *errorMessage) {

    if (manifest.name.empty()) {
        SetError(errorMessage, "Project manifest has an empty name.");

        return false;
    }

    if (!IsSafeRelativePath(manifest.startupScene)) {
        SetError(errorMessage, "Project startup scene must be a safe relative path.");

        return false;
    }

    std::ofstream output(path);

    if (!output.is_open()) {
        SetError(errorMessage, "Could not open project manifest for writing: " + path.string());

        return false;
    }

    output << ProjectFileHeader << ' ' << ProjectManifest::CurrentFormatVersion << '\n';

    output << std::quoted(manifest.name) << '\n';
    output << std::quoted(manifest.startupScene.generic_string()) << '\n';

    output << std::quoted(manifest.runtimeLibrary.generic_string()) << '\n';

    if (!output.good()) {
        SetError(errorMessage, "Failed while writing project manifest: " + path.string());

        return false;
    }

    return true;
}

std::optional<ProjectManifest> ProjectManager::LoadManifest(const std::filesystem::path &path,
                                                            std::string *errorMessage) {

    std::ifstream input(path);

    if (!input.is_open()) {
        SetError(errorMessage, "Could not open project manifest: " + path.string());

        return std::nullopt;
    }

    std::string header;
    unsigned int version = 0;

    if (!(input >> header >> version)) {
        SetError(errorMessage, "Project manifest header is missing or invalid.");

        return std::nullopt;
    }

    if (header != ProjectFileHeader) {
        SetError(errorMessage, "This is not a PipeFrame project manifest.");

        return std::nullopt;
    }

    if (version < ProjectManifest::OldestSupportedFormatVersion || version > ProjectManifest::CurrentFormatVersion) {
        SetError(errorMessage, "Unsupported PipeFrame project version.");

        return std::nullopt;
    }

    ProjectManifest manifest;

    manifest.formatVersion = version;

    std::string startupScene;

    if (!(input >> std::quoted(manifest.name) >> std::quoted(startupScene))) {

        SetError(errorMessage, "Project manifest is incomplete or invalid.");

        return std::nullopt;
    }

    manifest.startupScene = startupScene;

    if (version >= 2) {
        std::string runtimeLibrary;

        if (!(input >> std::quoted(runtimeLibrary))) {
            SetError(
                errorMessage,
                "Project runtime-library entry is invalid.");

            return std::nullopt;
        }

        manifest.runtimeLibrary = runtimeLibrary;
    }

    if (manifest.name.empty()) {
        SetError(errorMessage, "Project manifest has an empty name.");

        return std::nullopt;
    }

    if (!manifest.runtimeLibrary.empty() &&
    !IsSafeRelativePath(
        manifest.runtimeLibrary)) {

        SetError(
            errorMessage,
            "Project manifest contains an unsafe "
            "runtime-library path.");

        return std::nullopt;
        }

    if (!IsSafeRelativePath(manifest.startupScene)) {
        SetError(errorMessage, "Project manifest contains an unsafe startup scene path.");

        return std::nullopt;
    }

    return manifest;
}

void ProjectManager::RecordRecentProject(const std::filesystem::path &manifestPath) {

    const std::filesystem::path normalizedPath = NormalizeAbsolutePath(manifestPath);

    std::vector<std::filesystem::path> recentProjects = GetRecentProjects();

    std::erase_if(recentProjects, [&normalizedPath](const std::filesystem::path &existingPath) {
        return NormalizeAbsolutePath(existingPath) == normalizedPath;
    });

    recentProjects.insert(recentProjects.begin(), normalizedPath);

    if (recentProjects.size() > MaximumRecentProjectCount) {
        recentProjects.resize(MaximumRecentProjectCount);
    }

    std::error_code directoryError;

    std::filesystem::create_directories(recentProjectsFile.parent_path(), directoryError);

    if (directoryError) {
        return;
    }

    std::ofstream output(recentProjectsFile);

    if (!output.is_open()) {
        return;
    }

    for (const std::filesystem::path &path : recentProjects) {
        output << std::quoted(path.string()) << '\n';
    }
}

} // namespace pipeframe::editor

namespace pipeframe::editor {
bool ProjectManager::SetRuntimeLibrary(const std::filesystem::path &path,std::string *error){
    if(!activeManifest||!IsSafeRelativePath(path)){SetError(error,"Runtime path must be project relative");return false;}
    auto next=*activeManifest;next.runtimeLibrary=path;const auto temporary=activeProjectDirectory/"project.pipeframe.build-tmp";
    if(!SaveManifest(next,temporary,error))return false;
    std::error_code ec;std::filesystem::rename(temporary,GetActiveManifestPath(),ec);
    if(ec){SetError(error,ec.message());std::filesystem::remove(temporary,ec);return false;}
    activeManifest=std::move(next);return true;
}
}

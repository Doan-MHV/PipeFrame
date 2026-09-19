#ifndef PIPEFRAME_PROJECT_RUNTIME_LIBRARY_H
#define PIPEFRAME_PROJECT_RUNTIME_LIBRARY_H

#include <PipeFrame/Project/ProjectRuntime.h>

#include <filesystem>
#include <string>

namespace pipeframe {

class ProjectRuntimeLibrary final {
public:
    ProjectRuntimeLibrary() = default;

    ~ProjectRuntimeLibrary();

    ProjectRuntimeLibrary(const ProjectRuntimeLibrary&) = delete;

    ProjectRuntimeLibrary& operator=(const ProjectRuntimeLibrary&) = delete;

    void Swap(ProjectRuntimeLibrary& other) noexcept;

    bool Load(const std::filesystem::path& libraryPath, std::string* errorMessage = nullptr);

    void Unload();

    bool IsLoaded() const;

    ProjectRuntime* GetRuntime();
    const ProjectRuntime* GetRuntime() const;

    const std::filesystem::path& GetLibraryPath() const;

    static std::filesystem::path ResolveLibraryPath(const std::filesystem::path& pathWithoutExtension);

private:
    static void SetError(std::string* errorMessage, std::string message);

    void* libraryHandle = nullptr;

    ProjectRuntime* runtime = nullptr;

    DestroyProjectRuntimeFunction destroyFunction = nullptr;

    std::filesystem::path libraryPath;
};

}  // namespace pipeframe

#endif

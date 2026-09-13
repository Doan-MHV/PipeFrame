#include <PipeFrame/Project/ProjectRuntimeLibrary.h>

#include <array>
#include <system_error>
#include <utility>

#if defined(_WIN32)
    #define NOMINMAX
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

namespace pipeframe {

namespace {

void *OpenLibrary(
    const std::filesystem::path &path) {

#if defined(_WIN32)
    return reinterpret_cast<void *>(
        LoadLibraryW(path.wstring().c_str()));
#else
    return dlopen(
        path.string().c_str(),
        RTLD_NOW | RTLD_LOCAL);
#endif
}

void CloseLibrary(void *handle) {
    if (handle == nullptr) {
        return;
    }

#if defined(_WIN32)
    FreeLibrary(
        reinterpret_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void *FindSymbol(
    void *handle,
    const char *symbolName) {

#if defined(_WIN32)
    return reinterpret_cast<void *>(
        GetProcAddress(
            reinterpret_cast<HMODULE>(handle),
            symbolName));
#else
    return dlsym(handle, symbolName);
#endif
}

std::string GetPlatformError() {
#if defined(_WIN32)
    return "Windows dynamic-library operation failed.";
#else
    const char *message = dlerror();

    return message != nullptr
               ? std::string(message)
               : "Dynamic-library operation failed.";
#endif
}

} // namespace

ProjectRuntimeLibrary::~ProjectRuntimeLibrary() {
    Unload();
}

void ProjectRuntimeLibrary::Swap(ProjectRuntimeLibrary &other) noexcept {
    using std::swap;
    swap(libraryHandle, other.libraryHandle);
    swap(runtime, other.runtime);
    swap(destroyFunction, other.destroyFunction);
    swap(libraryPath, other.libraryPath);
}

bool ProjectRuntimeLibrary::Load(
    const std::filesystem::path &requestedPath,
    std::string *errorMessage) {

    Unload();

    const std::filesystem::path resolvedPath =
        ResolveLibraryPath(requestedPath);

    std::error_code existsError;

    if (!std::filesystem::exists(
            resolvedPath,
            existsError) ||
        existsError) {

        SetError(
            errorMessage,
            "Project runtime library does not exist: " +
                resolvedPath.string());

        return false;
    }

    libraryHandle = OpenLibrary(resolvedPath);

    if (libraryHandle == nullptr) {
        SetError(
            errorMessage,
            "Could not load project runtime library: " +
                resolvedPath.string() +
                ". " +
                GetPlatformError());

        return false;
    }

    const auto createFunction =
        reinterpret_cast<CreateProjectRuntimeFunction>(
            FindSymbol(
                libraryHandle,
                CreateProjectRuntimeSymbol));

    destroyFunction =
        reinterpret_cast<DestroyProjectRuntimeFunction>(
            FindSymbol(
                libraryHandle,
                DestroyProjectRuntimeSymbol));

    if (createFunction == nullptr ||
        destroyFunction == nullptr) {

        SetError(
            errorMessage,
            "Project runtime does not export the required "
            "PipeFrame factory functions.");

        Unload();

        return false;
    }

    runtime = createFunction();

    if (runtime == nullptr) {
        SetError(
            errorMessage,
            "Project runtime factory returned null.");

        Unload();

        return false;
    }

    libraryPath = resolvedPath;

    return true;
}

void ProjectRuntimeLibrary::Unload() {
    if (runtime != nullptr &&
        destroyFunction != nullptr) {

        destroyFunction(runtime);
    }

    runtime = nullptr;
    destroyFunction = nullptr;

    CloseLibrary(libraryHandle);

    libraryHandle = nullptr;
    libraryPath.clear();
}

bool ProjectRuntimeLibrary::IsLoaded() const {
    return libraryHandle != nullptr &&
           runtime != nullptr;
}

ProjectRuntime *
ProjectRuntimeLibrary::GetRuntime() {
    return runtime;
}

const ProjectRuntime *
ProjectRuntimeLibrary::GetRuntime() const {
    return runtime;
}

const std::filesystem::path &
ProjectRuntimeLibrary::GetLibraryPath() const {
    return libraryPath;
}

std::filesystem::path
ProjectRuntimeLibrary::ResolveLibraryPath(
    const std::filesystem::path &pathWithoutExtension) {

#if defined(_WIN32)
    constexpr std::array<const char *,2> extensions{"", ".dll"};
#elif defined(__APPLE__)
    constexpr std::array<const char *,3> extensions{"", ".dylib", ".so"};
#else
    constexpr std::array<const char *,2> extensions{"", ".so"};
#endif
    // Manifests remain configuration-independent: Build/Foo resolves to
    // Build/Debug/Foo (or Release) before considering legacy flat outputs.
    const auto configured=pathWithoutExtension.parent_path()/PIPEFRAME_RUNTIME_CONFIGURATION/pathWithoutExtension.filename();
    for(const auto &base:std::array{configured,pathWithoutExtension}) {
        for(const auto *extension:extensions) {
            auto candidate=base; candidate+=extension;
            std::error_code error;
            if(std::filesystem::is_regular_file(candidate,error) && !error) return candidate;
        }
    }
    return pathWithoutExtension;
}

void ProjectRuntimeLibrary::SetError(
    std::string *errorMessage,
    std::string message) {

    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

} // namespace pipeframe

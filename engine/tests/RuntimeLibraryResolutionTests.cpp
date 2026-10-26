#include <PipeFrame/Project/ProjectRuntimeLibrary.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        ("pipeframe-runtime-resolution-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root / PIPEFRAME_TEST_CONFIGURATION);
#if defined(_WIN32)
    const char *name = "Fixture.dll";
#elif defined(__APPLE__)
    const char *name = "Fixture.dylib";
#else
    const char *name = "Fixture.so";
#endif
    const auto flat = root / name, matching = root / PIPEFRAME_TEST_CONFIGURATION / name;
    std::ofstream(flat) << "legacy";
    std::ofstream(matching) << "matching configuration";
    const bool preferred = pipeframe::ProjectRuntimeLibrary::ResolveLibraryPath(root / "Fixture") == matching;
    const bool explicitPath = pipeframe::ProjectRuntimeLibrary::ResolveLibraryPath(matching) == matching;
    std::filesystem::remove(matching);
    const bool fallback = pipeframe::ProjectRuntimeLibrary::ResolveLibraryPath(root / "Fixture") == flat;
    std::filesystem::remove_all(root);
    if (!preferred || !explicitPath || !fallback) {
        std::cerr << "Runtime configuration resolution failed\n";
        return 1;
    }
    std::cout << "Runtime configuration resolution passed\n";
}

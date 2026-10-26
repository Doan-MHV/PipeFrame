#ifndef PIPEFRAME_NATIVE_PROJECT_DIALOG_H
#define PIPEFRAME_NATIVE_PROJECT_DIALOG_H

#include <filesystem>
#include <optional>

namespace pipeframe::editor {

class NativeProjectDialog final {
public:
    static std::optional<std::filesystem::path>
    SelectProjectDirectory();

    static std::optional<std::filesystem::path>
    SelectNewProjectDirectory();

    static std::optional<std::filesystem::path>
    SelectAssetFile();

private:
    static std::optional<std::filesystem::path>
    SelectDirectory();
};

} // namespace pipeframe::editor

#endif

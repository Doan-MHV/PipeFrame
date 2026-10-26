#include "NativeProjectDialog.h"

#include <nfd.h>

namespace pipeframe::editor {

namespace {

class NativeDialogSession final {
public:
    NativeDialogSession()
        : initialized(
              NFD_Init() == NFD_OKAY) {}

    ~NativeDialogSession() {
        if (initialized) {
            NFD_Quit();
        }
    }

    bool IsInitialized() const {
        return initialized;
    }

private:
    bool initialized = false;
};

} // namespace

std::optional<std::filesystem::path>
NativeProjectDialog::SelectProjectDirectory() {
    return SelectDirectory();
}

std::optional<std::filesystem::path>
NativeProjectDialog::SelectNewProjectDirectory() {
    return SelectDirectory();
}

std::optional<std::filesystem::path> NativeProjectDialog::SelectAssetFile() {
    NativeDialogSession session;
    if (!session.IsInitialized()) return std::nullopt;
    nfdchar_t *selectedPath = nullptr;
    const nfdresult_t result = NFD_OpenDialog(&selectedPath, nullptr, 0, nullptr);
    if (result != NFD_OKAY || selectedPath == nullptr) return std::nullopt;
    std::filesystem::path path(selectedPath); NFD_FreePath(selectedPath); return path;
}

std::optional<std::filesystem::path>
NativeProjectDialog::SelectDirectory() {
    NativeDialogSession session;

    if (!session.IsInitialized()) {
        return std::nullopt;
    }

    nfdchar_t *selectedPath = nullptr;

    const nfdresult_t result =
        NFD_PickFolder(
            &selectedPath,
            nullptr);

    if (result != NFD_OKAY ||
        selectedPath == nullptr) {

        return std::nullopt;
        }

    std::filesystem::path resultPath(
        selectedPath);

    NFD_FreePath(selectedPath);

    return resultPath;
}

} // namespace pipeframe::editor

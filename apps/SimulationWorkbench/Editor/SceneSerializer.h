#ifndef PIPEFRAME_SCENE_SERIALIZER_H
#define PIPEFRAME_SCENE_SERIALIZER_H

#include "SceneDocument.h"

#include <filesystem>
#include <optional>
#include <string>

namespace pipeframe::editor {

class SceneSerializer {
public:
    static bool Save(
        const SceneDocument &document,
        const std::filesystem::path &path,
        std::string *errorMessage = nullptr);

    static std::optional<SceneDocument> Load(
        const std::filesystem::path &path,
        std::string *errorMessage = nullptr);
};

} // namespace pipeframe::editor

#endif
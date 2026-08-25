#ifndef PIPEFRAME_SCENE_SERIALIZER_H
#define PIPEFRAME_SCENE_SERIALIZER_H

#include <filesystem>
#include <optional>
#include <string>

#include "SceneDocument.h"

namespace pipeframe::editor {

class SceneSerializer {
  public:
    static bool Save(const SceneDocument &document, const std::filesystem::path &path,
                     std::string *errorMessage = nullptr);

    static std::optional<SceneDocument> Load(const std::filesystem::path &path, std::string *errorMessage = nullptr);
};

} // namespace pipeframe::editor

#endif
#ifndef PIPEFRAME_EDITOR_PREFAB_LIBRARY_H
#define PIPEFRAME_EDITOR_PREFAB_LIBRARY_H

#include "SceneDocument.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pipeframe::editor {

struct PrefabDefinition {
    std::string id;
    std::uint32_t revision{1};
    std::string basePrefabId;
    SceneDocument source;
};

struct PrefabInstantiation {
    SceneObjectId rootObjectId{};
    std::vector<SceneObjectId> objectIds;
};

enum class PrefabOverrideKind {
    Name,
    Transform,
    ObjectProperties,
    Components,
    AddedObject,
    MissingObject,
};

struct PrefabOverride {
    SceneObjectId sourceObjectId{};
    SceneObjectId instanceObjectId{};
    PrefabOverrideKind kind{PrefabOverrideKind::Transform};
    std::string path;
};

struct PrefabConflict {
    SceneObjectId sourceObjectId{};
    std::string message;
};

class PrefabLibrary final {
public:
    static constexpr std::uint32_t CurrentFormatVersion = 1;

    std::optional<PrefabDefinition> Create(const std::string &id,
                                           const SceneDocument &scene,
                                           SceneObjectId rootObjectId,
                                           std::string *error = nullptr) const;

    std::optional<PrefabDefinition> CreateVariant(const std::string &id,
                                                  const PrefabDefinition &base,
                                                  SceneDocument customizedSource,
                                                  std::string *error = nullptr) const;

    bool Save(const PrefabDefinition &prefab, const std::filesystem::path &path,
              std::string *error = nullptr) const;
    std::optional<PrefabDefinition> Load(const std::filesystem::path &path,
                                         std::string *error = nullptr) const;

    std::optional<PrefabInstantiation> Instantiate(const PrefabDefinition &prefab,
                                                   SceneDocument &scene,
                                                   SceneObjectId parentId = 0,
                                                   SceneTransform placement = {},
                                                   std::string *error = nullptr) const;
    bool AdoptInstance(const PrefabDefinition &prefab, SceneDocument &scene,
                       SceneObjectId rootObjectId, std::string *error = nullptr) const;

    std::vector<PrefabOverride> GetOverrides(const PrefabDefinition &prefab,
                                             const SceneDocument &scene,
                                             SceneObjectId instanceRootId) const;
    std::vector<PrefabConflict> GetConflicts(const PrefabDefinition &prefab,
                                             const SceneDocument &scene,
                                             SceneObjectId instanceRootId) const;

    bool ApplyInstance(PrefabDefinition &prefab, const SceneDocument &scene,
                       SceneObjectId instanceRootId, std::string *error = nullptr) const;
    bool RevertInstance(const PrefabDefinition &prefab, SceneDocument &scene,
                        SceneObjectId instanceRootId, std::string *error = nullptr) const;
    bool UnpackInstance(SceneDocument &scene, SceneObjectId instanceRootId,
                        std::string *error = nullptr) const;

private:
    static void SetError(std::string *error, std::string message);
};

} // namespace pipeframe::editor

#endif

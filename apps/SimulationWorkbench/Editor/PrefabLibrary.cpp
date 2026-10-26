#include "PrefabLibrary.h"

#include "SceneSerializer.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

namespace pipeframe::editor {
namespace {

using IdMap = std::unordered_map<SceneObjectId, SceneObjectId>;

const PrefabInstanceLink *FindLink(const SceneObjectData &object,
                                   const std::string &prefabId,
                                   const SceneObjectId instanceRootId) {
    const auto link = std::ranges::find_if(object.prefabLinks, [&](const auto &item) {
        return item.prefabId == prefabId && item.instanceRootId == instanceRootId;
    });
    return link == object.prefabLinks.end() ? nullptr : &*link;
}

void RemapValue(PropertyValue &value, const IdMap &ids) {
    auto *reference = std::get_if<SceneObjectReference>(&value);
    if (!reference) return;
    const auto mapped = ids.find(reference->objectId);
    if (mapped != ids.end()) reference->objectId = mapped->second;
}

void RemapObjectReferences(SceneObjectData &object, const IdMap &ids) {
    for (auto &[key, value] : object.properties) {
        (void)key;
        RemapValue(value, ids);
    }
    for (auto &component : object.components)
        for (auto &[key, value] : component.properties) {
            (void)key;
            RemapValue(value, ids);
        }
    for (auto &link : object.prefabLinks) {
        if (const auto mapped = ids.find(link.instanceRootId); mapped != ids.end())
            link.instanceRootId = mapped->second;
    }
}

std::vector<const SceneObjectData *> CollectSubtree(const SceneDocument &scene,
                                                    const SceneObjectId rootId) {
    std::vector<const SceneObjectData *> result;
    if (!scene.FindObject(rootId)) return result;
    std::vector<SceneObjectId> pending{rootId};
    while (!pending.empty()) {
        const auto id = pending.back();
        pending.pop_back();
        const auto *object = scene.FindObject(id);
        if (!object) continue;
        result.push_back(object);
        const auto children = scene.GetChildren(id);
        for (auto iterator = children.rbegin(); iterator != children.rend(); ++iterator)
            pending.push_back((*iterator)->id);
    }
    return result;
}

const SceneObjectData *FindSource(const PrefabDefinition &prefab, const SceneObjectId id) {
    return prefab.source.FindObject(id);
}

bool SameComponents(std::vector<SceneComponentData> a,
                    std::vector<SceneComponentData> b,
                    const bool ignoreTransform) {
    if (ignoreTransform) {
        std::erase_if(a, [](const auto &component) { return component.typeId == Transform2DComponentTypeId; });
        std::erase_if(b, [](const auto &component) { return component.typeId == Transform2DComponentTypeId; });
    }
    return a == b;
}

std::filesystem::path SourcePathFor(const std::filesystem::path &manifestPath) {
    auto path = manifestPath;
    path += ".scene";
    return path;
}

} // namespace

void PrefabLibrary::SetError(std::string *error, std::string message) {
    if (error) *error = std::move(message);
}

std::optional<PrefabDefinition> PrefabLibrary::Create(const std::string &id,
                                                      const SceneDocument &scene,
                                                      const SceneObjectId rootObjectId,
                                                      std::string *error) const {
    if (id.empty()) {
        SetError(error, "Prefab ID is required.");
        return std::nullopt;
    }
    const auto objects = CollectSubtree(scene, rootObjectId);
    if (objects.empty()) {
        SetError(error, "Prefab root does not exist.");
        return std::nullopt;
    }

    PrefabDefinition result{id, 1, {}, {}};
    result.source.SetSettings(scene.GetSettings());
    IdMap ids;
    for (std::size_t index = 0; index < objects.size(); ++index)
        ids.emplace(objects[index]->id, static_cast<SceneObjectId>(index + 1));

    for (const auto *source : objects) {
        SceneObjectData copy = *source;
        copy.id = ids.at(source->id);
        copy.parentId = source->id == rootObjectId ? 0 : ids.at(source->parentId);
        if (source->id == rootObjectId) copy.transform = {};
        RemapObjectReferences(copy, ids);
        if (!result.source.RestoreObject(std::move(copy))) {
            SetError(error, "Prefab source contains invalid hierarchy data.");
            return std::nullopt;
        }
    }
    for (const auto &connection : scene.GetConnections()) {
        const auto from = ids.find(connection.from.objectId);
        const auto to = ids.find(connection.to.objectId);
        if (from == ids.end() || to == ids.end()) continue;
        auto copy = connection;
        copy.from.objectId = from->second;
        copy.to.objectId = to->second;
        if (!result.source.AddConnection(std::move(copy))) {
            SetError(error, "Prefab contains an invalid internal connection.");
            return std::nullopt;
        }
    }
    result.source.MarkClean();
    return result;
}

std::optional<PrefabDefinition> PrefabLibrary::CreateVariant(const std::string &id,
                                                             const PrefabDefinition &base,
                                                             SceneDocument customizedSource,
                                                             std::string *error) const {
    if (id.empty() || id == base.id || customizedSource.GetObjects().empty()) {
        SetError(error, "A variant needs a unique ID and a non-empty source.");
        return std::nullopt;
    }
    customizedSource.MarkClean();
    return PrefabDefinition{id, 1, base.id, std::move(customizedSource)};
}

bool PrefabLibrary::Save(const PrefabDefinition &prefab,
                         const std::filesystem::path &path,
                         std::string *error) const {
    if (prefab.id.empty() || prefab.revision == 0 || prefab.source.GetObjects().empty()) {
        SetError(error, "Prefab definition is invalid.");
        return false;
    }
    std::error_code filesystemError;
    std::filesystem::create_directories(path.parent_path(), filesystemError);
    if (filesystemError) {
        SetError(error, "Could not create the prefab directory: " + filesystemError.message());
        return false;
    }
    const auto sourcePath = SourcePathFor(path);
    auto temporarySource = sourcePath;
    temporarySource += ".tmp";
    auto temporaryManifest = path;
    temporaryManifest += ".tmp";
    if (!SceneSerializer::Save(prefab.source, temporarySource, error)) return false;
    std::ofstream output(temporaryManifest);
    if (!output.is_open()) {
        SetError(error, "Could not open prefab manifest for writing: " + temporaryManifest.string());
        return false;
    }
    output << "PIPEFRAME_PREFAB " << CurrentFormatVersion << '\n'
           << std::quoted(prefab.id) << ' ' << prefab.revision << ' '
           << std::quoted(prefab.basePrefabId) << ' '
           << std::quoted(sourcePath.filename().string()) << '\n';
    if (!output.good()) {
        SetError(error, "Could not write prefab manifest: " + path.string());
        return false;
    }
    output.close();
    std::filesystem::rename(temporarySource, sourcePath, filesystemError);
    if (filesystemError) {
        SetError(error, "Could not replace prefab source: " + filesystemError.message());
        return false;
    }
    std::filesystem::rename(temporaryManifest, path, filesystemError);
    if (filesystemError) {
        SetError(error, "Could not replace prefab manifest: " + filesystemError.message());
        return false;
    }
    return true;
}

std::optional<PrefabDefinition> PrefabLibrary::Load(const std::filesystem::path &path,
                                                    std::string *error) const {
    std::ifstream input(path);
    std::string header, id, baseId, sourceFile;
    std::uint32_t version = 0, revision = 0;
    if (!input.is_open() || !(input >> header >> version) || header != "PIPEFRAME_PREFAB" ||
        version != CurrentFormatVersion ||
        !(input >> std::quoted(id) >> revision >> std::quoted(baseId) >> std::quoted(sourceFile)) ||
        id.empty() || revision == 0 || sourceFile.empty()) {
        SetError(error, "Prefab manifest is invalid: " + path.string());
        return std::nullopt;
    }
    auto source = SceneSerializer::Load(path.parent_path() / sourceFile, error);
    if (!source || source->GetObjects().empty()) return std::nullopt;
    return PrefabDefinition{std::move(id), revision, std::move(baseId), std::move(*source)};
}

std::optional<PrefabInstantiation> PrefabLibrary::Instantiate(const PrefabDefinition &prefab,
                                                              SceneDocument &scene,
                                                              const SceneObjectId parentId,
                                                              const SceneTransform placement,
                                                              std::string *error) const {
    const auto roots = prefab.source.GetChildren(0);
    if (prefab.id.empty() || roots.size() != 1 || (parentId != 0 && !scene.FindObject(parentId))) {
        SetError(error, "Prefab must contain exactly one root and use a valid destination parent.");
        return std::nullopt;
    }
    const auto sourceObjects = CollectSubtree(prefab.source, roots.front()->id);
    IdMap ids;
    PrefabInstantiation result;
    for (const auto *source : sourceObjects) {
        SceneObjectData copy = *source;
        const auto sourceId = source->id;
        copy.parentId = sourceId == roots.front()->id ? parentId : ids.at(source->parentId);
        if (sourceId == roots.front()->id) copy.transform = placement;
        copy.id = 0;
        const auto created = scene.CreateObject(std::move(copy));
        if (created == 0) {
            SetError(error, "Could not create a prefab instance object.");
            return std::nullopt;
        }
        ids.emplace(sourceId, created);
        result.objectIds.push_back(created);
        if (sourceId == roots.front()->id) result.rootObjectId = created;
    }
    for (const auto *source : sourceObjects) {
        const auto instanceId = ids.at(source->id);
        SceneObjectData replacement = *source;
        replacement.id = instanceId;
        replacement.parentId = source->parentId == 0 ? parentId : ids.at(source->parentId);
        if (source->parentId == 0) replacement.transform = placement;
        RemapObjectReferences(replacement, ids);
        auto links = replacement.prefabLinks;
        for (auto &link : links)
            if (const auto mapped = ids.find(link.instanceRootId); mapped != ids.end())
                link.instanceRootId = mapped->second;
        links.push_back({prefab.id, prefab.revision, source->id, result.rootObjectId, prefab.basePrefabId});
        replacement.prefabLinks = std::move(links);
        if (!scene.ReplaceObjectData(instanceId, std::move(replacement))) {
            SetError(error, "Could not finalize prefab instance data.");
            return std::nullopt;
        }
    }
    for (const auto &connection : prefab.source.GetConnections()) {
        auto copy = connection;
        copy.id = 1;
        while (std::ranges::any_of(scene.GetConnections(), [&](const auto &item) { return item.id == copy.id; })) ++copy.id;
        copy.from.objectId = ids.at(connection.from.objectId);
        copy.to.objectId = ids.at(connection.to.objectId);
        if (!scene.AddConnection(std::move(copy))) {
            SetError(error, "Could not create a prefab instance connection.");
            return std::nullopt;
        }
    }
    return result;
}

bool PrefabLibrary::AdoptInstance(const PrefabDefinition &prefab, SceneDocument &scene,
                                  const SceneObjectId rootId, std::string *error) const {
    const auto sourceRoots = prefab.source.GetChildren(0);
    const auto instances = CollectSubtree(scene, rootId);
    if (sourceRoots.size() != 1) {
        SetError(error, "Prefab source must contain exactly one root.");
        return false;
    }
    const auto sources = CollectSubtree(prefab.source, sourceRoots.front()->id);
    if (sources.size() != instances.size()) {
        SetError(error, "Selected hierarchy no longer matches the prefab source.");
        return false;
    }
    for (std::size_t index = 0; index < instances.size(); ++index) {
        auto links = instances[index]->prefabLinks;
        links.push_back({prefab.id, prefab.revision, sources[index]->id, rootId, prefab.basePrefabId});
        if (!scene.SetPrefabLinks(instances[index]->id, std::move(links))) {
            SetError(error, "Could not attach prefab identity to the selected hierarchy.");
            return false;
        }
    }
    return true;
}

std::vector<PrefabOverride> PrefabLibrary::GetOverrides(const PrefabDefinition &prefab,
                                                        const SceneDocument &scene,
                                                        const SceneObjectId rootId) const {
    std::vector<PrefabOverride> result;
    std::unordered_set<SceneObjectId> foundSources;
    for (const auto &instance : scene.GetObjects()) {
        const auto *link = FindLink(instance, prefab.id, rootId);
        if (!link) continue;
        foundSources.insert(link->sourceObjectId);
        const auto *source = FindSource(prefab, link->sourceObjectId);
        if (!source) {
            result.push_back({link->sourceObjectId, instance.id, PrefabOverrideKind::AddedObject, "object"});
            continue;
        }
        if (instance.name != source->name)
            result.push_back({source->id, instance.id, PrefabOverrideKind::Name, "name"});
        const bool isRoot = instance.id == rootId;
        if (!isRoot && instance.transform != source->transform)
            result.push_back({source->id, instance.id, PrefabOverrideKind::Transform, "transform"});
        if (instance.properties != source->properties)
            result.push_back({source->id, instance.id, PrefabOverrideKind::ObjectProperties, "properties"});
        if (!SameComponents(instance.components, source->components, isRoot))
            result.push_back({source->id, instance.id, PrefabOverrideKind::Components, "components"});
    }
    for (const auto &source : prefab.source.GetObjects())
        if (!foundSources.contains(source.id))
            result.push_back({source.id, 0, PrefabOverrideKind::MissingObject, "object"});
    return result;
}

std::vector<PrefabConflict> PrefabLibrary::GetConflicts(const PrefabDefinition &prefab,
                                                        const SceneDocument &scene,
                                                        const SceneObjectId rootId) const {
    std::vector<PrefabConflict> result;
    std::unordered_set<SceneObjectId> sourceIds;
    for (const auto &source : prefab.source.GetObjects()) sourceIds.insert(source.id);
    for (const auto &instance : scene.GetObjects()) {
        const auto *link = FindLink(instance, prefab.id, rootId);
        if (!link) continue;
        if (!sourceIds.contains(link->sourceObjectId))
            result.push_back({link->sourceObjectId, "Instance object no longer exists in the prefab source."});
        else if (link->sourceRevision != prefab.revision)
            result.push_back({link->sourceObjectId, "Instance was authored from a different prefab revision."});
    }
    return result;
}

bool PrefabLibrary::ApplyInstance(PrefabDefinition &prefab, const SceneDocument &scene,
                                  const SceneObjectId rootId, std::string *error) const {
    const auto *root = scene.FindObject(rootId);
    if (!root || !FindLink(*root, prefab.id, rootId)) {
        SetError(error, "Selected object is not the root of this prefab instance.");
        return false;
    }
    auto updated = Create(prefab.id, scene, rootId, error);
    if (!updated) return false;
    updated->revision = prefab.revision + 1;
    updated->basePrefabId = prefab.basePrefabId;
    // Applying the outer instance must not bake that instance link back into its source.
    for (const auto &object : updated->source.GetObjects()) {
        auto links = object.prefabLinks;
        std::erase_if(links, [&](const auto &link) { return link.prefabId == prefab.id; });
        updated->source.SetPrefabLinks(object.id, std::move(links));
    }
    updated->source.MarkClean();
    prefab = std::move(*updated);
    return true;
}

bool PrefabLibrary::RevertInstance(const PrefabDefinition &prefab, SceneDocument &scene,
                                   const SceneObjectId rootId, std::string *error) const {
    const auto *root = scene.FindObject(rootId);
    if (!root || !FindLink(*root, prefab.id, rootId)) {
        SetError(error, "Selected object is not the root of this prefab instance.");
        return false;
    }
    const SceneTransform placement = root->transform;
    const SceneObjectId destinationParent = root->parentId;
    std::unordered_map<SceneObjectId, SceneObjectId> sourceToInstance;
    std::vector<SceneObjectId> stale;
    for (const auto &object : scene.GetObjects()) {
        if (const auto *link = FindLink(object, prefab.id, rootId)) {
            sourceToInstance.emplace(link->sourceObjectId, object.id);
            if (!prefab.source.FindObject(link->sourceObjectId)) stale.push_back(object.id);
        }
    }
    for (const auto id : stale) if (scene.FindObject(id)) scene.RemoveObject(id);

    const auto sourceRoots = prefab.source.GetChildren(0);
    if (sourceRoots.size() != 1) {
        SetError(error, "Prefab source does not have exactly one root.");
        return false;
    }
    const auto sourceObjects = CollectSubtree(prefab.source, sourceRoots.front()->id);
    for (const auto *source : sourceObjects) {
        if (sourceToInstance.contains(source->id)) continue;
        SceneObjectData copy = *source;
        copy.id = 0;
        copy.parentId = source->parentId == 0 ? destinationParent : sourceToInstance.at(source->parentId);
        const auto created = scene.CreateObject(std::move(copy));
        if (!created) {
            SetError(error, "Could not restore a missing prefab object.");
            return false;
        }
        sourceToInstance.emplace(source->id, created);
    }
    for (const auto *source : sourceObjects) {
        const auto instanceId = sourceToInstance.at(source->id);
        SceneObjectData replacement = *source;
        replacement.id = instanceId;
        replacement.parentId = source->parentId == 0 ? destinationParent : sourceToInstance.at(source->parentId);
        if (source->parentId == 0) replacement.transform = placement;
        RemapObjectReferences(replacement, sourceToInstance);
        for (auto &link : replacement.prefabLinks)
            if (const auto mapped = sourceToInstance.find(link.instanceRootId); mapped != sourceToInstance.end())
                link.instanceRootId = mapped->second;
        replacement.prefabLinks.push_back({prefab.id, prefab.revision, source->id, rootId, prefab.basePrefabId});
        if (!scene.ReplaceObjectData(instanceId, std::move(replacement))) {
            SetError(error, "Could not restore prefab instance data.");
            return false;
        }
    }
    std::unordered_set<SceneObjectId> instanceIds;
    for (const auto &[sourceId, instanceId] : sourceToInstance) {
        (void)sourceId;
        instanceIds.insert(instanceId);
    }
    std::vector<std::uint64_t> oldConnectionIds;
    for (const auto &connection : scene.GetConnections())
        if (instanceIds.contains(connection.from.objectId) && instanceIds.contains(connection.to.objectId))
            oldConnectionIds.push_back(connection.id);
    for (const auto id : oldConnectionIds) scene.RemoveConnection(id);
    for (const auto &connection : prefab.source.GetConnections()) {
        auto copy = connection;
        copy.id = 1;
        while (std::ranges::any_of(scene.GetConnections(), [&](const auto &item) { return item.id == copy.id; })) ++copy.id;
        copy.from.objectId = sourceToInstance.at(connection.from.objectId);
        copy.to.objectId = sourceToInstance.at(connection.to.objectId);
        if (!scene.AddConnection(std::move(copy))) {
            SetError(error, "Could not restore prefab instance connections.");
            return false;
        }
    }
    return true;
}

bool PrefabLibrary::UnpackInstance(SceneDocument &scene, const SceneObjectId rootId,
                                   std::string *error) const {
    const auto *root = scene.FindObject(rootId);
    if (!root || root->prefabLinks.empty()) {
        SetError(error, "Selected object is not a prefab instance.");
        return false;
    }
    const auto outer = root->prefabLinks.back();
    bool changed = false;
    std::vector<SceneObjectId> ids;
    for (const auto &object : scene.GetObjects())
        if (FindLink(object, outer.prefabId, rootId)) ids.push_back(object.id);
    for (const auto id : ids) {
        const auto *object = scene.FindObject(id);
        if (!object) continue;
        auto links = object->prefabLinks;
        const auto removed = std::erase_if(links, [&](const auto &link) {
            return link.prefabId == outer.prefabId && link.instanceRootId == rootId;
        });
        if (removed) changed |= scene.SetPrefabLinks(id, std::move(links));
    }
    if (!changed) SetError(error, "Prefab instance contained no linked objects.");
    return changed;
}

} // namespace pipeframe::editor

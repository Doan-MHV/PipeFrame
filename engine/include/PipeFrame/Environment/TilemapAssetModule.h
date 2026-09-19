#pragma once
#include <PipeFrame/Environment/TilemapChunkCache.h>
#include <PipeFrame/Environment/TilemapGeometry.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/Project/ProjectRuntime.h>

#include <fstream>
#include <map>
namespace pipeframe {
class TilemapAssetModule final : public RuntimeModule {
public:
    struct Resource {
        std::uint64_t revision{};
        std::optional<Tilemap2D> map;
        GeometryCommand geometry;
        std::optional<Tileset2D> atlas;
        std::string error;
        TilemapChunkCache chunks;
    };
    std::string_view GetModuleId() const override { return "pipeframe.tilemap-assets"; }
    bool Load(const ProjectRuntimeContext& context, std::string& error) override {
        Unload();
        database = context.services ? context.services->Find<assets::AssetDatabase>() : nullptr;
        log = context.log;
        error.clear();
        return true;
    }
    void Unload() override {
        cache.clear();
        database = nullptr;
        log = nullptr;
    }
    const Resource* Resolve(const AssetReference& reference) {
        if (reference.assetId.empty()) return nullptr;
        const auto* record = database ? database->Find(reference.assetId) : nullptr;
        auto& resource = cache[reference.assetId];
        const auto fail = [&](std::string message) -> const Resource* {
            if (resource.error != message && log) log->Write("tilemap", message);
            resource.error = std::move(message);
            resource.map.reset();
            resource.geometry.vertices.clear();
            return &resource;
        };
        if (!record || record->type != assets::AssetType::Tilemap || record->state != assets::AssetState::Ready) {
            // Missing -> Ready can happen without a content revision change.
            // Do not let a transient availability error poison the decoded cache.
            resource.revision = 0;
            return fail("Tilemap asset is missing, failed or has the wrong type: " + reference.assetId);
        }
        if (resource.revision == record->revision && (resource.map || !resource.error.empty())) return &resource;
        resource.revision = record->revision;
        std::ifstream input(database->GetProjectDirectory() / record->cachePath, std::ios::binary);
        std::string line;
        if (!std::getline(input, line) || line != "PIPEFRAME_COMPILED_ASSET 1")
            return fail("Invalid tilemap cache header: " + reference.assetId);
        if (!std::getline(input, line) || line != "TYPE Tilemap")
            return fail("Invalid tilemap cache type: " + reference.assetId);
        if (!std::getline(input, line) || !line.starts_with("FORMAT ") || !std::getline(input, line) ||
            !line.starts_with("SIZE ") || !std::getline(input, line) || line != "DATA")
            return fail("Invalid tilemap cache: " + reference.assetId);
        std::string error;
        auto map = TilemapSerializer::Load(input, error);
        if (!map) return fail("Cannot decode tilemap " + reference.assetId + ": " + error);
        resource.error.clear();
        resource.map = std::move(map);
        resource.chunks.Update(*resource.map, resource.atlas ? &*resource.atlas : nullptr, true);
        resource.chunks.Flatten(resource.geometry.vertices);
        return &resource;
    }
    const GeometryCommand* Geometry(const AssetReference& id, const Tileset2D* atlas = nullptr) {
        const auto* resolved = Resolve(id);
        if (!resolved || !resolved->map) return nullptr;
        auto& resource = cache[id.assetId];
        std::optional<Tileset2D> desired = atlas ? std::optional{*atlas} : std::nullopt;
        if (resource.atlas != desired) {
            resource.atlas = desired;
            resource.chunks.Update(*resource.map, atlas, true);
            resource.chunks.Flatten(resource.geometry.vertices);
        }
        return &resource.geometry;
    }

private:
    assets::AssetDatabase* database{};  // Host-owned; outlives module.
    ProjectLogSink* log{};
    std::map<std::string, Resource> cache;
};
}  // namespace pipeframe

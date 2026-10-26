#pragma once
#include <PipeFrame/Environment/VisualAssets2D.h>
#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/Render/Canvas.h>
#include <PipeFrame/Resources/ImageData.h>

#include <fstream>
#include <map>

namespace pipeframe {
// CPU authoring definitions and render-thread GPU resources share stable asset IDs.
class VisualAssetModule final : public RuntimeModule {
public:
    struct MaterialResource {
        std::uint64_t revision{}, textureRevision{};
        Material2D value;
        Vector2u textureSize{};
        std::shared_ptr<GraphicsResourceService> resources;
        TextureHandle texture;
        std::string error;
        RenderState State() const {
            RenderState state;
            if (resources && texture) state.texture = {*resources, texture};
            return state;
        }
    };
    struct TilesetResource {
        std::uint64_t revision{};
        Tileset2D value;
        std::string error;
    };
    std::string_view GetModuleId() const override { return "pipeframe.visual-assets"; }
    bool Load(const ProjectRuntimeContext& context, std::string& error) override {
        Unload();
        database = context.services ? context.services->Find<assets::AssetDatabase>() : nullptr;
        log = context.log;
        error.clear();
        return true;
    }
    void Unload() override {
        textures.clear();
        materials.clear();
        tilesets.clear();
        database = nullptr;
        log = nullptr;
    }
    struct TextureResource {
        std::uint64_t revision{};
        Vector2u size{};
        std::shared_ptr<GraphicsResourceService> resources;
        TextureHandle texture;
        std::string error;
        RenderState State() const {
            RenderState state;
            if (resources && texture) state.texture = {*resources, texture};
            return state;
        }
    };
    // Stable asset references and revision caching are shared by built-in and custom renderers.
    const TextureResource* ResolveTexture(const AssetReference& id) {
        if (id.assetId.empty()) return nullptr;
        auto& result = textures[id.assetId];
        const auto* record = Record(id, assets::AssetType::Texture);
        if (!record) {
            result.revision = 0;
            result.texture = {};
            result.resources.reset();
            Fail(result.error, "Texture is missing or not ready: " + id.assetId);
            return &result;
        }
        if (result.revision != record->revision || !result.resources) {
            const auto path = database->GetProjectDirectory() / record->cachePath.parent_path() / "image.png";
            ImageData image;
            std::string error;
            if (!LoadImageData(path, image, error)) {
                Fail(result.error, error);
                return &result;
            }
            auto resources = std::make_shared<GraphicsResourceService>();
            auto texture = resources->LoadTextureWithOptions(path.string(), false, false, &error);
            if (resources->State(texture) != ResourceState::Ready) {
                Fail(result.error, error);
                return &result;
            }
            result.size = image.Size();
            result.resources = std::move(resources);
            result.texture = texture;
            result.revision = record->revision;
        }
        result.error.clear();
        return &result;
    }
    const MaterialResource* ResolveMaterial(const AssetReference& id) {
        if (id.assetId.empty()) return nullptr;
        auto& result = materials[id.assetId];
        const auto* record = Record(id, assets::AssetType::Material);
        if (!record) {
            result.revision = 0;
            Fail(result.error, "Material is missing or not ready: " + id.assetId);
            return &result;
        }
        if (result.revision != record->revision) {
            std::string error;
            auto data = Read<Material2D>(*record, error);
            if (!data) {
                Fail(result.error, error);
                return &result;
            }
            result.value = *data;
            result.revision = record->revision;
            result.textureRevision = 0;
            result.resources.reset();
            result.texture = {};
        }
        if (result.value.texture.assetId.empty()) {
            result.error.clear();
            return &result;
        }
        const auto* textureRecord = Record(result.value.texture, assets::AssetType::Texture);
        if (!textureRecord) {
            result.textureRevision = 0;
            Fail(result.error, "Material texture is missing or not ready: " + result.value.texture.assetId);
            return &result;
        }
        if (result.textureRevision != textureRecord->revision || !result.resources) {
            const auto path = database->GetProjectDirectory() / textureRecord->cachePath.parent_path() / "image.png";
            ImageData image;
            std::string error;
            if (!LoadImageData(path, image, error)) {
                Fail(result.error, "Reimport texture to create decoded cache: " + error);
                return &result;
            }
            auto resources = std::make_shared<GraphicsResourceService>();
            auto texture = resources->LoadTextureWithOptions(path.string(), result.value.smooth, true, &error);
            if (resources->State(texture) != ResourceState::Ready) {
                Fail(result.error, error);
                return &result;
            }
            result.textureSize = image.Size();
            result.resources = std::move(resources);
            result.texture = texture;
            result.textureRevision = textureRecord->revision;
        }
        result.error.clear();
        return &result;
    }
    const TilesetResource* ResolveTileset(const AssetReference& id) {
        if (id.assetId.empty()) return nullptr;
        auto& result = tilesets[id.assetId];
        const auto* record = Record(id, assets::AssetType::Tileset);
        if (!record) {
            result.revision = 0;
            Fail(result.error, "Tileset is missing or not ready: " + id.assetId);
            return &result;
        }
        if (result.revision != record->revision) {
            std::string error;
            auto data = Read<Tileset2D>(*record, error);
            if (!data) {
                Fail(result.error, error);
                return &result;
            }
            result.value = *data;
            result.revision = record->revision;
        }
        const auto* material = ResolveMaterial(result.value.material);
        if (!material || !material->error.empty() || !material->texture) {
            Fail(result.error, "Tileset requires a ready textured material");
            return &result;
        }
        if (!result.value.Fits(material->textureSize)) {
            Fail(result.error, "Atlas regions exceed the texture dimensions");
            return &result;
        }
        result.error.clear();
        return &result;
    }

private:
    template <class T>
    std::optional<T> Read(const assets::AssetRecord& record, std::string& error) {
        std::ifstream input(database->GetProjectDirectory() / record.cachePath);
        std::string line;
        if (!std::getline(input, line) || line != "PIPEFRAME_COMPILED_ASSET 1") {
            error = "Invalid visual asset cache";
            return {};
        }
        for (int i = 0; i < 4; ++i)
            if (!std::getline(input, line)) {
                error = "Truncated visual asset cache";
                return {};
            }
        if (line != "DATA") {
            error = "Invalid visual asset payload";
            return {};
        }
        return LoadVisualAsset<T>(input, error);
    }
    const assets::AssetRecord* Record(const AssetReference& id, assets::AssetType type) const {
        const auto* record = database ? database->Find(id.assetId) : nullptr;
        return record && record->type == type && record->state == assets::AssetState::Ready ? record : nullptr;
    }
    void Fail(std::string& previous, std::string error) {
        if (previous != error && log) log->Write("assets", error);
        previous = std::move(error);
    }
    assets::AssetDatabase* database{};
    ProjectLogSink* log{};
    std::map<std::string, TextureResource> textures;
    std::map<std::string, MaterialResource> materials;
    std::map<std::string, TilesetResource> tilesets;
};
inline Color MultiplyTint(Color a, Color b) {
    return {std::uint8_t(unsigned(a.r) * b.r / 255), std::uint8_t(unsigned(a.g) * b.g / 255),
            std::uint8_t(unsigned(a.b) * b.b / 255), std::uint8_t(unsigned(a.a) * b.a / 255)};
}
}  // namespace pipeframe

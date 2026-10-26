#ifndef PIPEFRAME_RESOURCES_GRAPHICS_RESOURCE_SERVICE_H
#define PIPEFRAME_RESOURCES_GRAPHICS_RESOURCE_SERVICE_H
#include <PipeFrame/Resources/AssetRegistry.h>

#include <cstdint>
#include <memory>
#include <string>
namespace pipeframe {
namespace backend::sfml {
class GraphicsResourceAccess;
}
struct RenderSurfaceTag {};
using RenderSurfaceHandle = AssetHandle<RenderSurfaceTag>;
class GraphicsResourceService {
public:
    GraphicsResourceService();
    ~GraphicsResourceService();
    GraphicsResourceService(GraphicsResourceService&&) noexcept;
    GraphicsResourceService& operator=(GraphicsResourceService&&) noexcept;
    GraphicsResourceService(const GraphicsResourceService&) = delete;
    GraphicsResourceService& operator=(const GraphicsResourceService&) = delete;
    TextureHandle LoadTexture(const std::string& path, bool smooth = true, std::string* error = nullptr);
    TextureHandle LoadTextureWithOptions(const std::string& path, bool smooth, bool repeat,
                                         std::string* error = nullptr);
    FontHandle LoadFont(const std::string& path, std::string* error = nullptr);
    ShaderHandle LoadFragmentShader(const std::string& source, std::string* error = nullptr);
    RenderSurfaceHandle CreateSurface(std::uint32_t width, std::uint32_t height, std::string* error = nullptr);
    void Clear();
    [[nodiscard]] static bool ShadersAvailable();
    [[nodiscard]] ResourceState State(TextureHandle) const;
    [[nodiscard]] ResourceState State(FontHandle) const;
    [[nodiscard]] ResourceState State(ShaderHandle) const;
    [[nodiscard]] ResourceState State(RenderSurfaceHandle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    friend class backend::sfml::GraphicsResourceAccess;
};
}  // namespace pipeframe
#endif

#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <unordered_map>
namespace pipeframe {
struct GraphicsResourceService::Impl {
    template <typename T> struct Item {
        T value;
        ResourceState state{ResourceState::Ready};
    };
    std::unordered_map<std::uint32_t, Item<sf::Texture>> textures;
    std::unordered_map<std::uint32_t, Item<sf::Font>> fonts;
    std::unordered_map<std::uint32_t, Item<sf::Shader>> shaders;
    std::unordered_map<std::uint32_t, Item<std::unique_ptr<sf::RenderTexture>>> surfaces;
    std::unordered_map<std::string, std::uint32_t> textureIds, fontIds, shaderIds;
    std::uint32_t nextTexture{1}, nextFont{1}, nextShader{1}, nextSurface{1};
};
GraphicsResourceService::GraphicsResourceService() : impl(std::make_unique<Impl>()) {}
GraphicsResourceService::~GraphicsResourceService() = default;
GraphicsResourceService::GraphicsResourceService(GraphicsResourceService &&) noexcept = default;
GraphicsResourceService &GraphicsResourceService::operator=(GraphicsResourceService &&) noexcept = default;
TextureHandle GraphicsResourceService::LoadTexture(const std::string &path, bool smooth, std::string *error) {
    return LoadTextureWithOptions(path, smooth, false, error);
}
TextureHandle GraphicsResourceService::LoadTextureWithOptions(const std::string &path, bool smooth, bool repeat,
                                                              std::string *error) {
    const std::string key = path + (smooth ? "#smooth" : "#nearest") + (repeat ? "#repeat" : "#clamp");
    if (const auto cached = impl->textureIds.find(key); cached != impl->textureIds.end())
        return {cached->second, 1};
    const auto id = impl->nextTexture++;
    Impl::Item<sf::Texture> item;
    if (!item.value.loadFromFile(path)) {
        item.state = ResourceState::Failed;
        if (error)
            *error = "Unable to load texture: " + path;
    } else {
        item.value.setSmooth(smooth);
        item.value.setRepeated(repeat);
    }
    impl->textures.emplace(id, std::move(item));
    impl->textureIds.emplace(key, id);
    return {id, 1};
}
FontHandle GraphicsResourceService::LoadFont(const std::string &path, std::string *error) {
    if (const auto cached = impl->fontIds.find(path); cached != impl->fontIds.end())
        return {cached->second, 1};
    const auto id = impl->nextFont++;
    Impl::Item<sf::Font> item;
    if (!item.value.openFromFile(path)) {
        item.state = ResourceState::Failed;
        if (error)
            *error = "Unable to load font: " + path;
    }
    impl->fonts.emplace(id, std::move(item));
    impl->fontIds.emplace(path, id);
    return {id, 1};
}
ShaderHandle GraphicsResourceService::LoadFragmentShader(const std::string &source, std::string *error) {
    if (const auto cached = impl->shaderIds.find(source); cached != impl->shaderIds.end())
        return {cached->second, 1};
    const auto id = impl->nextShader++;
    Impl::Item<sf::Shader> item;
    if (!sf::Shader::isAvailable() || !item.value.loadFromMemory(source, sf::Shader::Type::Fragment)) {
        item.state = ResourceState::Failed;
        if (error)
            *error = "Fragment shader unavailable or invalid";
    }
    impl->shaders.emplace(id, std::move(item));
    impl->shaderIds.emplace(source, id);
    return {id, 1};
}
RenderSurfaceHandle GraphicsResourceService::CreateSurface(std::uint32_t width, std::uint32_t height,
                                                           std::string *error) {
    const auto id = impl->nextSurface++;
    Impl::Item<std::unique_ptr<sf::RenderTexture>> item;
    item.value = std::make_unique<sf::RenderTexture>();
    if (!item.value->resize({width, height})) {
        item.state = ResourceState::Failed;
        if (error)
            *error = "Unable to create render surface";
    }
    impl->surfaces.emplace(id, std::move(item));
    return {id, 1};
}
void GraphicsResourceService::Clear() {
    impl->textures.clear();
    impl->fonts.clear();
    impl->shaders.clear();
    impl->surfaces.clear();
    impl->textureIds.clear();
    impl->fontIds.clear();
    impl->shaderIds.clear();
}
bool GraphicsResourceService::ShadersAvailable() { return sf::Shader::isAvailable(); }
template <class Map> static ResourceState LookupState(const Map &map, std::uint32_t id) {
    const auto it = map.find(id);
    return it == map.end() ? ResourceState::Unloaded : it->second.state;
}
ResourceState GraphicsResourceService::State(TextureHandle h) const { return LookupState(impl->textures, h.index); }
ResourceState GraphicsResourceService::State(FontHandle h) const { return LookupState(impl->fonts, h.index); }
ResourceState GraphicsResourceService::State(ShaderHandle h) const { return LookupState(impl->shaders, h.index); }
ResourceState GraphicsResourceService::State(RenderSurfaceHandle h) const {
    return LookupState(impl->surfaces, h.index);
}
namespace backend::sfml {
sf::Texture *GraphicsResourceAccess::Texture(GraphicsResourceService &s, TextureHandle h) {
    auto it = s.impl->textures.find(h.index);
    return it == s.impl->textures.end() || it->second.state != ResourceState::Ready ? nullptr : &it->second.value;
}
const sf::Texture *GraphicsResourceAccess::Texture(const GraphicsResourceService &s, TextureHandle h) {
    auto it = s.impl->textures.find(h.index);
    return it == s.impl->textures.end() || it->second.state != ResourceState::Ready ? nullptr : &it->second.value;
}
sf::Font *GraphicsResourceAccess::Font(GraphicsResourceService &s, FontHandle h) {
    auto it = s.impl->fonts.find(h.index);
    return it == s.impl->fonts.end() || it->second.state != ResourceState::Ready ? nullptr : &it->second.value;
}
const sf::Font *GraphicsResourceAccess::Font(const GraphicsResourceService &s, FontHandle h) {
    auto it = s.impl->fonts.find(h.index);
    return it == s.impl->fonts.end() || it->second.state != ResourceState::Ready ? nullptr : &it->second.value;
}
sf::Shader *GraphicsResourceAccess::Shader(GraphicsResourceService &s, ShaderHandle h) {
    auto it = s.impl->shaders.find(h.index);
    return it == s.impl->shaders.end() || it->second.state != ResourceState::Ready ? nullptr : &it->second.value;
}
sf::RenderTexture *GraphicsResourceAccess::Surface(GraphicsResourceService &s, RenderSurfaceHandle h) {
    auto it = s.impl->surfaces.find(h.index);
    return it == s.impl->surfaces.end() || it->second.state != ResourceState::Ready ? nullptr : it->second.value.get();
}
} // namespace backend::sfml
} // namespace pipeframe

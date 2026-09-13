#ifndef PIPEFRAME_BACKEND_SFML_GRAPHICS_RESOURCE_ACCESS_H
#define PIPEFRAME_BACKEND_SFML_GRAPHICS_RESOURCE_ACCESS_H
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
namespace pipeframe::backend::sfml {class GraphicsResourceAccess{public:static sf::Texture *Texture(GraphicsResourceService&,TextureHandle);static const sf::Texture *Texture(const GraphicsResourceService&,TextureHandle);static sf::Font *Font(GraphicsResourceService&,FontHandle);static const sf::Font *Font(const GraphicsResourceService&,FontHandle);static sf::Shader *Shader(GraphicsResourceService&,ShaderHandle);static sf::RenderTexture *Surface(GraphicsResourceService&,RenderSurfaceHandle);};}
#endif

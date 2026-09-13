#pragma once
#include <PipeFrame/Render/Canvas.h>
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <PipeFrame/Backend/SFML/VertexAccess.h>
namespace pipeframe::backend::sfml {
inline Canvas MakeCanvas(sf::RenderTarget &target) {
    return Canvas(&target,[](void *opaque,std::span<const Vertex2D> vertices,PrimitiveTopology topology,const RenderState &state) {
        sf::RenderStates native;
        native.blendMode=state.blendMode==BlendMode::Add?sf::BlendAdd:sf::BlendAlpha;
        if(state.texture.resources)native.texture=GraphicsResourceAccess::Texture(*state.texture.resources,state.texture.handle);
        if(state.shader.resources) {
            auto *shader=GraphicsResourceAccess::Shader(const_cast<GraphicsResourceService &>(*state.shader.resources),state.shader.handle);
            if(shader)shader->setUniform("texture",sf::Shader::CurrentTexture);
            native.shader=shader;
        }
        sf::PrimitiveType primitive=sf::PrimitiveType::Triangles;
        switch(topology) {
        case PrimitiveTopology::Points:primitive=sf::PrimitiveType::Points;break;
        case PrimitiveTopology::Lines:primitive=sf::PrimitiveType::Lines;break;
        case PrimitiveTopology::LineStrip:primitive=sf::PrimitiveType::LineStrip;break;
        case PrimitiveTopology::TriangleStrip:primitive=sf::PrimitiveType::TriangleStrip;break;
        default:break;
        }
        DrawVertices(*static_cast<sf::RenderTarget *>(opaque),vertices,primitive,native);
    });
}
}

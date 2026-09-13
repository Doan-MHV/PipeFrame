#pragma once
#include <PipeFrame/Render/RenderTypes.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <span>
#include <array>
#include <cmath>
namespace pipeframe {
struct TextureBinding {
    const GraphicsResourceService *resources{}; TextureHandle handle{};
    TextureBinding()=default;TextureBinding(std::nullptr_t){}
    TextureBinding(const GraphicsResourceService &resources,TextureHandle handle):resources(&resources),handle(handle){}
};
struct ShaderBinding {
    const GraphicsResourceService *resources{}; ShaderHandle handle{};
    ShaderBinding()=default;ShaderBinding(std::nullptr_t){}
    ShaderBinding(const GraphicsResourceService &resources,ShaderHandle handle):resources(&resources),handle(handle){}
};
enum class BlendMode { Alpha, Add };
struct RenderState {
    TextureBinding texture;ShaderBinding shader;BlendMode blendMode{BlendMode::Alpha};
    static const RenderState Default;
};
inline const RenderState RenderState::Default{};
// Non-owning, allocation-free drawing view. Only backend adapters construct it.
class Canvas {
public:
    using Submit=void(*)(void *,std::span<const Vertex2D>,PrimitiveTopology,const RenderState &);
    Canvas(void *target,Submit submit):target(target),submit(submit){}
    void Draw(const Vertex2D *vertices,std::size_t count,PrimitiveTopology topology,const RenderState &state={}) const {
        if(count)submit(target,{vertices,count},topology,state);
    }
    void DrawRing(Vector2f center,float radius,float thickness,Color color) const {
        if(radius<0 || thickness<=0)return;
        std::array<Vertex2D,64*6> vertices;
        for(std::size_t i=0;i<64;++i) {
            const float angle=static_cast<float>(i)*6.28318530718f/64;
            const float next=static_cast<float>(i+1)*6.28318530718f/64;
            const Vector2f a{std::cos(angle),std::sin(angle)},b{std::cos(next),std::sin(next)};
            const Vertex2D innerA{center+a*radius,color,{}},innerB{center+b*radius,color,{}};
            const Vertex2D outerA{center+a*(radius+thickness),color,{}},outerB{center+b*(radius+thickness),color,{}};
            const std::array quad{innerA,outerA,outerB,innerA,outerB,innerB};
            for(std::size_t j=0;j<6;++j)vertices[i*6+j]=quad[j];
        }
        Draw(vertices.data(),vertices.size(),PrimitiveTopology::Triangles);
    }
private:void *target;Submit submit;
};
}

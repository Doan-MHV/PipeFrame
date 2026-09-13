#ifndef PIPEFRAME_RENDER_TYPES_H
#define PIPEFRAME_RENDER_TYPES_H

#include <cstdint>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe {

enum class PrimitiveTopology : std::uint8_t { Points, Lines, LineStrip, Triangles, TriangleStrip };
enum class RenderSpace : std::uint8_t { World, Screen };

struct Vertex2D {
    Vector2f position{};
    Color color{};
    Vector2f textureCoordinate{};
};

struct GeometryCommand {
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    RenderSpace space{RenderSpace::World};
    std::vector<Vertex2D> vertices;
    std::uint64_t textureHandle{};
};

struct RenderFrameInfo {
    Rectanglei pixelViewport{};
    Rectanglef worldViewport{};
    TimeSpan elapsed{TimeSpan::FromSeconds(0.0)};
};

class RenderCommandSink {
public:
    virtual ~RenderCommandSink() = default;
    virtual void Submit(const GeometryCommand &command) = 0;
};

} // namespace pipeframe

#endif

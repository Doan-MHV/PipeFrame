#ifndef PIPEFRAME_RENDER_RENDER_SERVICES_2D_H
#define PIPEFRAME_RENDER_RENDER_SERVICES_2D_H
#include <PipeFrame/Render/RenderTypes.h>
#include <PipeFrame/Resources/AssetRegistry.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace pipeframe {
class VertexBatch2D {
public:
    explicit VertexBatch2D(PrimitiveTopology topology = PrimitiveTopology::Triangles) : topology(topology) {}
    void Clear() { vertices.clear(); }
    void Reserve(std::size_t count) { vertices.reserve(count); }
    void Add(Vertex2D vertex) { vertices.push_back(vertex); }
    void Add(std::span<const Vertex2D> source) { vertices.insert(vertices.end(), source.begin(), source.end()); }
    [[nodiscard]] GeometryCommand Command(TextureHandle texture = {}, RenderSpace space = RenderSpace::World) const {
        return {topology, space, vertices, texture.index};
    }
    [[nodiscard]] std::span<const Vertex2D> Vertices() const { return vertices; }

private:
    PrimitiveTopology topology;
    std::vector<Vertex2D> vertices;
};
struct Path2D {
    std::vector<Vector2f> points;
    Color color{255, 255, 255, 255};
    float thickness{1};
    bool closed{};
};
inline GeometryCommand BuildPathCommand(const Path2D& path, RenderSpace space = RenderSpace::World) {
    GeometryCommand command;
    command.topology = PrimitiveTopology::LineStrip;
    command.space = space;
    for (auto point : path.points)
        command.vertices.push_back({point, path.color, {}});
    if (path.closed && !path.points.empty()) command.vertices.push_back({path.points.front(), path.color, {}});
    return command;
}
using SurfaceHandle = std::uint32_t;
struct SurfaceDescriptor {
    std::uint32_t width{}, height{};
    bool persistent{};
};
class SurfaceRegistry {
public:
    SurfaceHandle Create(SurfaceDescriptor descriptor) {
        const auto id = next++;
        surfaces.emplace_back(id, descriptor);
        return id;
    }
    bool Remove(SurfaceHandle handle) {
        return std::erase_if(surfaces, [handle](const auto& item) { return item.first == handle; }) > 0;
    }
    [[nodiscard]] const SurfaceDescriptor* Find(SurfaceHandle handle) const {
        for (const auto& [id, value] : surfaces)
            if (id == handle) return &value;
        return nullptr;
    }

private:
    SurfaceHandle next{1};
    std::vector<std::pair<SurfaceHandle, SurfaceDescriptor>> surfaces;
};
class PingPongSurface {
public:
    void Initialize(SurfaceRegistry& registry, SurfaceDescriptor descriptor) {
        first = registry.Create(descriptor);
        second = registry.Create(descriptor);
        frontFirst = true;
    }
    void Swap() { frontFirst = !frontFirst; }
    [[nodiscard]] SurfaceHandle Front() const { return frontFirst ? first : second; }
    [[nodiscard]] SurfaceHandle Back() const { return frontFirst ? second : first; }

private:
    SurfaceHandle first{}, second{};
    bool frontFirst{true};
};
enum class EffectKind : std::uint8_t { Copy, BlurHorizontal, BlurVertical, Shadow, Custom };
struct EffectPass {
    EffectKind kind{EffectKind::Copy};
    ShaderHandle shader{};
    SurfaceHandle source{}, destination{};
    float amount{1};
};
class EffectGraph {
public:
    void Add(EffectPass pass) { passes.push_back(pass); }
    void Clear() { passes.clear(); }
    [[nodiscard]] std::span<const EffectPass> Passes() const { return passes; }
    [[nodiscard]] EffectKind ResolvedKind(const EffectPass& pass, bool shadersAvailable) const {
        return !shadersAvailable && (pass.kind == EffectKind::Custom || pass.kind == EffectKind::BlurHorizontal ||
                                     pass.kind == EffectKind::BlurVertical || pass.kind == EffectKind::Shadow)
                   ? EffectKind::Copy
                   : pass.kind;
    }

private:
    std::vector<EffectPass> passes;
};
struct RenderParticle2D {
    Vector2f position{}, velocity{};
    Color color{255, 255, 255, 255};
    float size{1}, lifetime{1};
};
class ParticleSystem2D {
public:
    void Emit(RenderParticle2D particle) { particles.push_back(particle); }
    void Update(float delta) {
        for (auto& particle : particles) {
            particle.position += particle.velocity * delta;
            particle.lifetime -= delta;
        }
        std::erase_if(particles, [](const auto& particle) { return particle.lifetime <= 0; });
    }
    [[nodiscard]] GeometryCommand Command() const {
        GeometryCommand command;
        command.topology = PrimitiveTopology::Points;
        for (const auto& particle : particles)
            command.vertices.push_back({particle.position, particle.color, {}});
        return command;
    }
    [[nodiscard]] std::size_t Size() const { return particles.size(); }

private:
    std::vector<RenderParticle2D> particles;
};
class DebugDraw2D {
public:
    void Line(Vector2f a, Vector2f b, Color color) {
        lines.Add({a, color, {}});
        lines.Add({b, color, {}});
    }
    void Clear() { lines.Clear(); }
    [[nodiscard]] GeometryCommand Command() const { return lines.Command(); }

private:
    VertexBatch2D lines{PrimitiveTopology::Lines};
};
}  // namespace pipeframe
#endif

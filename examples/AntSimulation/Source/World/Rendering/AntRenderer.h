#include <PipeFrame/World/World.h>
#ifndef ANT_RENDERER_H
#define ANT_RENDERER_H

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Resources/GraphicsResourceService.h>

#include "Configuration/AntConfiguration.h"
#include "World/Rendering/AntGeometry.h"
#include "World/Runtime/AntView.h"

namespace ant_simulation {

enum class AntRenderingMode {
    DetailedQuads,
    SimpleQuads,
    Points,
};

struct AntRendererStatistics {
    std::size_t candidateCount{0};
    std::size_t visibleCount{0};
    std::size_t vertexCount{0};

    AntRenderingMode mode{AntRenderingMode::Points};
};

class AntRenderer final : public pipeframe::RenderLayer {
  public:
    static constexpr float PointZoomThreshold{2.0f};

    explicit AntRenderer(const AntConfiguration &configuration);

    bool LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage);

    void SetAutomaticMode(bool enabled);

    void SetMode(AntRenderingMode mode);

    void UpdateGeometry(std::span<const AntView> ants, const pipeframe::Rectanglef &viewport, float zoom);

    void Draw(pipeframe::Canvas target, pipeframe::RenderState states = pipeframe::RenderState::Default) const override;

    [[nodiscard]]
    bool AreAssetsLoaded() const;

    [[nodiscard]]
    AntRenderingMode GetMode() const;

    [[nodiscard]]
    const AntRendererStatistics &GetStatistics() const;

    [[nodiscard]]
    const AntGeometry &GetGeometry() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetPointVertices() const;

  private:
    [[nodiscard]]
    AntRenderingMode SelectMode(float zoom) const;

    [[nodiscard]]
    static bool IsVisible(pipeframe::Vector2f position, const pipeframe::Rectanglef &viewport, float margin);

    static void DrawVertices(pipeframe::Canvas target, std::span<const pipeframe::Vertex2D> vertices,
                             pipeframe::PrimitiveTopology primitiveType, const pipeframe::RenderState &states);

    const AntConfiguration &configuration;

    bool automaticMode{true};
    bool assetsLoaded{false};

    AntRenderingMode mode{AntRenderingMode::Points};

    AntGeometry geometry;
    std::vector<pipeframe::Vertex2D> pointVertices;

    pipeframe::GraphicsResourceService resources;
    pipeframe::TextureHandle circleTexture;
    pipeframe::TextureHandle bodyTexture;
    pipeframe::TextureHandle fullAntTexture;
    pipeframe::TextureHandle legTexture;

    AntRendererStatistics statistics;
};

} // namespace ant_simulation

#endif

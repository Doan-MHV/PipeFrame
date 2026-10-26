#include <PipeFrame/World/World.h>
#ifndef ANT_SHADOW_RENDERER_H
#define ANT_SHADOW_RENDERER_H

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Resources/GraphicsResourceService.h>

#include "World/Rendering/AntGeometry.h"
#include "World/Rendering/AntRenderer.h"

namespace ant_simulation {

class ShadowRenderer final : public pipeframe::RenderLayer {
  public:
    static constexpr pipeframe::Vector2f DefaultOffset{
        0.1f,
        0.1f,
    };

    static constexpr pipeframe::Color DefaultColor{
        8,
        10,
        14,
        105,
    };

    bool LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage);

    void SetEnabled(bool enabled);

    void SetOffset(pipeframe::Vector2f offset);

    void SetColor(pipeframe::Color color);

    void Update(const AntGeometry &sourceGeometry, AntRenderingMode mode);

    void Draw(pipeframe::Canvas target, pipeframe::RenderState states = pipeframe::RenderState::Default) const override;

    [[nodiscard]]
    bool IsEnabled() const;

    [[nodiscard]]
    bool AreAssetsLoaded() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetBodyVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetLegVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetFoodVertices() const;

  private:
    void CopyAndTransform(std::span<const pipeframe::Vertex2D> source,
                          std::vector<pipeframe::Vertex2D> &destination) const;

    static void DrawVertices(pipeframe::Canvas target, std::span<const pipeframe::Vertex2D> vertices,
                             pipeframe::TextureBinding texture, pipeframe::RenderState states);

    bool enabled{true};
    bool assetsLoaded{false};

    AntRenderingMode mode{AntRenderingMode::Points};

    pipeframe::Vector2f offset{DefaultOffset};

    pipeframe::Color color{DefaultColor};

    std::vector<pipeframe::Vertex2D> bodyVertices;
    std::vector<pipeframe::Vertex2D> legVertices;
    std::vector<pipeframe::Vertex2D> foodVertices;

    pipeframe::GraphicsResourceService resources;
    pipeframe::TextureHandle bodyShadowTexture, legShadowTexture, circleTexture, fullAntTexture;
    pipeframe::ShaderHandle alphaMaskShader;
    bool shaderLoaded{false};
};

} // namespace ant_simulation

#endif

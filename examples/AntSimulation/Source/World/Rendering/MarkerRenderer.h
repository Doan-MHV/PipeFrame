#include <PipeFrame/World/World.h>
#ifndef ANT_MARKER_RENDERER_H
#define ANT_MARKER_RENDERER_H

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Resources/GraphicsResourceService.h>

#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

class MarkerRenderer final : public pipeframe::RenderLayer {
  public:
    static constexpr std::size_t UpdateDecimation{20};

    static constexpr float MarkerColorPower{0.025f};

    static constexpr float MarkerHalfSize{1.5f};

    explicit MarkerRenderer(const AntConfiguration &configuration);

    bool LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage);

    void SetEnabled(bool enabled);

    void SetColorPower(float colorPower);

    void Update(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport);

    void ForceUpdate(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport);

    void Draw(pipeframe::Canvas target, pipeframe::RenderState states = pipeframe::RenderState::Default) const override;

    [[nodiscard]]
    bool IsEnabled() const;

    [[nodiscard]]
    bool AreAssetsLoaded() const;

    [[nodiscard]]
    std::size_t GetCandidateCount() const;

    [[nodiscard]]
    std::size_t GetVisibleMarkerCount() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetVertices() const;

    [[nodiscard]]
    pipeframe::Color GetCellColor(const AntWorldCell &cell) const;

  private:
    void AddMarkerQuad(pipeframe::Vector2f center, pipeframe::Color color);

    [[nodiscard]]
    static bool IsVisible(pipeframe::Vector2f position, const pipeframe::Rectanglef &viewport);

    [[nodiscard]]
    static std::uint8_t ToColorChannel(float value);

    const AntConfiguration &configuration;

    bool enabled{false};
    bool assetsLoaded{false};

    float markerColorPower{MarkerColorPower};

    std::size_t frameCount{0};
    std::size_t candidateCount{0};
    std::size_t visibleMarkerCount{0};

    std::vector<pipeframe::Vertex2D> vertices;
    pipeframe::GraphicsResourceService resources;
    pipeframe::TextureHandle markerTexture;
};

} // namespace ant_simulation

#endif

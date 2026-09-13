#include <PipeFrame/World/World.h>
#include <PipeFrame/Environment/PlaygroundGeometry.h>
#ifndef ANT_ENVIRONMENT_RENDERER_H
#define ANT_ENVIRONMENT_RENDERER_H

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Resources/GraphicsResourceService.h>

#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/MarkerRenderer.h"
#include "World/Rendering/WallRenderer.h"
#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {

class EnvironmentRenderer final : public pipeframe::RenderLayer {
public:
    static constexpr pipeframe::Color BackgroundColor{
        22,
        21,
        20,
        255,
    };

    static constexpr pipeframe::Color GridColor{
        36,
        34,
        32,
        255,
    };

    static constexpr pipeframe::Color MajorGridColor{
        48,
        45,
        42,
        255,
    };

    static constexpr pipeframe::Color ColonyShadowColor{
        0,
        0,
        0,
        90,
    };

    static constexpr pipeframe::Vector2f ColonyShadowOffset{
        0.0f,
        1.25f,
    };

    static constexpr float FoodHalfSize{
        0.35f
    };

    static constexpr std::size_t ColonySegmentCount{
        64
    };

    explicit EnvironmentRenderer(
        const AntConfiguration &configuration
    );

    bool LoadAssets(
        const std::filesystem::path &assetRoot,
        std::string &errorMessage
    );

    void SetGridEnabled(bool enabled);
    void SetTerrainVisible(bool value){terrainVisible=value;}
    void SetGround(std::optional<pipeframe::PlaygroundComponent> value,pipeframe::RenderState state={},pipeframe::Vector2f uv={}){authoredGround=std::move(value);groundState=state;groundUv=uv;}

    void SetMarkersEnabled(bool enabled);

    void SetMarkerColorPower(float colorPower);

    void SetWallShadowEnabled(bool enabled);

    void Update(
        const AntEnvironment &environment,
        std::span<const ColonyView> colonies,
        const pipeframe::Rectanglef &viewport
    );

    void Draw(
        pipeframe::Canvas target,
        pipeframe::RenderState states =
            pipeframe::RenderState::Default
    ) const override;

    [[nodiscard]]
    bool AreAssetsLoaded() const;

    [[nodiscard]]
    bool IsGridEnabled() const;

    [[nodiscard]]
    std::size_t GetVisibleFoodCount() const;

    [[nodiscard]]
    std::size_t GetVisibleColonyCount() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetBackgroundVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetGridVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetFoodVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetColonyFillVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetColonyOutlineVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetColonyShadowVertices() const;

    [[nodiscard]]
    const MarkerRenderer &
    GetMarkerRenderer() const;

    [[nodiscard]]
    const WallRenderer &
    GetWallRenderer() const;

private:
    void BuildBackground(
        const AntEnvironment &environment
    );

    void BuildGrid(
        const AntEnvironment &environment
    );

    void BuildFood(
        const AntEnvironment &environment,
        const pipeframe::Rectanglef &viewport
    );

    void BuildColonies(
        std::span<const ColonyView> colonies,
        const pipeframe::Rectanglef &viewport
    );

    static void AddQuad(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f center,
        pipeframe::Vector2f halfSize,
        pipeframe::Color color,
        bool textured
    );

    static void AddCircle(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f center,
        float radius,
        pipeframe::Color color
    );

    static void AddRing(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f center,
        float outerRadius,
        float innerRadius,
        pipeframe::Color color
    );

    [[nodiscard]]
    static bool IsVisible(
        pipeframe::Vector2f center,
        float radius,
        const pipeframe::Rectanglef &viewport
    );

    const AntConfiguration &configuration;

    bool assetsLoaded{false};
    std::optional<pipeframe::PlaygroundComponent> authoredGround;
    pipeframe::RenderState groundState;
    pipeframe::Vector2f groundUv;
    bool terrainVisible{true};
    bool gridEnabled{true};

    std::size_t visibleFoodCount{0};
    std::size_t visibleColonyCount{0};

    pipeframe::GraphicsResourceService resources;
    pipeframe::TextureHandle circleTexture;

    std::vector<pipeframe::Vertex2D> backgroundVertices;
    std::vector<pipeframe::Vertex2D> gridVertices;
    std::vector<pipeframe::Vertex2D> foodVertices;

    std::vector<pipeframe::Vertex2D> colonyFillVertices;
    std::vector<pipeframe::Vertex2D> colonyOutlineVertices;
    std::vector<pipeframe::Vertex2D> colonyShadowVertices;

    MarkerRenderer markerRenderer;
    WallRenderer wallRenderer;
};

} // namespace ant_simulation

#endif

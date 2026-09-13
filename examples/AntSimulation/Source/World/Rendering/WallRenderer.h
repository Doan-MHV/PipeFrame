#include <PipeFrame/World/World.h>
#ifndef ANT_WALL_RENDERER_H
#define ANT_WALL_RENDERER_H

#include <cstddef>
#include <span>
#include <vector>


#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/WallBuilder.h"

namespace ant_simulation {

class WallRenderer final : public pipeframe::RenderLayer {
public:
    static constexpr pipeframe::Color WallColor{
        42,
        39,
        37,
        255,
    };

    static constexpr pipeframe::Color ShadowColor{
        0,
        0,
        0,
        90,
    };

    static constexpr pipeframe::Vector2f ShadowOffset{
        0.15f,
        0.2f,
    };

    void SetShadowEnabled(bool enabled);

    void Rebuild(
        const AntEnvironment &environment,
        const pipeframe::Rectanglef &viewport
    );

    void Draw(
        pipeframe::Canvas target,
        pipeframe::RenderState states =
            pipeframe::RenderState::Default
    ) const override;

    [[nodiscard]]
    bool IsShadowEnabled() const;

    [[nodiscard]]
    std::size_t GetCandidateCount() const;

    [[nodiscard]]
    std::size_t GetVisibleWallCount() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetWallVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetShadowVertices() const;

private:
    void AddWall(
        pipeframe::Vector2f position,
        WallType type
    );

    static void AddShape(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f position,
        WallType type,
        pipeframe::Color color,
        pipeframe::Vector2f offset
    );

    [[nodiscard]]
    static bool IsVisible(
        pipeframe::Vector2f position,
        const pipeframe::Rectanglef &viewport
    );

    bool shadowEnabled{true};

    std::size_t candidateCount{0};
    std::size_t visibleWallCount{0};

    std::vector<pipeframe::Vertex2D> wallVertices;
    std::vector<pipeframe::Vertex2D> shadowVertices;
};

} // namespace ant_simulation

#endif
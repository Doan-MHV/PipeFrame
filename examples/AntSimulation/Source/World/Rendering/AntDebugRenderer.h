#include <PipeFrame/World/World.h>
#ifndef ANT_DEBUG_RENDERER_H
#define ANT_DEBUG_RENDERER_H

#include <cstddef>
#include <optional>
#include <span>
#include <vector>


#include "World/Runtime/AntView.h"
#include "World/Physics/AntBodySystem.h"

namespace ant_simulation {

class AntDebugRenderer final : public pipeframe::RenderLayer {
public:
    static constexpr std::size_t CircleSegmentCount{
        64
    };

    static constexpr std::size_t PhysicsCircleSegmentCount{
        24
    };

    static constexpr float SelectionRadius{
        1.0f
    };

    static constexpr float SelectionThickness{
        0.1f
    };

    static constexpr float TargetRadius{
        0.25f
    };

    static constexpr float PhysicsBodyRadius{
        0.5f
    };

    static constexpr pipeframe::Color SelectionColor{
        255,
        255,
        255,
        255,
    };

    static constexpr pipeframe::Color TargetColor{
        0,
        255,
        0,
        255,
    };

    static constexpr pipeframe::Color MovingPhysicsColor{
        239,
        71,
        111,
        100,
    };

    static constexpr pipeframe::Color StaticPhysicsColor{
        255,
        255,
        255,
        255,
    };

    void SetSelectedAnt(
        std::optional<AntId> antId
    );

    void SetTargetVisible(bool visible);

    void SetPhysicsDebugVisible(bool visible);

    void Update(
        std::span<const AntView> ants,
        std::span<const AntPhysicsBody> bodies,
        const pipeframe::Rectanglef &viewport
    );

    void Draw(
        pipeframe::Canvas target,
        pipeframe::RenderState states =
            pipeframe::RenderState::Default
    ) const override;

    [[nodiscard]]
    std::optional<AntId>
    GetSelectedAnt() const;

    [[nodiscard]]
    bool IsTargetVisible() const;

    [[nodiscard]]
    bool IsPhysicsDebugVisible() const;

    [[nodiscard]]
    std::size_t GetPhysicsCandidateCount() const;

    [[nodiscard]]
    std::size_t GetVisiblePhysicsBodyCount() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetSelectionVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetTargetVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetPhysicsVertices() const;

private:
    static void AddCircle(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f center,
        float radius,
        pipeframe::Color color,
        std::size_t segmentCount
    );

    static void AddRing(
        std::vector<pipeframe::Vertex2D> &vertices,
        pipeframe::Vector2f center,
        float outerRadius,
        float innerRadius,
        pipeframe::Color color,
        std::size_t segmentCount
    );

    [[nodiscard]]
    static bool IsVisible(
        pipeframe::Vector2f center,
        float radius,
        const pipeframe::Rectanglef &viewport
    );

    std::optional<AntId> selectedAntId;

    bool targetVisible{false};
    bool physicsDebugVisible{false};

    std::size_t physicsCandidateCount{0};
    std::size_t visiblePhysicsBodyCount{0};

    std::vector<pipeframe::Vertex2D> selectionVertices;
    std::vector<pipeframe::Vertex2D> targetVertices;
    std::vector<pipeframe::Vertex2D> physicsVertices;
};

} // namespace ant_simulation

#endif
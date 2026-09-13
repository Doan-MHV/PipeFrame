#include <PipeFrame/Render/Canvas.h>
#include <PipeFrame/Render/RenderTypes.h>
#ifndef ANT_GEOMETRY_H
#define ANT_GEOMETRY_H

#include <cstddef>
#include <span>
#include <vector>


#include "World/Runtime/AntView.h"
#include "Configuration/AntConfiguration.h"

namespace ant_simulation {

class AntGeometry {
public:
    static constexpr std::size_t VerticesPerQuad{6};
    static constexpr std::size_t DetailedBodyQuads{3};
    static constexpr std::size_t DetailedLegQuads{6};

    void ResizeDetailed(std::size_t antCount);

    void ResizeSimple(std::size_t antCount);

    void UpdateDetailed(
        const AntView &ant,
        std::size_t antIndex,
        const AntConfiguration &configuration
    );

    void UpdateSimple(
        const AntView &ant,
        std::size_t antIndex,
        const AntConfiguration &configuration
    );

    void ClearDetailed(std::size_t antIndex);

    void ClearSimple(std::size_t antIndex);

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetBodyVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetLegVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D>
    GetFoodVertices() const;

    [[nodiscard]]
    static pipeframe::Color GetAntColor(
        const AntView &ant,
        const AntConfiguration &configuration
    );

private:
    static void WriteQuad(
        std::vector<pipeframe::Vertex2D> &vertices,
        std::size_t quadIndex,
        pipeframe::Vector2f center,
        pipeframe::Vector2f direction,
        pipeframe::Vector2f size,
        pipeframe::Vector2f textureMinimum,
        pipeframe::Vector2f textureMaximum,
        pipeframe::Color color
    );

    static void ClearQuad(
        std::vector<pipeframe::Vertex2D> &vertices,
        std::size_t quadIndex
    );

    void UpdateFood(
        const AntView &ant,
        std::size_t antIndex,
        const AntConfiguration &configuration
    );

    static pipeframe::Vector2f Normalize(
        pipeframe::Vector2f value
    );

    std::vector<pipeframe::Vertex2D> bodyVertices;
    std::vector<pipeframe::Vertex2D> legVertices;
    std::vector<pipeframe::Vertex2D> foodVertices;
};

} // namespace ant_simulation

#endif
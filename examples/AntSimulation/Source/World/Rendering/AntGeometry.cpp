#include <PipeFrame/Render/RenderTypes.h>
#include "World/Rendering/AntGeometry.h"


#include <array>
#include <cmath>

#include "Components/AntPoseComponent.h"
#include "Components/ForagingComponent.h"

namespace ant_simulation {

void AntGeometry::ResizeDetailed(
    const std::size_t antCount
) {
    bodyVertices.resize(
        antCount *
        DetailedBodyQuads *
        VerticesPerQuad);

    legVertices.resize(
        antCount *
        DetailedLegQuads *
        VerticesPerQuad);

    foodVertices.resize(
        antCount *
        VerticesPerQuad);
}

void AntGeometry::ResizeSimple(
    const std::size_t antCount
) {
    bodyVertices.resize(
        antCount *
        VerticesPerQuad);

    legVertices.clear();

    foodVertices.resize(
        antCount *
        VerticesPerQuad);
}

void AntGeometry::UpdateDetailed(
    const AntView &ant,
    const std::size_t antIndex,
    const AntConfiguration &configuration
) {
    const pipeframe::Color color =
        GetAntColor(
            ant,
            configuration);

    const pipeframe::Vector2f bodyDirection =
        Normalize(
            ant.GetDirection());

    const pipeframe::Vector2f headDirection =
        Normalize(
            
                ant.Pose().headDirection.GetDirection());

    const pipeframe::Vector2f tailDirection =
        Normalize(
            
                ant.Pose().tailDirection.GetDirection()) *
        -1.0f;

    constexpr pipeframe::Vector2f BaseBodySize{
        0.38f,
        0.15f,
    };

    constexpr pipeframe::Vector2f MainBodySize{
        BaseBodySize.x * 4.1f,
        BaseBodySize.y * 6.8f,
    };

    const std::size_t bodyQuad =
        antIndex *
        DetailedBodyQuads;

    WriteQuad(
        bodyVertices,
        bodyQuad,
        ant.GetPosition(),
        bodyDirection,
        MainBodySize,
        {149.0f, 96.0f},
        {840.0f, 475.0f},
        color);

    const pipeframe::Vector2f headAnchor =
        ant.GetPosition() +
        bodyDirection *
            (BaseBodySize.x * 0.5f);

    constexpr pipeframe::Vector2f HeadSize{
        0.5f,
        0.5f,
    };

    WriteQuad(
        bodyVertices,
        bodyQuad + 1,
        headAnchor +
            headDirection *
                (HeadSize.x * 0.5f),
        headDirection,
        HeadSize,
        {795.0f, 836.0f},
        {1013.0f, 1024.0f},
        color);

    const pipeframe::Vector2f tailAnchor =
        ant.GetPosition() -
        bodyDirection *
            (BaseBodySize.x * 0.5f);

    constexpr pipeframe::Vector2f TailSize{
        0.45f,
        0.34f,
    };

    WriteQuad(
        bodyVertices,
        bodyQuad + 2,
        tailAnchor +
            tailDirection *
                (TailSize.x * 0.5f),
        tailDirection,
        TailSize,
        {176.0f, 892.0f},
        {0.0f, 1024.0f},
        color);

    for (std::size_t legIndex = 0;
         legIndex <
             DetailedLegQuads;
         ++legIndex) {
        const AntLegPose &leg =
            ant.GetLegs()[legIndex];

        const pipeframe::Vector2f start =
            leg.GetWorldStart(
                ant.GetPosition(),
                ant.GetAngle());

        const pipeframe::Vector2f end =
            leg.GetCurrentWorldEnd();

        const pipeframe::Vector2f legVector =
            end - start;

        const float length =
            std::sqrt(
                legVector.x *
                    legVector.x +
                legVector.y *
                    legVector.y);

        const std::size_t legQuad =
            antIndex *
                DetailedLegQuads +
            legIndex;

        if (length <= 0.0f) {
            ClearQuad(
                legVertices,
                legQuad);

            continue;
        }

        const pipeframe::Vector2f direction =
            legVector /
            length;

        const pipeframe::Vector2f center =
            start +
            direction *
                (length * 0.5f);

        constexpr pipeframe::Vector2f
            LegTextureSize{
                350.0f,
                350.0f,
            };

        const bool mirrored =
            legIndex % 2 == 0;

        WriteQuad(
            legVertices,
            legQuad,
            center,
            direction,
            {length, 0.35f},
            mirrored
                ? pipeframe::Vector2f{
                      0.0f,
                      LegTextureSize.y}
                : pipeframe::Vector2f{},
            mirrored
                ? pipeframe::Vector2f{
                      LegTextureSize.x,
                      0.0f}
                : LegTextureSize,
            color);
    }

    UpdateFood(
        ant,
        antIndex,
        configuration);
}

void AntGeometry::UpdateSimple(
    const AntView &ant,
    const std::size_t antIndex,
    const AntConfiguration &configuration
) {
    WriteQuad(
        bodyVertices,
        antIndex,
        ant.GetPosition(),
        Normalize(
            ant.GetDirection()),
        {2.0f, 2.0f},
        {},
        {1024.0f, 1024.0f},
        GetAntColor(
            ant,
            configuration));

    UpdateFood(
        ant,
        antIndex,
        configuration);
}

void AntGeometry::ClearDetailed(
    const std::size_t antIndex
) {
    for (std::size_t index = 0;
         index <
             DetailedBodyQuads;
         ++index) {
        ClearQuad(
            bodyVertices,
            antIndex *
                DetailedBodyQuads +
                index);
    }

    for (std::size_t index = 0;
         index <
             DetailedLegQuads;
         ++index) {
        ClearQuad(
            legVertices,
            antIndex *
                DetailedLegQuads +
                index);
    }

    ClearQuad(
        foodVertices,
        antIndex);
}

void AntGeometry::ClearSimple(
    const std::size_t antIndex
) {
    ClearQuad(
        bodyVertices,
        antIndex);

    ClearQuad(
        foodVertices,
        antIndex);
}

std::span<const pipeframe::Vertex2D>
AntGeometry::GetBodyVertices() const {
    return bodyVertices;
}

std::span<const pipeframe::Vertex2D>
AntGeometry::GetLegVertices() const {
    return legVertices;
}

std::span<const pipeframe::Vertex2D>
AntGeometry::GetFoodVertices() const {
    return foodVertices;
}

pipeframe::Color AntGeometry::GetAntColor(
    const AntView &ant,
    const AntConfiguration &configuration
) {
    if (!configuration.dynamicAntColor) {
        return ant.Identity().color;
    }

    return 
        ant.IsCarryingFood()
            ? configuration.toHomeAntColor
            : configuration.toFoodAntColor;
}

void AntGeometry::WriteQuad(
    std::vector<pipeframe::Vertex2D> &vertices,
    const std::size_t quadIndex,
    const pipeframe::Vector2f center,
    pipeframe::Vector2f direction,
    const pipeframe::Vector2f size,
    const pipeframe::Vector2f textureMinimum,
    const pipeframe::Vector2f textureMaximum,
    const pipeframe::Color color
) {
    const std::size_t first =
        quadIndex *
        VerticesPerQuad;

    if (first +
            VerticesPerQuad >
        vertices.size()) {
        return;
    }

    direction =
        Normalize(direction);

    const pipeframe::Vector2f perpendicular{
        -direction.y,
        direction.x,
    };

    const pipeframe::Vector2f horizontal =
        direction *
        (size.x * 0.5f);

    const pipeframe::Vector2f vertical =
        perpendicular *
        (size.y * 0.5f);

    const std::array<pipeframe::Vector2f, 4>
        positions{
            center - horizontal -
                vertical,
            center + horizontal -
                vertical,
            center + horizontal +
                vertical,
            center - horizontal +
                vertical,
        };

    const std::array<pipeframe::Vector2f, 4>
        textureCoordinates{
            pipeframe::Vector2f{
                textureMinimum.x,
                textureMinimum.y,
            },
            pipeframe::Vector2f{
                textureMaximum.x,
                textureMinimum.y,
            },
            pipeframe::Vector2f{
                textureMaximum.x,
                textureMaximum.y,
            },
            pipeframe::Vector2f{
                textureMinimum.x,
                textureMaximum.y,
            },
        };

    constexpr std::array<std::size_t, 6>
        indices{
            0,
            1,
            2,
            0,
            2,
            3,
        };

    for (std::size_t index = 0;
         index <
             VerticesPerQuad;
         ++index) {
        const std::size_t corner =
            indices[index];

        vertices[first + index].position =
            positions[corner];

        vertices[first + index].color =
            color;

        vertices[first + index].textureCoordinate =
            textureCoordinates[corner];
    }
}

void AntGeometry::ClearQuad(
    std::vector<pipeframe::Vertex2D> &vertices,
    const std::size_t quadIndex
) {
    const std::size_t first =
        quadIndex *
        VerticesPerQuad;

    if (first +
            VerticesPerQuad >
        vertices.size()) {
        return;
    }

    for (std::size_t index = 0;
         index <
             VerticesPerQuad;
         ++index) {
        vertices[first + index].position = {};
        vertices[first + index].textureCoordinate = {};
        vertices[first + index].color =
            pipeframe::Color::Transparent;
    }
}

void AntGeometry::UpdateFood(
    const AntView &ant,
    const std::size_t antIndex,
    const AntConfiguration &configuration
) {
    if (!ant.IsCarryingFood()) {
        ClearQuad(
            foodVertices,
            antIndex);

        return;
    }

    constexpr float FoodSize{
        0.24f
    };

    constexpr float FoodOffset{
        0.38f * 1.125f
    };

    const pipeframe::Vector2f direction =
        Normalize(
            ant.GetDirection());

    WriteQuad(
        foodVertices,
        antIndex,
        ant.GetPosition() +
            direction *
                FoodOffset,
        {1.0f, 0.0f},
        {FoodSize, FoodSize},
        {},
        {1024.0f, 1024.0f},
        configuration.foodColor);
}

pipeframe::Vector2f AntGeometry::Normalize(
    const pipeframe::Vector2f value
) {
    const float lengthSquared =
        value.x * value.x +
        value.y * value.y;

    if (lengthSquared <= 0.0f) {
        return {
            1.0f,
            0.0f,
        };
    }

    return value /
           std::sqrt(lengthSquared);
}

} // namespace ant_simulation

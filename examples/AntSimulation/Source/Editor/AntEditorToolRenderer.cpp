#include "Editor/AntEditorToolRenderer.h"
#include <PipeFrame/Render/Canvas.h>

#include <array>

namespace ant_simulation {

AntEditorToolRenderer::AntEditorToolRenderer(const AntConfiguration &newConfiguration)
    : configuration(newConfiguration) {}

void AntEditorToolRenderer::Update(const AntEditorTool &tool, const AntEnvironment &environment,
                                   const pipeframe::Rectanglef &viewport) {
    brushVertices.clear();
    pendingVertices.clear();

    visibleBrushCellCount = 0;
    visiblePendingCellCount = 0;

    if (!tool.IsEnabled() || tool.GetMode() == AntEditorToolMode::None) {
        return;
    }

    BuildBrushPreview(tool, environment, viewport);

    BuildPendingPreview(tool, viewport);
}

void AntEditorToolRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    states.texture = nullptr;
    states.blendMode = pipeframe::BlendMode::Alpha;

    // Pending edits are drawn first. The live cursor preview is then
    // drawn over them, matching the AntPezza editor behavior.
    if (!pendingVertices.empty()) {
        target.Draw(pendingVertices.data(), pendingVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (!brushVertices.empty()) {
        target.Draw(brushVertices.data(), brushVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }
}

std::size_t AntEditorToolRenderer::GetVisibleBrushCellCount() const { return visibleBrushCellCount; }

std::size_t AntEditorToolRenderer::GetVisiblePendingCellCount() const { return visiblePendingCellCount; }

std::span<const pipeframe::Vertex2D> AntEditorToolRenderer::GetBrushVertices() const { return brushVertices; }

std::span<const pipeframe::Vertex2D> AntEditorToolRenderer::GetPendingVertices() const { return pendingVertices; }

pipeframe::Color AntEditorToolRenderer::GetPreviewColor(const AntEditorToolMode mode) const {
    switch (mode) {
    case AntEditorToolMode::AddFood:
        return {
            configuration.foodColor.r,
            configuration.foodColor.g,
            configuration.foodColor.b,
            PreviewAlpha,
        };

    case AntEditorToolMode::AddWall:
        return WallPreviewColor;

    case AntEditorToolMode::Erase:
        return ErasePreviewColor;

    case AntEditorToolMode::None:
        return pipeframe::Color::Transparent;
    }

    return pipeframe::Color::Transparent;
}

void AntEditorToolRenderer::BuildBrushPreview(const AntEditorTool &tool, const AntEnvironment &environment,
                                              const pipeframe::Rectanglef &viewport) {
    const pipeframe::Color color = GetPreviewColor(tool.GetMode());

    if (color.a == 0) {
        return;
    }

    const pipeframe::Vector2f modelPosition = tool.GetPosition();
    const pipeframe::Vector2f position = modelPosition;

    const float radius = tool.GetRadius();

    const float radiusSquared = radius * radius;

    const int integerRadius = static_cast<int>(radius);

    const pipeframe::Vector2i centerCell = AntEnvironment::WorldToCell(modelPosition);

    const std::size_t maximumCellCount =
        static_cast<std::size_t>(integerRadius * 2 + 1) * static_cast<std::size_t>(integerRadius * 2 + 1);

    brushVertices.reserve(maximumCellCount * 6);

    for (int y = centerCell.y - integerRadius; y <= centerCell.y + integerRadius; ++y) {
        for (int x = centerCell.x - integerRadius; x <= centerCell.x + integerRadius; ++x) {
            if (!environment.ContainsCell(x, y)) {
                continue;
            }

            const pipeframe::Vector2f cellCenter = AntEnvironment::GetCellCenter({
                x,
                y,
            });

            const pipeframe::Vector2f difference = cellCenter - position;

            const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

            if (distanceSquared >= radiusSquared) {
                continue;
            }

            if (!IsVisible(cellCenter, viewport)) {
                continue;
            }

            AddCellQuad(brushVertices, cellCenter, color);

            ++visibleBrushCellCount;
        }
    }
}

void AntEditorToolRenderer::BuildPendingPreview(const AntEditorTool &tool, const pipeframe::Rectanglef &viewport) {
    const std::span<const pipeframe::Vector2f> pendingCells = tool.GetPendingPreviewCells();

    if (pendingCells.empty()) {
        return;
    }

    const pipeframe::Color color = GetPreviewColor(tool.GetMode());

    if (color.a == 0) {
        return;
    }

    pendingVertices.reserve(pendingCells.size() * 6);

    for (const pipeframe::Vector2f modelCellCenter : pendingCells) {
        const pipeframe::Vector2f cellCenter = modelCellCenter;
        if (!IsVisible(cellCenter, viewport)) {
            continue;
        }

        AddCellQuad(pendingVertices, cellCenter, color);

        ++visiblePendingCellCount;
    }
}

void AntEditorToolRenderer::AddCellQuad(std::vector<pipeframe::Vertex2D> &vertices,
                                        const pipeframe::Vector2f cellCenter, const pipeframe::Color color) {
    constexpr pipeframe::Vector2f halfSize{
        0.5f,
        0.5f,
    };

    const pipeframe::Vector2f northWest{
        cellCenter.x - halfSize.x,
        cellCenter.y - halfSize.y,
    };

    const pipeframe::Vector2f northEast{
        cellCenter.x + halfSize.x,
        cellCenter.y - halfSize.y,
    };

    const pipeframe::Vector2f southWest{
        cellCenter.x - halfSize.x,
        cellCenter.y + halfSize.y,
    };

    const pipeframe::Vector2f southEast{
        cellCenter.x + halfSize.x,
        cellCenter.y + halfSize.y,
    };

    const std::array<pipeframe::Vector2f, 6> positions{
        northWest, northEast, southWest, southWest, southEast, northEast,
    };

    for (const pipeframe::Vector2f position : positions) {
        pipeframe::Vertex2D vertex;

        vertex.position = position;
        vertex.color = color;
        vertex.textureCoordinate = {};

        vertices.push_back(vertex);
    }
}

bool AntEditorToolRenderer::IsVisible(const pipeframe::Vector2f cellCenter, const pipeframe::Rectanglef &viewport) {
    constexpr float radius{0.5f};

    const float left = viewport.position.x;

    const float top = viewport.position.y;

    const float right = viewport.position.x + viewport.size.x;

    const float bottom = viewport.position.y + viewport.size.y;

    return cellCenter.x + radius >= left && cellCenter.x - radius <= right && cellCenter.y + radius >= top &&
           cellCenter.y - radius <= bottom;
}

} // namespace ant_simulation

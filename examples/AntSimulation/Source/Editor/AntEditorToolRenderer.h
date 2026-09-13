#include <PipeFrame/World/World.h>
#ifndef ANT_EDITOR_TOOL_RENDERER_H
#define ANT_EDITOR_TOOL_RENDERER_H

#include <cstddef>
#include <span>
#include <vector>

#include "Configuration/AntConfiguration.h"
#include "Editor/AntEditorTool.h"
#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {

class AntEditorToolRenderer final : public pipeframe::RenderLayer {
  public:
    static constexpr std::uint8_t PreviewAlpha{100};

    static constexpr pipeframe::Color WallPreviewColor{
        95,
        168,
        211,
        PreviewAlpha,
    };

    static constexpr pipeframe::Color ErasePreviewColor{
        255,
        0,
        0,
        PreviewAlpha,
    };

    explicit AntEditorToolRenderer(const AntConfiguration &configuration);

    void Update(const AntEditorTool &tool, const AntEnvironment &environment, const pipeframe::Rectanglef &viewport);

    void Draw(pipeframe::Canvas target, pipeframe::RenderState states = pipeframe::RenderState::Default) const override;

    [[nodiscard]]
    std::size_t GetVisibleBrushCellCount() const;

    [[nodiscard]]
    std::size_t GetVisiblePendingCellCount() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetBrushVertices() const;

    [[nodiscard]]
    std::span<const pipeframe::Vertex2D> GetPendingVertices() const;

    [[nodiscard]]
    pipeframe::Color GetPreviewColor(AntEditorToolMode mode) const;

  private:
    void BuildBrushPreview(const AntEditorTool &tool, const AntEnvironment &environment,
                           const pipeframe::Rectanglef &viewport);

    void BuildPendingPreview(const AntEditorTool &tool, const pipeframe::Rectanglef &viewport);

    static void AddCellQuad(std::vector<pipeframe::Vertex2D> &vertices, pipeframe::Vector2f cellCenter,
                            pipeframe::Color color);

    [[nodiscard]]
    static bool IsVisible(pipeframe::Vector2f cellCenter, const pipeframe::Rectanglef &viewport);

    const AntConfiguration &configuration;

    std::size_t visibleBrushCellCount{0};
    std::size_t visiblePendingCellCount{0};

    std::vector<pipeframe::Vertex2D> brushVertices;
    std::vector<pipeframe::Vertex2D> pendingVertices;
};

} // namespace ant_simulation

#endif
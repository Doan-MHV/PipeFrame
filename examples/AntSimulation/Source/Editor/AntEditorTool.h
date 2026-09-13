#ifndef ANT_EDITOR_TOOL_H
#define ANT_EDITOR_TOOL_H

#include <cstddef>
#include <span>
#include <vector>

#include <PipeFrame/Editor/EditorTool.h>
#include <PipeFrame/Foundation/MathTypes.h>

#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {

enum class AntEditorToolMode {
    None = 0,
    Erase = 1,
    AddFood = 2,
    AddWall = 3,
};

class AntEditorTool final : public pipeframe::EditorTool {
  public:
    static constexpr float MinimumRadius{0.5f};

    static constexpr float MaximumRadius{128.0f};

    static constexpr std::size_t DefaultFoodQuantity{10};

    AntEditorTool() = default;

    [[nodiscard]] std::string_view GetToolId() const override { return "ant.world-brush"; }
    void SetEnabled(bool enabled) override;

    void SetMode(AntEditorToolMode mode);

    void SetRadius(float radius);

    void SetFoodQuantity(std::size_t quantity);

    void SetPosition(pipeframe::Vector2f position);

    std::size_t BeginStroke(AntEnvironment &environment);

    std::size_t UpdateStroke(AntEnvironment &environment);

    std::size_t EndStroke(AntEnvironment &environment);

    void CancelStroke(AntEnvironment &environment);

    void CycleMode();

    [[nodiscard]]
    bool IsEnabled() const override;

    [[nodiscard]]
    bool IsStrokeActive() const;

    [[nodiscard]]
    AntEditorToolMode GetMode() const;

    [[nodiscard]]
    float GetRadius() const;

    [[nodiscard]]
    std::size_t GetFoodQuantity() const;

    [[nodiscard]]
    pipeframe::Vector2f GetPosition() const;

    [[nodiscard]]
    std::span<const pipeframe::Vector2f> GetPendingPreviewCells() const;

    [[nodiscard]]
    static const char *GetModeName(AntEditorToolMode mode);

  private:
    std::size_t Apply(AntEnvironment &environment);

    void ClearPendingRequests(AntEnvironment &environment);

    template <typename Callback> void ForEachBrushCell(const AntEnvironment &environment, Callback &&callback) const;

    bool enabled{false};
    bool strokeActive{false};
    bool hasAppliedPosition{false};

    AntEditorToolMode mode{AntEditorToolMode::None};

    float radius{8.0f};

    std::size_t foodQuantity{DefaultFoodQuantity};

    pipeframe::Vector2f position{};
    pipeframe::Vector2f lastAppliedPosition{};

    std::vector<pipeframe::Vector2f> pendingPreviewCells;
};

template <typename Callback>
void AntEditorTool::ForEachBrushCell(const AntEnvironment &environment, Callback &&callback) const {
    const pipeframe::Vector2i centerCell = AntEnvironment::WorldToCell(position);

    const int integerRadius = static_cast<int>(radius);

    const float radiusSquared = radius * radius;

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

            callback(cellCenter);
        }
    }
}

} // namespace ant_simulation

#endif

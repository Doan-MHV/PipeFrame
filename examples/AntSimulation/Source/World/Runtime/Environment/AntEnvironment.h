#ifndef ANT_ENVIRONMENT_H
#define ANT_ENVIRONMENT_H

#include <PipeFrame/Simulation/System.h>
#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Food.h"

namespace ant_simulation {

class AntEnvironment final : public pipeframe::FixedUpdateSystem<void> {
  public:
    static constexpr int BorderMargin{2};

    AntEnvironment() = default;

    explicit AntEnvironment(
        const AntConfiguration &configuration
    );

    bool Initialize(
        const AntConfiguration &configuration,
        std::string &errorMessage
    );

    void Clear();

    std::string_view GetSystemId() const override {return "ant.environment";}
    void Update(float deltaTime) override;

    [[nodiscard]]
    const AntConfiguration &GetConfiguration() const;

    [[nodiscard]]
    int GetWidth() const;

    [[nodiscard]]
    int GetHeight() const;

    [[nodiscard]]
    std::size_t GetCellCount() const;

    [[nodiscard]]
    bool IsInitialized() const;

    [[nodiscard]]
    bool ContainsCell(
        int x,
        int y
    ) const;

    [[nodiscard]]
    bool IsSimulationPositionValid(
        pipeframe::Vector2f worldPosition
    ) const;

    [[nodiscard]]
    static pipeframe::Vector2i WorldToCell(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    static pipeframe::Vector2f GetCellCenter(
        pipeframe::Vector2i cellPosition
    );

    [[nodiscard]]
    AntWorldCell *TryGetCell(
        int x,
        int y
    );

    [[nodiscard]]
    const AntWorldCell *TryGetCell(
        int x,
        int y
    ) const;

    [[nodiscard]]
    AntWorldCell *TryGetCell(
        pipeframe::Vector2i cellPosition
    );

    [[nodiscard]]
    const AntWorldCell *TryGetCell(
        pipeframe::Vector2i cellPosition
    ) const;

    [[nodiscard]]
    AntWorldCell *TryGetCellAtWorldPosition(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    const AntWorldCell *TryGetCellAtWorldPosition(
        pipeframe::Vector2f worldPosition
    ) const;

    [[nodiscard]]
    std::span<AntWorldCell> GetCells();

    [[nodiscard]]
    std::span<const AntWorldCell> GetCells() const;

    [[nodiscard]]
    std::span<const Food> GetFoodEntities() const;

    [[nodiscard]]
    const Food *FindFoodEntity(
        WorldEntityId id
    ) const;

    [[nodiscard]]
    std::size_t GetTotalFoodQuantity() const;

    [[nodiscard]]
    WorldEntityId AddFood(
        pipeframe::Vector2f worldPosition,
        std::size_t quantity
    );

    [[nodiscard]]
    std::size_t AddFoodPatch(
        pipeframe::Vector2f center,
        float radius,
        std::size_t quantityPerCell
    );

    [[nodiscard]]
    std::size_t ConsumeFood(
        pipeframe::Vector2f worldPosition,
        std::size_t requestedQuantity = 1
    );

    [[nodiscard]]
    bool RemoveFood(
        pipeframe::Vector2f worldPosition
    );

    void ClearAllFood();

    [[nodiscard]]
    bool AddWall(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    bool RemoveWall(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    std::size_t GetWallCount() const;

    [[nodiscard]]
    bool MarkCellForWall(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    bool MarkCellForErase(
        pipeframe::Vector2f worldPosition
    );

    [[nodiscard]]
    std::size_t MarkWallBrush(
        pipeframe::Vector2f center,
        float radius
    );

    [[nodiscard]]
    std::size_t MarkEraseBrush(
        pipeframe::Vector2f center,
        float radius
    );

    [[nodiscard]]
    std::size_t ApplyWallRequests();

    [[nodiscard]]
    std::size_t ApplyEraseRequests();

    void CreateBorderWalls();

  private:
    [[nodiscard]]
    std::size_t GetCellIndex(
        int x,
        int y
    ) const;

    [[nodiscard]]
    WorldEntityId CreateFoodEntity(
        pipeframe::Vector2f position
    );

    void RemoveFoodEntity(
        WorldEntityId id
    );

    AntConfiguration configuration;
    std::vector<AntWorldCell> cells;

    std::vector<Food> foodEntities;

    std::unordered_map<
        WorldEntityId,
        std::size_t
    > foodEntityIndices;

    WorldEntityId nextFoodEntityId{1};

    int width{0};
    int height{0};
};

} // namespace ant_simulation

#endif